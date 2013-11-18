#include "TQueue.h"

#define self this

#if TQUEUE_USE_CAS_VERSION

TQueue::TQueue() {
   self->head = &self->tail;
   self->tail.next = 0;
}

void TQueue::push(Node* n) {
#if TQ_USE_MUTEX_TEMP
	Lock l(mutex);
#endif

   n->next = 0;
   Node* prev = (Node*) TAtomic::AtomicExchange(&self->head, n);
   prev->next = n;
}

Node* TQueue::pop() {
#if TQ_USE_MUTEX_TEMP
	Lock l(mutex);
#endif
	
   Node* tail = self->tail.next;
   if (0 == tail)
		return 0;
	
   Node* next = tail->next;
   if (next) {
		self->tail.next = next;
		return tail;
   }
	
   Node* head = self->head;
   if (tail != head)
		return 0;
	
   self->tail.next = 0;
   if (TCompareAndSwapPtrBarrier(&self->head, head, &self->tail)) {
		return tail;
		
   }
   next = tail->next;
   if (next) {
		self->tail.next = next;
		return tail;
   }
   return 0;
}

#else

TQueue::TQueue() {
	stub.next = 0;
	this->head = &stub;
	this->tail = &stub;
}

void TQueue::push(Node* n) {
#if TQ_USE_MUTEX_TEMP
	Lock l(mutex);
#endif
	
	n->next = nullptr;

	Node* prev = (Node*) TAtomicExchange(&this->head, (void*) n);
	prev->next = n;
}

TQueue::Node* TQueue::peek() {
#if TQ_USE_MUTEX_TEMP
	Lock l(mutex);
#endif
	
	Node* tail = this->tail;
	Node* next = tail->next;
	if (tail == &this->stub) {
		return next;
	}
	else {
		return tail;
	}
}

TQueue::Node* TQueue::pop() {	
#if TQ_USE_MUTEX_TEMP
	Lock l(mutex);
#endif
	
	Node* tail = this->tail;
	Node* next = tail->next;
	if (tail == &this->stub) {
		if (0 == next)
			return 0;
		this->tail = next;
		tail = next;
		next = next->next;
	}
	
	if (next) {
		this->tail = next;
		return tail;
	}
	
	Node* headT = (Node*) this->head;
	if (tail != headT)
		return 0;
	
	push(&this->stub);
	next = tail->next;
	if (next) {
		this->tail = next;
		return tail;
	}
	return 0;
}

#endif
