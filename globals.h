/*
 * Global parameters
 *
 * $Log: not supported by cvs2svn $
 */
# include <ui.h>
# include <graphics.h>

# define TRUE	1
# define FALSE	0

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
