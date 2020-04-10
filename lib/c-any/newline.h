#ifndef __newline_h
#define __newline_h

typedef enum {
	NEWLINE_LF,	///< Unix-style line feed
	NEWLINE_CR,	///< Mac-style and terminal line feed
	NEWLINE_CRLF,	///< DOS/Windows-style line feed
	NEWLINE_UNKNOWN	///< auto-detect line feed style
} NewlineStyle;

#endif

