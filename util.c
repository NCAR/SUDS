/*
 * Utility routines
 *
 * $Log: not supported by cvs2svn $
 */
# ifdef VMS
#	include <stdlib.h>
# endif
# include <ctype.h>

void
strcpyUC (dest, src)
char	*dest, *src;
/*
 * Copy src to dest, converting to upper case
 */
{
	while (*src)
		*dest++ = islower (*src) ? *src++ - 'a' + 'A' : *src++;

	*dest = 0;
}
