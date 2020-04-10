#include <fifo.h>

Fifo emptyFifo = { 0, 0, };

Fifo * volatile fi = &emptyFifo;
Fifo * volatile fo = &emptyFifo;

