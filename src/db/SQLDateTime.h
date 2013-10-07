/*
 * Copyright (C) Tildeslash Ltd. All rights reserved.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 *
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.
 */


#ifndef SQLDATETIME_INCLUDED
#define SQLDATETIME_INCLUDED


/**
 * This interface contains a set of simple structures representing SQL
 * temporal data types.
 *
 * @see ResultSet.h
 * @file
 */


/**
 * A Date object represent a SQL Date value.
 */
typedef struct sqldate_s {
        /** Year, the range is database dependent. For instance, MySQL's epoch is year 1000 */
        int year;
        /** Month of the year (0 - 11) */
        int month;
        /** Day of the month (1-31) */
        int day;
} sqldate_t;


/**
 * A Time object represent a SQL Time value.
 */
typedef struct sqltime_s {
        /** Hour of day (0-23) */
        int hour;
        /** Minutes of hour (0-59) */
        int min;
        /** Seconds of minute (0-60) */
        int sec;
        /** Microseconds of seconds up to 6 digits. Availability is database dependent */
        int microseconds;
} sqltime_t;


/**
 * A DateTime object represent a SQL DateTime value.
 */
typedef struct sqldatetime_s {
        /** The Date part */
        sqldate_t date;
        /** The Time part */
        sqltime_t time;
} sqldatetime_t;


#endif
