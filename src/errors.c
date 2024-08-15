/*
 * File name: 
 * Description:
 *
 * Notes:
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

void
fatal_error(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    vprintf(format, args);
    printf("\n");

    va_end(args);
    exit(1);
}

/* End of file: errors.c */
