/*
  roundRobin.h 
  Copyright 2015 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef c_any_roundRobin_h
#define c_any_roundRobin_h

/** @file
 * @brief Simple structure and functions to periodically run tasks in a round robin manner.
 *
 * This module depends on a hardware timer like usTimer(). The timing unit is suggested to be 1us, but that's not a
 * strict demand. Take care, that nTasks is much greater than cycleUs for accuracy.
 */

/** RoundRobin is a periodic timer, that times n tasks. Tasks have numbers 1,2,3....
 * 0 indicates 'no task'. RoundRobin builds upon a hardware timer, like usTimer() .
 */
typedef struct {
	Int32	subCycleUs;
	Int32	nTasks;
	Int32	tUs;		///< expired periodically
	Int32	task;
} RoundRobin;

/** Calculates the parameters to efficiently run a round robin timer.
 * @param robin the state variable.
 * @param cycleUs the cycle time for one loop through all tasks
 * @param nTasks the number of tasks. Must be at least 1.
 * @param tUs the current time.
 * @return true if the cycle time will be exact, false if a rounding error will accumulate.
 */
inline static bool roundRobinInit (RoundRobin *robin, Int32 cycleUs, int nTasks, Int32 tUs) {
	robin->subCycleUs = cycleUs / nTasks;
	robin->nTasks = nTasks;
	robin->tUs = tUs;
	robin->task = 0;

	return robin->subCycleUs*nTasks == cycleUs;		// check for rounding error
}

/** Checks for the next task to be started.
 * @param robin the state variable.
 * @param tUs the current time.
 * @return 0 if no task is to be started or a value >0 that indicates the task to be started.
 */
inline static int roundRobinNextTask (RoundRobin *robin, Int32 tUs) {
	if (tUs - robin->tUs >=0) {
		robin->tUs += robin->subCycleUs;
		const Int32 task = robin->task;
		robin->task = task+1>=robin->nTasks ? 0 : task+1;
		return task;
	}
	else return 0;	// no task ready to run
}

#endif

