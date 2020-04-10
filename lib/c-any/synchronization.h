#ifndef synchronization_h
#define synchronization_h

#include <stdbool.h>
#include <integers.h>

/** @file
 * @brief Basic synchronization primitives interface.
 *
 * The primitives must be implemented on the target platform - this header is an interface definition only.
 */

/** A SynchronizationToken manages responsibility between different parties. Only one party can be active at a time.
 * The token can be passed over to another party, but only from the current active party.
 * The 2-element structure should provide enough flexibility for implementing the token on all architectures.
 */
typedef struct {
	volatile Int8	mutex;		///< a mutex to lock a more complex code region, if needed
	volatile Int8	party;		///< the token value.
} SynchronizationToken;

/** Pass over the token to a different party atomically.
 * @param token the token object
 * @param from the party that tries to pass over the token
 * @param to the destination party.
 * @return true if token was equal to from and thus changed to to, false otherwise and token unchanged.
 */
bool synchronizationTokenPassTo(SynchronizationToken *token, int from, int to);


#endif
