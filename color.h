/*
 * Color definitions and color table
 *
 * $Revision: 1.4 $ $Date: 1990-01-23 09:05:04 $ $Author: burghart $
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
# define C_DATA1	(6 + Colorbase)
# define C_DATA2	(7 + Colorbase)
# define C_DATA3	(8 + Colorbase)
# define C_DATA4	(9 + Colorbase)
# define C_DATA5	(10 + Colorbase)
# define C_DATA6	(11 + Colorbase)
# define C_DATA7	(12 + Colorbase)
# define C_DATA8	(13 + Colorbase)
# define C_DATA9	(14 + Colorbase)

/*
 * Color count (change this if another color is added above)
 */
# define NCOLORS	15

/*
 * The base for our color allocation
 */
var int		Colorbase;
