/*
 * Color definitions and color table
 *
 * $Log: not supported by cvs2svn $
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
# define C_LGRAY	2
# define C_DGRAY	3
# define C_RED		4
# define C_GREEN	5
# define C_BLUE		6
# define C_MAGENTA	7
# define C_YELLOW	8
# define C_CYAN		9
# define C_WET		10
# define C_DRY		11
/*
 * Color count (change this if another color is added above)
 */
# define NCOLORS	12

/*
 * The base for our color allocation and the color values
 */
var int		Colorbase;

var float 	Red[NCOLORS];
var float 	Green[NCOLORS];
var float	Blue[NCOLORS];
