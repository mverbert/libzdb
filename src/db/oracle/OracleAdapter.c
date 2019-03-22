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

#include "Config.h"

#include <stdlib.h>

#include "Thread.h"
#include "system/Time.h"
#include "OracleAdapter.h"

/* Error handling: Oracle requires a buffer to store
 error message, to keep error handling thread safe
 TSD is used
 */

/* Key for the thread-specific buffer */
static ThreadData_T error_msg_key;

/* Once-only initialisation of the key */
static Once_T error_msg_key_once = PTHREAD_ONCE_INIT;


/* Return the thread-specific buffer */
static char * get_err_buffer(void) {
        char * err_buffer = (char *) ThreadData_get(error_msg_key);
        if (err_buffer == NULL) {
                err_buffer = malloc(STRLEN);
                ThreadData_set(error_msg_key, err_buffer);
        }

        return err_buffer;
}

/* Allocate the key */
static void error_msg_key_alloc() {
        ThreadData_create(error_msg_key, free);
}


// MARK:- API


/* This is a general error function also used in OracleResultSet */
const char *OraclePreparedStatement_getLastError(int err, OCIError *errhp) {
        sb4 errcode;
        Thread_once(error_msg_key_once, error_msg_key_alloc);
        char* erb = get_err_buffer();
        assert(erb);
        assert(errhp);
        switch (err)
        {
                case OCI_SUCCESS:
                        return "";
                case OCI_SUCCESS_WITH_INFO:
                        return "Info - OCI_SUCCESS_WITH_INFO";
                        break;
                case OCI_NEED_DATA:
                        return "Error - OCI_NEED_DATA";
                        break;
                case OCI_NO_DATA:
                        return "Error - OCI_NODATA";
                        break;
                case OCI_ERROR:
                        OCIErrorGet(errhp, 1, NULL, &errcode, erb, STRLEN, OCI_HTYPE_ERROR);
                        return erb;
                        break;
                case OCI_INVALID_HANDLE:
                        return "Error - OCI_INVALID_HANDLE";
                        break;
                case OCI_STILL_EXECUTING:
                        return "Error - OCI_STILL_EXECUTE";
                        break;
                case OCI_CONTINUE:
                        return "Error - OCI_CONTINUE";
                        break;
                default:
                        break;
        }
        return erb;
}

