/*
 * Color definitions and color table
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  89/03/16  15:12:13  burghart
 * Initial revision
 * 
 */
# ifndef var
#	ifdef VMS
#		define var globalref
#	else
#		define var extern
#	endif
# endif

/*
 * Color identifiers
 */
# define C_BLACK	0
# define C_WHITE	1
# define C_BG1		2
# define C_BG2		3
# define C_BG3		4
# define C_BG4		5
# define C_TRACE1	6
# define C_TRACE2	7
# define C_TRACE3	8
# define C_TRACE4	9
# define C_TRACE5	10
# define C_TRACE6	11

/*
 * Color count (change this if another color is added above)
 */
# define NCOLORS	12

/*
 * The base for our color allocation
 */
var int		Colorbase;
