#ifndef _PrintfBuffer_h
#define _PrintfBuffer_h

#include "TCommon.h"
#include "Nullable.h"
#include "TVector.h"

#ifdef __cplusplus

#include <string>

/**
 Like ostringstream but for printf formatting - stores a buffer which can write to with multiple
 printf-style calls.
 
 Also has some custom HTML-syntax highlighting when printWithErrorHighlighting() is used.
 */
class PrintfBuf {
private:
	TVector<char> buf;
	
	int index = 0;
	
	DISALLOW_COPY_AND_ASSIGN(PrintfBuf);
	
public:
	
	bool allowingHtml = true;
	bool inTable = false;
	bool inList = false;
	int tableRowNumber = 0;
	
	PrintfBuf(int initialBufferSize=2048) {
		buf.resize(initialBufferSize);
	}
	
	/// Returns how much room is left in the buffer.
	int charsRemaining() { return static_cast<int>(buf.size())-index; }
	
	/**
	 Works just like vprintf, appending the output to the internal buffer.
	 */
	void vprintf(const char* s, va_list args);
	
	/**
	 Works just like printf, appending the output to the internal buffer.
	 */
	void printf(const char* s, ...);
	
	void endListIfIn();
	void addHeading(const char* s, ...);
	void addListItem(const char* s, ...);
	void addListItemWithErrorHighlighting(bool isError, const char* s, ...);
	void addLine(const char* s, ...);
	void addLineWithErrorHighlighting(bool isError, const char* s, ...);
	void addOpeningErrorHighlighting(bool isError);
	void addClosingErrorHighlighting(bool isError);
	
	/**
	 Will set the given output to be either red+bold, or green, depending upon whether isError is
	 true or false respectively, using an HTML span style.
	 
	 This is more of a custom function we used when outputting certain things to a UIWebView.
	 */
	void printWithErrorHighlighting(bool isError, const char* s, ...);
	
	/**
	 Returns a reference to the beginning of the buffer as a C-style string.
	 */
	char* c_str() { return &buf[0]; }

	/// Returns a copy of the buffer as a string.
	std::string getString() { return &buf[0]; }

	/// Returns a copy of the buffer as a string.
	std::string str() { return &buf[0]; }
	
	int size() { return index; }
	
	void reset();
};


class ScopedTable {
	PrintfBuf& buf;
public:
	ScopedTable(PrintfBuf& _buf);
	~ScopedTable();
};

enum TableRowAlignment { TRA_LEFT, TRA_CENTER, TRA_RIGHT };

class ScopedTableRow {
	PrintfBuf& buf;
	PrintfBuf cachedRow; //TODO: inefficient
	NullableBool isError;
	int cellN = 0;
public:
	void setIsError(bool b) {
		isError = b;
	}
	ScopedTableRow(PrintfBuf& _buf);
	~ScopedTableRow();
	void addCell(TableRowAlignment alignment, const char* heading, const char* s, ...);
};

#endif
#endif
