/*
 * Definitions for the flags
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
 * $Id: flags.h,v 1.4 1991-10-30 21:32:09 burghart Exp $
 */

# ifndef var
#	ifdef VMS
#		define var globalref
#	else
#		define var extern
#	endif
# endif

var char	Flg_mli;	/* Modified lifted index	*/
var char	Flg_winds;	/* Winds in skew-t plot?	*/
var char	Flg_lift;	/* Show the lifted parcel?	*/
var char	Flg_theta_w;	/* Theta_w on skew-t plots?	*/
var char	Flg_vt;		/* Virtual temperature on skew-t plots?	*/
