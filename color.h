/*
 * Color definitions and color table
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
 * $Id: color.h,v 1.5 1991-10-30 21:34:35 burghart Exp $
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
