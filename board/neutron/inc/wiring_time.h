/**
 ******************************************************************************
 * @file     : wiring_time.h
 * @author   : robin
 * @version	 : V1.0.0
 * @date     : 6-December-2014
 * @brief    : 
 ******************************************************************************
  Copyright (c) 2013-2014 IntoRobot Team.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */
#ifndef   WIRING_TIME_H_
#define   WIRING_TIME_H_

#include "wiring.h"
#include <time.h>
#include "wiring_string.h"
#include "wiring_time_hal.h"


class TimeClass
{
    public:
        // Arduino time and date functions
        static int     hour();            			// current hour
        static int     hour(time_t t);				// the hour for the given time
        static int     hourFormat12();    			// current hour in 12 hour format
        static int     hourFormat12(time_t t);		// the hour for the given time in 12 hour format
        static uint8_t isAM();            			// returns true if time now is AM
        static uint8_t isAM(time_t t);    			// returns true the given time is AM
        static uint8_t isPM();            			// returns true if time now is PM
        static uint8_t isPM(time_t t);    			// returns true the given time is PM
        static int     minute();          			// current minute
        static int     minute(time_t t);  			// the minute for the given time
        static int     second();          			// current second
        static int     second(time_t t);  			// the second for the given time
        static int     day();             			// current day
        static int     day(time_t t);     			// the day for the given time
        static int     weekday();         			// the current weekday
        static int     weekday(time_t t); 			// the weekday for the given time
        static int     month();           			// current month
        static int     month(time_t t);   			// the month for the given time
        static int     year();            			// current four digit year
        static int     year(time_t t);    			// the year for the given time
        static time_t  now();              			// return the current time as seconds since Jan 1 1970
        static void    zone(float GMT_Offset);		// set the time zone (+/-) offset from GMT
        static void    setTime(time_t t);			// set the given time as unix/rtc time
        static String  timeStr();			// returns string representation for the current time
        static String  timeStr(time_t t);			// returns string representation for the given time
};

extern TimeClass Time;	//eg. usage: Time.day();

#endif /*WIRING_TIME_H_*/
