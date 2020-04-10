#ifndef cyclicIterator_h
#define cyclicIterator_h

typedef struct {
	int wrap;	///< wrap to zero point
	int total;	///< total number of indices
	int pos;	///< current position.
	int done;	///< number of elements already iterated over
} CyclicIterator;

inline static bool cyclicIteratorHasNext(CyclicIterator const *i) {
	return i->done < i->total;
}

inline static int  cyclicIteratorNext(CyclicIterator *i) {
	const int index = i->pos;
	i->pos++; if (i->pos ==  i->wrap) i->pos = 0;
	i->done++;
	return index;
}

#endif

