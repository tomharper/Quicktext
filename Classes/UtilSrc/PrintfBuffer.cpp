#include "PrintfBuffer.h"
#include "TLogging.h"

#pragma mark - PrintfBuf

#if 0
#define OUTPUT_VARGS() \
	va_list ap; \
	va_start(ap, s); \
	index += vsprintf(&buf[index], s, ap); \
	va_end(ap);
#else
#define OUTPUT_VARGS(printfBuf) \
	va_list ap; \
	va_start(ap, s); \
	(printfBuf).vprintf(s, ap); \
	va_end(ap);
#endif


#define getLength(__s, __args) ::vsnprintf(nullptr, 0, __s, __args)
/*
int getLength(const char* s, va_list args) {
	//
	// Make sure to copy args, as they are
	// clobbered on 64bit systems
	//
	va_list cargs;
	va_copy(cargs, args);
	
	int mlen = ::vsnprintf(nullptr, 0, s, cargs);
	
	va_end(cargs);
	
	return mlen;
}
*/

void PrintfBuf::vprintf(const char* s, va_list args) {
	
	va_list cargs;
	va_copy(cargs, args);
	
	int lengthRequired = getLength(s, cargs);

	va_end(cargs);
	
	int reqSize = index+lengthRequired;
	if(reqSize >= static_cast<int>(buf.size())) {
		int targetSize = buf.size()*2;
		while(reqSize >= targetSize) {
			targetSize *= 2;
		}
		buf.resize(targetSize);
	}
	
	va_list cargs2;
	va_copy(cargs2, args);
	
	index += vsprintf(&buf[index], s, cargs2);
	
	va_end(cargs2);
}

void PrintfBuf::printf(const char* s, ...) {
	OUTPUT_VARGS(*this)
}

void PrintfBuf::endListIfIn() {
	if(inList) {
		if(allowingHtml) {
			printf("</ul>");
		}
	}
	inList = false;
}
void PrintfBuf::addHeading(const char* s, ...) {
	endListIfIn();
	
	if(allowingHtml) {
		printf("<h3>");
		OUTPUT_VARGS(*this);
		printf("</h3>");
	}
	else {
		printf("\n");
		OUTPUT_VARGS(*this);
		printf("\n");
	}
}

void PrintfBuf::addListItem(const char* s, ...) {
	if(!inList) {
		if(allowingHtml) {
			printf("<ul>");
		}
	}
	inList = true;
	
	if(allowingHtml) {
		printf("<li>");
	}
	else {
		printf(" - ");
	}
	OUTPUT_VARGS(*this);
	if(!allowingHtml) {
		printf("\n");
	}
}
void PrintfBuf::addLine(const char* s, ...) {
	endListIfIn();
	
	OUTPUT_VARGS(*this);
	if(allowingHtml) {
		printf("<br/>");
	}
	else {
		printf("\n");
	}
}
void PrintfBuf::addLineWithErrorHighlighting(bool isError, const char* s, ...) {
	endListIfIn();
	
	addOpeningErrorHighlighting(isError);
	
	OUTPUT_VARGS(*this);
	
	addClosingErrorHighlighting(isError);
	
	if(allowingHtml) {
		printf("<br/>");
	}
	else {
		printf("\n");
	}
}

void PrintfBuf::addListItemWithErrorHighlighting(bool isError, const char* s, ...) {
	if(!inList) {
		if(allowingHtml) {
			printf("<ul>");
		}
	}
	inList = true;
	
	if(allowingHtml) {
		printf("<li>");
	}
	else {
		printf(" - ");
	}
	addOpeningErrorHighlighting(isError);
	
	OUTPUT_VARGS(*this);
	
	addClosingErrorHighlighting(isError);
	
	if(!allowingHtml) {
		printf("\n");
	}
}

void PrintfBuf::addOpeningErrorHighlighting(bool isError) {
	if(allowingHtml) {
		if(isError) {
			printf("<span style='color:red; font-weight:bold'>");
		}
		else {
			printf("<span style='color:green'>");
		}
	}
}
void PrintfBuf::addClosingErrorHighlighting(bool isError) {
	if(allowingHtml) {
		printf("</span>");
	}
}

void PrintfBuf::printWithErrorHighlighting(bool isError, const char* s, ...) {
	addOpeningErrorHighlighting(isError);
	
	OUTPUT_VARGS(*this);
	
	addClosingErrorHighlighting(isError);
}

void PrintfBuf::reset() {
	buf[0] = 0;
	index = 0;
	
	inList = false;
}


#pragma mark ScopedTable

ScopedTable::ScopedTable(PrintfBuf& _buf):buf(_buf) {
	buf.inTable = true;
	buf.tableRowNumber = 0;
	
	buf.endListIfIn();
	
	if(buf.allowingHtml) {
		buf.printf("<table>\n");
	}
	else {
		buf.printf("\n");
	}
};
ScopedTable::~ScopedTable() {
	buf.inTable = false;
	
	if(buf.allowingHtml) {
		buf.printf("</table>\n");
	}
	else {
		buf.printf("\n");
	}
}


#pragma mark ScopedTableRow

ScopedTableRow::ScopedTableRow(PrintfBuf& _buf):buf(_buf) {
	cachedRow.allowingHtml = buf.allowingHtml;
	if(buf.allowingHtml) {
		buf.printf("<tr>");
	}
	else {}
};
ScopedTableRow::~ScopedTableRow() {
	if(buf.allowingHtml) {
		buf.printf("</tr>\n");
	}
	else {
		buf.printf("\n");
	}
	if(buf.allowingHtml && buf.tableRowNumber == 0) {
		buf.printf("<tr>%s</tr>\n", cachedRow.c_str());
	}
	buf.tableRowNumber++;
}
void ScopedTableRow::addCell(TableRowAlignment alignment, const char* heading, const char* s, ...) {
	auto* bufT = &buf;
	if(buf.allowingHtml) {
		if(buf.tableRowNumber == 0) {
			if(cellN) {
				buf.printf("<td>&nbsp;</td>");
			}
			buf.printf("<td align='center'><b>%s</b></td>", heading);
			bufT = &cachedRow;
		}
	}
	else {
		if(heading && strlen(heading)) {
			buf.printf("%s:", heading);
		}
	}
	
	if(buf.allowingHtml) {
		if(cellN++) {
			bufT->printf("<td>&nbsp;</td>");
		}
		
		const char* alignmentS = "left";
		if(alignment == TRA_CENTER) {
			alignmentS = "center";
		}
		else if(alignment == TRA_RIGHT) {
			alignmentS = "right";
		}
		bufT->printf("<td align='%s'>", alignmentS);
	}
	
	if(isError.isSet()) {
		bufT->addOpeningErrorHighlighting(isError.getValue());
	}
	OUTPUT_VARGS(*bufT);
	if(isError.isSet()) {
		bufT->addClosingErrorHighlighting(isError.getValue());
	}
	
	if(bufT->allowingHtml) {
		bufT->printf("</td>");
	}
	else {
		if(cellN++) {
			buf.printf(" ");
		}
	}
}
