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
 */

#ifndef ORACLEWATCHDOG_INCLUDED
#define ORACLEWATCHDOG_INCLUDED

#define WATCHDOG(FUNCNAME, TYPENAME)      \
static void *FUNCNAME(void *args) {       \
    TYPENAME S = args;                    \
    while (S->svc) {                      \
        if (S->running) {                 \
            if (S->countdown <= 0) {      \
                OCIBreak(S->svc, S->err); \
                S->running = false;       \
            } else {                      \
                S->countdown -= 10;       \
            }                             \
        }                                 \
        usleep(10000);                    \
    }                                     \
    return NULL;                          \
}

#endif
