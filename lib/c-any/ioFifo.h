#ifndef __ioFifo_h
#define __ioFifo_h

/** @file
 * @brief an interface designed for message-oriented IO with the messages build and parsed using Fifo.
 */

/** An interface that allows message IO using Fifo as message formatter/parser.
 */
typedef struct {
	bool 	(*canRead)(void);	///< checks, if an incoming packet is available
	bool	(*canWrite)(void);	///< checks, if a buffer for writing an outgoing packet is available.
	Fifo*	(*readBegin)(void);	///< Gets a Fifo for reading an incoming packet.
	void	(*readEnd)(void);	///< Disposes off the Fifo used for reading.
	Fifo*	(*writeBegin)(void);	///< Gets a Fifo for writing an outgoing packet.
	void	(*writeEnd)(void);	///< Disposes off the Fifo used for writing.
} IoFifo;

/** Generic result type for message handlers.
 */
typedef enum {
	MESSAGE_UNHANDLED,	///< message not handled - possible input for other handler.
	MESSAGE_DONE,		///< message handled successfully.
	MESSAGE_INVALID,	///< message handled with error - no recovery.
	MESSAGE_POSTPONE,	///< message not handled because of lacking resource - try again, later.
} MessageResult;

#endif
