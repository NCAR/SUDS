/*
 * Global parameters
 */
/*
 *		Copyright (C) 1988-91 by UCAR
 *	University Corporation for Atmospheric Research
 *		   All rights reserved
 *
 * No part of this work covered by the copyrights herein may be reproduced
 * or used in any form or by any means -- graphic, electronic, or mechanical,
 * including photocopying, recording, taping, or information storage and
 * retrieval systems -- without permission of the copyright owner.
 * 
 * This software and any accompanying written materials are provided "as is"
 * without warranty of any kind.  UCAR expressly disclaims all warranties of
 * any kind, either express or implied, including but not limited to the
 * implied warranties of merchantibility and fitness for a particular purpose.
 * UCAR does not indemnify any infringement of copyright, patent, or trademark
 * through use or modification of this software.  UCAR does not provide 
 * maintenance or updates for its software.
 *
 * $Id: globals.h,v 1.9 1994-06-24 21:51:00 burghart Exp $
 */

# include <ui.h>
# include <graphics.h>

# ifndef TRUE
#	define TRUE	1
#	define FALSE	0
# endif


/*
 * A macro to make function prototypes a little easier across both STDC and
 * non-STDC implementations.
 */
# ifdef __STDC__
#  define FP(stuff) stuff
# else
#  define FP(stuff) ()
# endif


# define BADVAL	-999.
# define BUFLEN	2048

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
 * Wind scale value and wind barb resolution
 */
var float	W_scale;
var float	Wb_res;

/* 
 * Wind barb plotting step value (plot each Nth wind sample)
 */
var int		Mark_inc;

/*
 * Pressure value to use for forecast analyses
 */
var float	Forecast_pres;

/*
 * Depth to use in calculating shear
 */
var float	Shear_depth;

/*
 * MIN macro
 */
# define MIN(x,y)	((x) < (y) ? (x) : (y))
