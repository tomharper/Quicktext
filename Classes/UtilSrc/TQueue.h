#ifndef _TQueue_h
#define _TQueue_h

#include "TCommon.h"
#include "TAtomic.h"
#include <utility> // std::move
#include "Mutex.h"

/**
 Based on http://www.1024cores.net/home/lock-free-algorithms/queues/non-intrusive-mpsc-node-based-queue +
   https://groups.google.com/forum/?hl=ru&fromgroups#!topic/comp.programming.threads/M_ecdRRlgvM[1-25] :
 
	 Advantages:
	 + Waitfree and fast producers. One XCHG is maximum what one can get with multi-producer non-distributed queue.
	 + Extremely fast consumer. On fast-path it's atomic-free, XCHG executed per node batch, in order to grab 'last item'.
	 + No need for node order reversion. So pop operation is always O(1).
	 + ABA-free.
	 + No need for PDR. That is, one can use this algorithm out-of-the-box. No need for thread registration/deregistration, periodic activity, deferred garbage etc.
	 
	 Disadvantages:
	 - Push function is blocking wrt consumer. I.e. if producer blocked in (*), then consumer is blocked too. Fortunately 'window of inconsistency' is extremely small - producer must be blocked exactly in (*). Actually it's disadvantage only as compared with totally lockfree algorithm. It's still much better lock-based algorithm.
 
  These qualities make it perfect for use in offloading logging to a separate thread, such that logging never blocks any criticical threads
  - we have exactly this multiple-producer + single-consumer scenario
  - the multiple producers are fast+waitfree, which is what we need
  - we don't care that the consumer [log output thread] could be very rarely be blocked)
 
  TQueue   - intrusive - inherit TQueue::Node with whatever data you want stored in the queue elements ...
  TQueueNI - non-intrusive ...
 */

#if TARGET_OS_MAC
#	define TQ_USE_MUTEX_TEMP 1
#else
#	define TQ_USE_MUTEX_TEMP 0
#endif

#if TQ_USE_MUTEX_TEMP
#	include "Mutex.h"
#endif


//
// the author of this queue suggested an alternative CAS-using one, which avoids the need of the dummy node ...
// - however the CAS version contains _both_ CAS + XCHG (whereas the other contains just XCHG), which can't help performance
// - plus, would have to check whether the CAS-use could cause potential ABA issues
// - thus, using the normal, CAS-free, version
//
#define TQUEUE_USE_CAS_VERSION 0
#if TQUEUE_USE_CAS_VERSION

class TQueue {
public:
	struct Node {
		Node* volatile next;
		~Node() {}
	};
	
private:
   Node* volatile  head;
   Node tail;
	
#if TQ_USE_MUTEX_TEMP
	mutable Mutex mutex;
#endif
	
public:
	TQueue();
	
	void push(Node* n);
	Node* pop();
};

#else // #if TQUEUE_USE_CAS_VERSION

class TQueue {
public:
	struct Node {
		Node* volatile  next;
		~Node() {}
	};
	
private:
	Node stub;
	
#if TQ_USE_MUTEX_TEMP
	mutable Mutex mutex;
#endif
	
#if USE_CPP11_EXCHANGE
	std::atomic<void*> head;
#else
	void* volatile head;
#endif
	Node* tail;
	
public:
	TQueue();
	
	void push(Node* n);
	Node* pop();
	
	// added this - a bit unsure about the thread-safety of this as implemented exactly, but at least for SRSW case could be ok ...
	Node* peek();
};

#endif // #if TQUEUE_USE_CAS_VERSION


class TQueueWithCounter {
	TQueue zq;
	AtomicInt count;

public:

	void push(TQueue::Node* n) {
		++count;
		zq.push(n);
	}
	
	TQueue::Node* peek() {
		return zq.peek();
	}
	TQueue::Node* pop() {
		TQueue::Node* result = zq.pop();
		if(result) {
			--count;
		}
		return result;
	}
	
