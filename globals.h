/*
 * Global parameters
 *
 * $Revision: 1.3 $ $Date: 1989-08-02 14:38:41 $ $Author: burghart $ 
 */
# include <ui.h>
# include <graphics.h>

# ifndef TRUE
#	define TRUE	1
#	define FALSE	0
# endif

# define BADVAL	-999.
# define BUFLEN	1024

# define T_K	273.15

/*
 * Border width on plot
 */
# define BORDER	0.2

# ifndef var
#	ifdef VMS
#		define var	globalref
#	else
#		define var	extern
#	endif
# endif

/*
 * Our current workstation + device and type
 */
var ws	Wkstn;
var char	Out_dev[40], Dev_type[40];

/*
 * Have we had a keyboard interrupt?
 */
var int	Interrupt;

/*
 * Wind scale value
 */
var float	W_scale;

/*
 * MIN macro
 */
# define MIN(x,y)	((x) < (y) ? (x) : (y))
