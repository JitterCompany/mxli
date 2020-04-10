//HEADER
#include <stringTable.h>
#include <fifoParse.h>

//SLICE
const char* stringTableWriteString (StringTable *st, const char* string) {
	const char *start = st->buffer + st->wPos;

	while (st->wPos < st->bufferSize && *string!=0) {	// copy all 'visible' characters
		st->buffer[st->wPos] = *string;
		st->wPos++;
		string++;
	}
	if (st->wPos < st->bufferSize) {
		st->buffer[st->wPos] = 0;	// terminate string
		st->wPos++;
		return *string==0 ? start : 0;
	}
	else return false;	// no space left for trailing \0
}

//SLICE
const char* stringTableWriteStringUnique (StringTable *st, const char* string) {
	const char *known = stringTableLookupString (st,string);
	if (known!=0) return known;
	else return stringTableWriteString (st,string);
}

//SLICE
const char* stringTableWriteFifo (StringTable *st, const Fifo *string) {
	Fifo clone = *string;
	const char *start = st->buffer + st->wPos;

	while (st->wPos < st->bufferSize && fifoCanRead (&clone)) {	// copy all 'visible' characters
		st->buffer[st->wPos] = fifoRead (&clone);
		st->wPos++;
	}
	if (st->wPos < st->bufferSize) {
		st->buffer[st->wPos] = 0;			// terminate string
		st->wPos++;
		return !fifoCanRead (&clone) ? start : 0;	// no space left for string copying
	}
	else return false;	// no space left for trailing \0
}

//SLICE
const char* stringTableWriteFifoUnique (StringTable *st, const Fifo *string) {
	const char* known = stringTableLookupFifo (st,string);
	if (known) return known;
	else return stringTableWriteFifo (st,string);
}

//SLICE
const char* stringTableNextString (const StringTable *stringTable, const char* string) {
	if (string!=0) {
		size_t index = string-stringTable->buffer;
		// find end
		while (index<stringTable->wPos && stringTable->buffer[index]!=0) index++;

		// skip 0 or wPos
		index++;

		if (index<stringTable->wPos) return &stringTable->buffer[index];
		else return 0;	// end reached.
	}
	else return 0;
}

/** Inefficient function for looking up a string in the table.
 * @param stringTable the StringTable object to use
 * @param string a 0-terminated String to be searched for.
 * @return a pointer to the copy of the string inside the table or 0 if no such string is found.
 */
const char* stringTableLookupString (StringTable *stringTable, const char *string) {
	const char *element = stringTableFirstString (stringTable);
	while (element!=0) {
		if (streq (element,string)) return element;
		element = stringTableNextString (stringTable,element);
	}
	return 0;	// not found
}

/** Inefficient function for looking up a string in the table.
 * @param stringTable the StringTable object to use
 * @param string a Fifo containing (only) the string to be searched for.
 * @return a pointer to the copy of the string inside the table or 0 if no such string is found.
 */
const char* stringTableLookupFifo (StringTable *stringTable, const Fifo *string) {
	const char *element = stringTableFirstString (stringTable);
	while (element!=0) {
		Fifo clone = *string;
		if (fifoParseExactString (&clone,element) && !fifoCanRead(&clone)) return element;
		element = stringTableNextString (stringTable,element);
	}
	return 0;	// not found
}

