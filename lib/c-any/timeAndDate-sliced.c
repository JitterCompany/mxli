//HEADER
/*
  timeAndDate-sliced.c 
  Copyright 2011 Marc Prager
 
  This file is part of the c-any library.
  c-any is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 
  c-any is published in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty
  of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License along with c-any.
  If not see <http://www.gnu.org/licenses/>
 */

#include <timeAndDate.h>
extern const char *const dayNames[8];
extern const char *const timeZones[3];	///< normal and daylight saving

//SLICE
const char *const dayNames[8] = { "?? ", "Mo ", "Di ", "Mi ", "Do ", "Fr ", "Sa ", "So " };
const char *const timeZones[3] = { " ??", " MESZ", " MEZ" };

//SLICE
bool fifoPrintTime(Fifo *fifo, const Time* time, bool showTz) {
	return
		fifoPrintUDec(fifo,time->hour,2,2)
		&& fifoPrintChar(fifo,':')
		&& fifoPrintUDec(fifo,time->minute,2,2)
		&& fifoPrintChar(fifo,'.')
		&& fifoPrintUDec(fifo,time->second,2,2)
		&& (!showTz || fifoPrintString(fifo,timeZones[time->timeZone&3]));
}

//SLICE
bool fifoPrintDate(Fifo *fifo, const Date* date, bool showDayOfWeek) {
	return
		(!showDayOfWeek || fifoPrintString(fifo,dayNames[date->dayOfWeek & 7]))
		&& fifoPrintUDec(fifo,date->day,2,2)
		&& fifoPrintChar(fifo,'.')
		&& fifoPrintUDec(fifo,date->month,2,2)
		&& fifoPrintChar(fifo,'.')
		&& fifoPrintSDec(fifo,date->year,4,5,false);
}

//SLICE
bool fifoParseTime(Fifo *fifo, Time *time) {
	Fifo clone = *fifo;
	int h,m;

	if (fifoParseIntLimited(&clone,&h,0,23)
	&& fifoParseExactChar(&clone,':')
	&& fifoParseIntLimited(&clone,&m,0,59)) {	// hh:mm at least
		time->hour = h;
		time->minute = m;
		fifoCopyReadPosition(fifo,&clone);	// commit this read - everything else is optional.

		int s;
		if (fifoParseExactChar(&clone,'.')
		&& fifoParseIntLimited(&clone,&s,0,59)) {	// hh:mm.ss
			time->second = s;
			fifoCopyReadPosition(fifo,&clone);
		}
		else {
			time->second = 0;
			clone = *fifo;		// no seconds, so restore
		}

		for (int tz=1; tz<=2; ++tz) if (fifoParseExactString(&clone,timeZones[tz])) time->timeZone = (char)tz;

		fifoCopyReadPosition(fifo,&clone);	// commit the read.
		return true;
	}
	else return false;
}

//SLICE
bool fifoParseDate(Fifo* fifo, Date* date) {
	Fifo clone = *fifo;
	int y,m,d;

	date->dayOfWeek = 0;
	for (int dow=1; dow<=7; ++dow) if (fifoParseExactString(&clone,dayNames[dow])) date->dayOfWeek = dow;

	if (fifoParseIntLimited(&clone,&d,1,31)
	&& fifoParseExactChar(&clone,'.')
	&& fifoParseIntLimited(&clone,&m,1,12)
	&& fifoParseExactChar(&clone,'.')
	&& fifoParseInt(&clone,&y)) {
		date->year = y;
		date->month = m;
		date->day = d;
		fifoCopyReadPosition(fifo,&clone);
		return true;
	}
	else return false;
}

//SLICE
int calendarDaysOfYear(int year) {
	return 365 + (year%4==0 && (year%100!=0 || year%400==0) ? 1 : 0);
}

//SLICE
int calendarDaysOfMonth(int month, int year) {
	static unsigned char dom[] = { 0, 31,28,31,30,31,30,31,31,30,31,30,31 };

	if (1<=month && month<=12) {
		if (month!=2 || calendarDaysOfYear(year)!=366) return dom[month];
		else return 29;
	}
	else return 0;
}

//SLICE
int calendarDayOfYear(const Date *date) {
	int day = 0;
	for (int m=1; m<date->month; ++m) day += calendarDaysOfMonth(m,date->year);
	return day + date->day - 1;
}

//SLICE
const Date dateBase = { 1,1,2000,SATURDAY };

//SLICE
int calendarDayAbsolute(const Date *date) {
	int day = 0;
	for (int y=dateBase.year; y<date->year; ++y) day += calendarDaysOfYear(y);
	for (int y=dateBase.year; y>date->year; --y) day -= calendarDaysOfYear(y-1);
	return day + calendarDayOfYear(date);
}

//SLICE
char calendarDayOfWeek(const Date *date) {
	int dayDelta = calendarDayAbsolute(date) - calendarDayAbsolute(&dateBase);

	char dayOfWeek = dateBase.dayOfWeek + (dayDelta%7);
	if (dayOfWeek>7) dayOfWeek -= 7;
	if (dayOfWeek<1) dayOfWeek += 7;

	return dayOfWeek;
}

//SLICE
bool calendarDayOfWeekAdjust(Date *date) {
	if (date->dayOfWeek==NODAY) {
		date->dayOfWeek = calendarDayOfWeek(date);
		return true;
	}
	else return false;
}

//SLICE
void calendarDateFromDayAbsolute(Date *date, int day) {
	int year = dateBase.year;

	for ( ; calendarDaysOfYear(year)<=day ; ++year) day -=calendarDaysOfYear(year);
	for ( ; day<0; --year) day +=calendarDaysOfYear(year-1);

	int month=1;
	for ( ; calendarDaysOfMonth(month,year)<=day; ++month) day -=calendarDaysOfMonth(month,year);
	
	date->day = day+1;
	date->month = month;
	date->year = year;

	date->dayOfWeek = NODAY;
	calendarDayOfWeekAdjust(date);
}

//SLICE
int timeToSeconds(const Time* time) {
	return	time->hour*3600 +
		time->minute*60 +
		time->second;
}

//SLICE
int timeAndDateToSeconds(const TimeAndDate* timeAndDate) {
 	return
		86400*calendarDayAbsolute(&timeAndDate->date) +
		timeToSeconds(&timeAndDate->time);
}

//SLICE
int timeFromSeconds(Time *time, int seconds) {
	int days = 0;
	while (seconds>=86400) {
		days++;
		seconds -= 86400;
	}
	while (seconds<0) {
		days--;
		seconds += 86400;
	}

	time->hour = seconds / 3600;
	seconds -= time->hour*3600;
	time->minute = seconds / 60;
	seconds -= time->minute*60;
	time->second = seconds;
	return days;
}

//SLICE
void timeAndDateFromSeconds(TimeAndDate* timeAndDate,int seconds) {
	const int days = timeFromSeconds(&timeAndDate->time,seconds);
	calendarDateFromDayAbsolute(&timeAndDate->date,days);
}

