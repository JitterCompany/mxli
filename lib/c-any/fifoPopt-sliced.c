//HEADER
/*
  fifoPopt-sliced.c 
  Copyright 2012-2013 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

// TODO: allow same option chars to appear more than once, see fifoPoptInt32Range().

#include <fifoPopt.h>
#include <fifoParse.h>
#include <fifoPrint.h>

#define LONG_OPTIONS

//SLICE
bool fifoPoptScanOption(Fifo *argList, char shortOption, const char *longOption) {
	Fifo clone = *argList;
	while (fifoCanRead(&clone)) {
		Fifo option;
		if (fifoqParseHead(&clone,&option)) {
			if (fifoParseExactChar(&option,'-')
			&& (	fifoParseExactChar(&option,shortOption)
				|| longOption!=0
				&& fifoParseExactChar(&option,'-') && fifoParseExactString(&option,longOption)
			)) return true;
		}
		else return false;
	}
	return false;
}

//SLICE
bool fifoPoptAccumulateCommandLine(Fifo *argList, int argc, const char* const* argv) {
	bool success = true;
	for (int a=1; a<argc; a++) success = success && fifoqPrintString(argList,argv[a]);
	return success;
}

bool fifoPoptShort(Fifo *fifo, char option, bool minus) {
	Fifo clone = *fifo;
	if (	option!=0				// option disabled
	&&	(minus					// continuation of a block
		|| fifoParseExactChar(&clone,'-'))	// new block start
	&& fifoParseExactChar(&clone,option)		// option char match
	) {
		fifoCopyReadPosition(fifo,&clone);	// remove '-' and option char and optionally Z.
		return true;
	}
	else return false;
}

bool fifoPoptLong(Fifo *fifo, const char* longOption, bool minus) {
	Fifo clone = *fifo;
	if (longOption!=0				// option disabled
	&& ! minus					// NOT a continuation of a single option block
	&& fifoParseExactString(&clone,"--")		// long option start
	&& fifoParseExactString(&clone,longOption)	// option name match
	) {
		fifoCopyReadPosition(fifo,&clone);
		return true;
	}
	else return false;
}

bool fifoPoptShortOrLong(Fifo *fifo, char option, const char* longOption, bool minus) {
	// this could be optimized... :o)
	return fifoPoptShort(fifo,option,minus) || fifoPoptLong(fifo,longOption,minus);
}

//SLICE
bool fifoPoptShortOrLongParam (Fifoq *fifo, char shortOption, const char* longOption, bool minus) {
	Fifo clone = *fifo;
	if (fifoPoptShort (&clone,shortOption,minus)) {			// parse short
		fifoParseZ (&clone);	// remove optional ' '
		fifoCopyReadPosition (fifo,&clone);
		return true;
	}
	else if (fifoPoptLong (&clone,longOption,minus)	// parse --long=value or --long value
	&& (	fifoParseExactChar (&clone,'=')		// or a '=' ..
		|| fifoParseZ (&clone))			// or a ' '
	) {	
		fifoCopyReadPosition(fifo,&clone);
		return true;
	}
	else return false;
}

//SLICE
bool fifoPoptBool (Fifo *fifo, const FifoPoptBool *options, bool *minus) {
	Fifo clone = *fifo;
	for (int o=0; options[o].shortOption!=0 || options[o].longOption!=0; ++o) {
		if (fifoPoptShort(&clone,options[o].shortOption, *minus)) {
			* options[o].value = true;
			fifoCopyReadPosition (fifo,&clone);
			*minus = !fifoParseZ (fifo);	// remove trailing Z (' ')
			return true;
		}
		else if (fifoPoptLong(&clone,options[o].longOption, *minus) && fifoParseZ(&clone)) {
			* options[o].value = true;
			fifoCopyReadPosition (fifo,&clone);
			*minus = false;
			return true;
		}
	}
	return false;
}

//SLICE
bool fifoPoptInt32Flag (Fifo *fifo, const FifoPoptInt32Flag *options, bool *minus) {
	Fifo clone = *fifo;
	for (int o=0; options[o].shortOption!=0 || options[o].longOption!=0; ++o) {
		if (fifoPoptShort (&clone,options[o].shortOption,*minus)) {
			* options[o].value = options[o].indicator;
			fifoCopyReadPosition (fifo,&clone);
			*minus = !fifoParseZ (fifo);	// remove trailing Z (' ')
			return true;
		}
		else if (fifoPoptLong (&clone,options[o].longOption,minus) && fifoParseZ (&clone)) {
			* options[o].value = options[o].indicator;
			fifoCopyReadPosition (fifo,&clone);
			*minus = false;
			return true;
		}
	}
	return false;
}

//SLICE
bool fifoPoptInt32 (Fifo *fifo, const FifoPoptInt32 *options, bool *minus) {
	for (int o=0; options[o].shortOption!=0 || options[o].longOption!=0; ++o) {
		const FifoPoptInt32 *option = options + o;
		Fifo clone = *fifo;
		if (fifoPoptShortOrLongParam (&clone,option->shortOption,option->longOption,*minus)) {
			FifoParseInt32 parseInt	= option->parseInt!=0
						? option->parseInt : fifoParseIntCStyle;
			Int32 temp;
			if (parseInt (&clone,&temp)
			&& fifoParseZ (&clone)) {
				* option->value = temp;
				fifoCopyReadPosition (fifo,&clone);
				*minus = false;		// end of block
				return true;	// there's only one option per -
			}
			else return false;	// parameter or spacing missing
		}
	}
	return false;
}

// TODO: continue short/long Option handling here
//
//SLICE
bool fifoPoptInt32Range(Fifo *fifo, const FifoPoptInt32Range *options, bool *minus) {
	for (int o=0; options[o].shortOption!=0 || options[o].longOption!=0; ++o) {
		const FifoPoptInt32Range *option = &options [o];
		Fifo clone = *fifo;		// we allow the same option multiple times.
		if (fifoPoptShortOrLongParam (&clone, option->shortOption, option->longOption, *minus)) {
			const FifoParseInt32 parseInt = option->parseInt!=0
						? option->parseInt : fifoParseIntCStyle;
			const char *separator = option->separator!=0 ? option->separator : "..";
			// option char present
			Int32 from,to;
			if (parseInt (&clone,&from)) {	// single value range
				if (fifoParseZ (&clone)) {
					option->value->fst = from;
					option->value->snd = from;
					fifoCopyReadPosition (fifo,&clone);
					*minus = false;		// end of block
					return true;
				}
				else if	(fifoParseExactString (&clone,separator)	// dual value range
					&& parseInt (&clone,&to)
					&& fifoParseZ (&clone)) {

					options[o].value->fst = from;
					options[o].value->snd = to;
					fifoCopyReadPosition (fifo,&clone);
					*minus = false;		// end of block
					return true;
				}
				else return false;
			}
			// else not an integer
		}
	}
	return false;	// no option found
}

//SLICE
static bool fifoPoptInt32TupleConditional (Fifoq *cmdLine, const FifoPoptInt32Tuple *options, bool setValues, bool *minus) {
	for (int o=0; options[o].shortOption!=0 || options[o].longOption!=0; ++o) {
		const FifoPoptInt32Tuple *option = &options [o];
		Fifo clone = *cmdLine;		// we allow the same option multiple times.
		if (fifoPoptShortOrLongParam (&clone, option->shortOption, option->longOption, *minus)) {
			FifoParseInt32 parseInt	= option->parseInt!=0
						? option->parseInt : fifoParseIntCStyle;
			for (int e=0; e<option->n; ++e) {	// n values
				// check for separator
				if (e!=0 && !fifoParseExactString(&clone,option->separator)) return false;
				// check for value
				Int32 temp;
				if (parseInt(&clone,&temp)) {
					if (setValues) option->values[e] = temp;
				}
				else return false;	// invalid value
			}
			// success!
			if (setValues) {
				fifoCopyReadPosition (cmdLine, &clone);
				*minus = false;
			}
			return true;
		}
	}
	return false;	// no option found
}

bool fifoPoptInt32Tuple (Fifoq *cmdLine, const FifoPoptInt32Tuple *options, bool *minus) {
	return	fifoPoptInt32TupleConditional (cmdLine,options,false,minus)	// check syntax
		&& fifoPoptInt32TupleConditional (cmdLine,options,true,minus);	// do for real
}

//SLICE
bool fifoPoptString(Fifo *fifo, const FifoPoptString *options, bool *minus) {
	for (int o=0; options[o].shortOption!=0 || options[o].longOption!=0; ++o) {
		const FifoPoptString *option = &options [o];
		Fifo clone = *fifo;		// we allow the same option multiple times.
		if (fifoPoptShortOrLongParam (&clone, option->shortOption, option->longOption, *minus)) {
			Fifo fifoWord;
			if (fifoParseStringZ (&clone,&fifoWord)) {
				* option->value = fifoWord;
				fifoCopyReadPosition (fifo,&clone);
				*minus = false;
				return true;
			}
			else return false;
		}
	}
	return false;
}

//SLICE
bool fifoPoptNonOptionAccumulate(Fifo *cmdLine, Fifo *listOfNonOptions, bool *minus) {
	if (!*minus	// non-options cannot be prefixed by a -
	&& fifoCanRead(cmdLine) && fifoLookAhead(cmdLine)!='-') {
		Fifo fifoString;
		if (fifoParseStringZ(cmdLine,&fifoString)) {
			return fifoAppend(listOfNonOptions,&fifoString) && fifoPrintChar(listOfNonOptions,0);
		}
		else return false;
	}
	else return false;
}

//SLICE
bool fifoPoptSymbol(Fifo *fifo, const FifoPoptSymbol *options, bool *minus) {
	for (int o=0; options[o].shortOption!=0 || options[o].longOption!=0; ++o) {
		const FifoPoptSymbol *option = &options [o];
		Fifo clone = *fifo;		// we allow the same option multiple times.
		if (fifoPoptShortOrLongParam (&clone, option->shortOption, option->longOption, *minus)) {
			for (int s=0; option->symbols[s]!=0; s++) {
				Fifo clone2 = clone;
				if (fifoParseExactString(&clone2,option->symbols[s])
				&& fifoParseZ(&clone2)) {
					* option->value = s;
					fifoCopyReadPosition(fifo,&clone2);
					*minus = false;
					return true;
				}
				// else no symbol match
			}
			// do not return here, because we allow overlapping options.
		}
	}
	return false;
}

//SLICE
bool fifoPoptInt32List(Fifoq *fifo, const FifoPoptInt32List *options, bool *minus) {
	for (int o=0; options[o].shortOption!=0 || options[o].longOption!=0; ++o) {
		Fifo clone = *fifo;
		const FifoPoptInt32List *option = options + o;
		if (fifoPoptShortOrLongParam (&clone,option->shortOption,option->longOption,*minus)) {
			Int32 bufferList[ int32ListSize(option->value) ];
			Int32List list = { bufferList, sizeof bufferList, };

			if (0 < fifoParseInt32List (&clone,&list,option->parseInt,option->separator)
			&& fifoParseZ (&clone)) {
				// copy lists
				int32ListCopy (option->value,&list);
				fifoCopyReadPosition (fifo,&clone);
				*minus = false;		// - consumed
				return true;
			}
			else return false;	// invalid params
		}
	}
	return false;
}

//SLICE
bool fifoPoptInt32PairList(Fifoq *fifo, const FifoPoptInt32PairList *options, bool *minus) {
	for (int o=0; options[o].shortOption!=0 || options[o].longOption!=0; ++o) {
		Fifo clone = *fifo;
		const FifoPoptInt32PairList *option = options + o;
		if (fifoPoptShortOrLongParam (&clone,option->shortOption,option->longOption,*minus)) {
			Int32Pair bufferList[ int32PairListSize(option->value) ];
			Int32PairList list = { bufferList, sizeof bufferList, };

			const FifoParseInt32Pair parse = {
				.parseFst = option->parseIntFst,
				.separator = option->separatorPair,
				.parseSnd = option->parseIntSnd
			};

			if (0 < fifoParseInt32PairListDynamic (&clone,&list,&parse, option->separatorList)
			&& fifoParseZ (&clone)) {
				int32PairListCopy (option->value, &list);
				fifoCopyReadPosition (fifo,&clone);
				*minus = false;
				return true;
			}
			else return false;
		}
	}
	return false;
}

//SLICE
bool fifoPoptInt32PairListWithInt32Parameter (Fifoq *fifo, const FifoPoptInt32PairListWithInt32Parameter *options, bool *minus) {
	for (int o=0; options[o].shortOption!=0 || options[o].longOption!=0; ++o) {
		Fifo clone = *fifo;
		const FifoPoptInt32PairListWithInt32Parameter *option = options + o;
		if (fifoPoptShortOrLongParam (&clone,option->shortOption,option->longOption,*minus)) {
			Int32Pair bufferList[ int32PairListSize(option->value) ];
			Int32PairList list = { bufferList, sizeof bufferList, };

			const FifoParseInt32PairWithInt32Parameter parser = {
				.parse = option->parsePair,
				.parameter = option->parameter,
			};
			if (0 < fifoParseInt32PairListWithInt32Parameter (&clone,&list,&parser,option->separator)
			&& fifoParseZ (&clone)) {
				// copy lists
				int32PairListCopy (option->value,&list);
				fifoCopyReadPosition (fifo,&clone);
				*minus = false;		// - consumed
				return true;
			}
			else return false;	// invalid params
		}
	}
	return false;
}

