/*
  timeAndDate.h 
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#ifndef __timeAndDate_h
#define __timeAndDate_h

/** @file
 * @brief Gregorian calendar.
 */

#include <stdbool.h>

/** Represents human-readable date.
 */
typedef struct {
	char	day;
	char	month;
	short	year;
	char	dayOfWeek;
} Date;

enum {
	NODAY,
	MONDAY,
	TUESDAY,
	WEDNESDAY,
	THURSDAY,
	FRIDAY,
	SATURDAY,
	SUNDAY,
};

/** Represents human-readable time of day.
 */
typedef struct {
	char	second;
	char	minute;
	char	hour;
	char	timeZone;		///< 1=MESZ (summer daylight saving), 2=MEZ. 
} Time;

enum {
	TIMEZONE_NONE,
	TIMEZONE_MESZ,
	TIMEZONE_MEZ,
};

typedef struct {
	Time	time;
	Date	date;
} TimeAndDate;

/** Calculates the number of days of a given year according to the Gregorian calendar. 01.01.2000 was a Saturday as was
 *  01.01.0000.
 * @param year the year including leading digits (2009 instead of 09). However, since the Gregorian calendar is
 *   400-year periodic, the results for 2-digit and 4-digit years are the same until year 2099. 
 * @return 365 or 366 
 */
int calendarDaysOfYear(int year);

/** Calculates the number of days of a given month.
 * @param month the month 1..12 .
 * @param year the year in the valid range of the Gregorian calendar.
 * @return number of days 28..31 .
 */
int calendarDaysOfMonth(int month, int year);

/** Calculates the day index within a year.
 * @param date the date of a day
 * @return the zero-based index of the day given. 0 is the first day, 2 the second ...
 */
int calendarDayOfYear(const Date *date);

extern const Date dateBase;	///< Arbitrary zero-point for linear and absolute times and dates.

/** Calculates the absolute days since dateBase. This is an expensive function.
 * @param date a date
 * @return the zero-based index of the day given.
 */
int calendarDayAbsolute(const Date *date);

/** Calculates the day of week of a given day using the Gregorian calendar. This is an expensive function.
 * @param date a numeric date.
 * @return the day of week 1..7 (Mo..Su).
 */
char calendarDayOfWeek(const Date *date);

/** Calculates the day of week of a given day using the Gregorian calendar if the day of week information is missing
 * from the given date. This is an expensive function.
 * @param date a numeric date.
 * @return true if a lookup of the calendar was neccessary, false if day of week was already set in date.
 */
bool calendarDayOfWeekAdjust(Date *date);

/** Translates a linear day number into a human-readable date. This is an expensive function.
 * @param date the destination buffer. All fields will be set.
 * @param day linear time as zero-based offset (of days) from dateBase.
 */
void calendarDateFromDayAbsolute(Date *date, int day);


/** Translates a time of day into linear time.
 * @param time the time of day.
 * @return the number of seconds elapsed since 00:00.00 (0..86399).
 */
int timeToSeconds(const Time* time);

/** Converts date and time into a linear value (seconds). Due to the limited range of int the valid range of dates is
 * years 1932..2067 (inclusively). This is an expensive function.
 * @param timeAndDate the exact time.
 * @return the seconds since 01.01.2000 
 */
int timeAndDateToSeconds(const TimeAndDate* timeAndDate);

/** Converts linear time since dateBase into days and time of day. This is an expensive function.
 * @param time the destination buffer.
 * @param seconds the signed offset from dateBase.
 * @return the number of days since dateBase.
 */
int timeFromSeconds(Time *time, int seconds);

/** Converts linear time since dateBase int date and time. This is an expensive function.
 * @param timeAndDate the destination buffer.
 * @param seconds the signed offset from dateBase.
 * @return the number of days since dateBase.
 */
void timeAndDateFromSeconds(TimeAndDate* timeAndDate, int seconds);

#include <fifoPrint.h>

/** Writes a time, format hh:mm.ss .
 * @param fifo the output FIFO.
 * @param time the time structure
 * @param showTz true to show timezone info
 * @return true if successfull written, false otherwise.
 */
bool fifoPrintTime(Fifo *fifo, const Time *time, bool showTz);

/** Writes a date, format [day ]dd.mm.yyyy
 * @param fifo the output FIFO.
 * @param date the date structure
 * @param showDayOfWeek true to show German day of week.
 * @return true if successfull written, false otherwise.
 */
bool fifoPrintDate(Fifo *fifo, const Date *date, bool showDayOfWeek);

#include <fifoParse.h>

/** Parses a time string. The following formats are supported: hh - hour, hh=0..23;
 * hh:mm - hour and minute, mm=0..59; hh:mm.ss - hour, minute and second, ss=0..59 .
 * Additionally, MEZ or MESZ may be given.
 * Unspecified fields are set to zero.
 * @param fifo the input.
 * @param time the destination.
 * @return true if successfully parsed, false otherwise (and fifo restored).
 */
bool fifoParseTime(Fifo *fifo, Time *time);

/** Parses a date string of the (German) style [dayOfWeek ]dd.mm.yyyy .
 * @param fifo the input.
 * @param date the destination.
 * @return true if successfully parsed, false otherwise (and fifo restored).
 */
bool fifoParseDate(Fifo *fifo, Date *date);


#endif

