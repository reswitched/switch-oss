/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
/* 
 * The following code replaces the use of fgets and other file stream
 * APIs from the standard C library with equivalents using NSPR's file
 * I/O APIs.  This is to better support portability by way of NSPR.
 */
#include "fileutils.h"


char *PORT_fgets(char *str, int size, PRFileDesc *file) {
    char                        *ret = str;
    PRInt32                     i;
    PRInt32                     bytesRead;

    if ((str == NULL) || (size <= 0) || (file == NULL)) {
        ret = NULL;
        goto _errExit;
    }

    /*  As long as no error (including EOF) occurs, keep reading up to
        a newline or one less than the total size.  */
    for (i = 0; i < (size - 1); i++) {
        bytesRead = PR_Read(file, (void *)(str + i), 1);
        if ((bytesRead == -1) || (bytesRead == 0)) {
            ret = NULL;
            break;
        }

        if ('\n' == *(str + i)) {
            /*  EOL encounterd, bail out  */
            break;
        }
    }

    /*  If no error was encountered, NULL terminate the string.  */
    if (ret != NULL) {
        *(str + (i + 1)) = '\0';
    }

_errExit:
    return ret;
}


int PORT_fputs(char *str, PRFileDesc *file) {
    int                         ret = -1;
    int                         len = (int)PL_strlen(str);
    PRInt32                     status;

    if ((str == NULL) || (file == NULL)) {
        goto _errExit;
    }

    status = PR_Write(file, str, len);
    if (status >= 0) {
        ret = (int)status;
    }

_errExit:
    return ret;
}

int PORT_feof(PRFileDesc *file) {
    PRInt64 availableToRead = PR_Available64(file);

    /* PR_Available64() can fail and return '-1', in this scenario
       PORT_feof() will return 'true' for EOF status. */
    return (availableToRead <= 0);
}
