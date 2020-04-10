#ifndef concurrentAccu_h
#define concurrentAccu_h

/** Limited Thread-safe version of a numeric accumulated value.
 * A numeric value must be transferred and summed up across threads (consumer and producer).
 */
typedef struct {
	volatile int	add;
	volatile int	sub;
} ConcurrentAccu;

/** Sends an additional value to the consumer.
 * @param ca the accumulator object.
 * @param value the value to add.
 */
inline static void concurrentAccuAdd(ConcurrentAccu *ca, int value) {
	ca->add += value;
}

/** Retrieves the accumulated values of the producer and zeros the accu.
 * @param ca the accumulator object.
 * @return the sum of the producer's values.
 */
inline static int concurrentAccuSub(ConcurrentAccu *ca) {
	const int add = ca->add;
	const int value = add - ca->sub;
	ca->sub = add;
	return value;
}

/** Retrieves the accumulated values of the producer without zeroing the value.
 * @param ca the accumulator object.
 * @return the sum of the producer's values.
 */
inline static int concurrentAccuPeek(ConcurrentAccu *ca) {
	return ca->add - ca->sub;
}

#endif
