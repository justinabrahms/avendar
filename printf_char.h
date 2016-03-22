/* source: EOD, by John Booth <???> */
/* stick this in in comm.c somewhere */
/* Remember to include <stdarg.h> */

void printf_to_char (CHAR_DATA *ch, char *fmt, ...)
{
	char buf [MAX_STRING_LENGTH];
	va_list args;
	va_start (args, fmt);
	vsprintf (buf, fmt, args);
	va_end (args);
	
	send_to_char (buf, ch);
}

