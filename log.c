/*
 * log.c
 *
 *  Created on: May 17, 2011
 *      Author: eric
 */

#include "log.h"

void log_debug(char * fmt, ...)
{
#ifdef DEBUG
	va_list list_p;
	va_start(list_p,fmt);
	vprintf(fmt,list_p);
	va_end(list_p);
#endif
}

void log_info(char * fmt, ...)
{
#ifdef INFO
	va_list list_p;
	va_start(list_p,fmt);
	vprintf(fmt,list_p);
	va_end(list_p);
#endif
}
