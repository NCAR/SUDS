/*
 * Color definitions and color table
 *
 * $Revision: 1.3 $ $Date: 1989-08-23 10:46:31 $ $Author: burghart $
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
# define C_BLACK	(0 + Colorbase)
# define C_WHITE	(1 + Colorbase)
# define C_BG1		(2 + Colorbase)
# define C_BG2		(3 + Colorbase)
# define C_BG3		(4 + Colorbase)
# define C_BG4		(5 + Colorbase)
# define C_TRACE1	(6 + Colorbase)
# define C_TRACE2	(7 + Colorbase)
# define C_TRACE3	(8 + Colorbase)
# define C_TRACE4	(9 + Colorbase)
# define C_TRACE5	(10 + Colorbase)
# define C_TRACE6	(11 + Colorbase)

/*
 * Color count (change this if another color is added above)
 */
# define NCOLORS	12

/*
 * The base for our color allocation
 */
var int		Colorbase;