	int getCount() {
		return count.Value();
	}
	int size() { return count.Value(); }
};


//
// implementing the non-instrusive version by wrapping the instrusive one ....
//
#if 1
template<class T>
class TQueueNI {
public:
	struct Node : TQueue::Node {
		T state;
		Node() {}
		Node(T&& _state):state(std::move(_state)) {}
		Node(const T& _state):state(_state) {}
		~Node() {}
	};
	
private:
	TQueue zq;

	void push(Node* n) { zq.push(n); }
	Node* pop() { return (Node*) zq.pop(); }
	Node* peek() { return (Node*) zq.peek(); }
	
public:
	TQueueNI() {}

	void push(T&& t) { push(new Node(std::move(t))); }
	void push(const T& t)  { push(new Node(t)); }
	
	/**
	 Returns true if able to fetch one element (i.e. queue not empty), and in this case sets t to the element fetched.
	 */
	bool pop(T& t) {
		Node* n = pop();
		if(n) {
			t = n->state;
			delete n;
			return true;
		}
		else {
			return false;
		}
	}
	bool peek(T& t) {
		Node* n = peek();
		if(n) {
			t = n->state;
			return true;
		}
		else {
			return false;
		}
	}
};

#else // obselete

template<class T>
class TQueueNI {
public:
	struct Node {
		Node* volatile next = nullptr;
		T state;
		Node() {}
		Node(T&& _state):state(std::move(_state)) {}
		Node(const T& _state):state(_state) {}
		~Node() {}
	};
	
private:
	Node stub;
	
#if USE_CPP11_EXCHANGE
	std::atomic<void*> head;
#else
	void* volatile head;
#endif
	
	Node* tail;
	
public:
	TQueueNI() {
		this->head = &stub;
		this->tail = &stub;
	}
	
	/// don't normally call this directly, unless like want to handle memory management of nodes ...
	void push(Node* n) {
		n->next = nullptr;
		
		Node* prev = (Node*) TAtomic::AtomicExchange(&this->head, (void*) n); // serialization-point wrt producers, acquire-release
		prev->next = n; // serialization-point wrt consumer, release
	}
	
	/// don't normally call this directly, unless like want to handle memory management of nodes ...
	Node* pop() {
		Node* tail = this->tail;
		Node* next = tail->next; // serialization-point wrt producers, acquire
		if (next) {
			this->tail = next;
			tail->state = next->state;
			return tail;
		}
		return 0;
	}
	
	
	void push(T&& t) {
		push(new Node(std::move(t)));
	}
	void push(const T& t) {
		push(new Node(t));
	}
	
	/**
	 Returns true if able to fetch one element (i.e. queue not empty), and in this case sets t to the element fetched.
	 */
	bool pop(T& t) {
		Node* n = pop();
		if(n) {
			t = n->state;
			delete n;
			return true;
		}
		else {
			return false;
		}
	}
};
#endif

template<class T>
class TQueueNIWithCounter {
	TQueueNI<T> zq;
	AtomicInt count;
	Mutex readMutex;
	
public:
	
	/*
	 void push(typename TQueueNI<T>::Node* n) {
	 ++count;
	 zq.push(n);
	 }
	 typename TQueueNI<T>::Node* pop() {
	 --count;
	 return zq.pop();
	 }
	 */
	
	void push(T&& t) {
		++count;
		zq.push(std::move(t));
	}
	void push(const T& t) {
		++count;
		zq.push(t);
	}
	
	bool pop(T& t) {
		bool result = zq.pop(t);
		if(result) {
			--count;
		}
		return result;
	}

	bool popWithLock(T& t) {
		Lock l(readMutex);
		bool result = zq.pop(t);
		l.unlock();
		
		if(result) {
			--count;
		}
		return result;
	}
	
	bool peek(T& t) {
		return zq.peek(t);
	}
	
	int getCount() { return count.Value(); }
	int size() { return count.Value(); }
};

#endif // #ifndef _TQueue_h
