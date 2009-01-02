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
 * $Id: flags.h,v 1.8 2004-06-02 21:57:21 burghart Exp $
 */

# ifndef var
#	ifdef VMS
#		define var globalref
#	else
#		define var extern
#	endif
# endif

# include <ui_param.h>	/* for bool */

var bool	Flg_mli;	/* Modified lifted index	*/
var bool	Flg_winds;	/* Winds in skew-t plot?	*/
var bool	Flg_lift;	/* Show the lifted parcel?	*/
var bool	Flg_theta_w;	/* Theta_w on skew-t plots?	*/
var bool	Flg_vt;		/* Virtual temperature on skew-t plots?	*/
var bool	Flg_hodo_msl;	/* Altitudes MSL in hodographs?	*/
var bool	Flg_barb;	/* Plot wind barbs on skew-t? */
var bool	Flg_logp;	/* Plot log(p) for xyplot and xsect plots */
var bool	Flg_datalim;	/* Use data to define interpolation limits */
var bool	Flg_lsqfit;	/* Use least squares fit for interpolation */
var bool	Flg_annotate;	/* Annotate on xsect plot? */
var bool	Flg_oldCLASS;	/* Write files in old class format? */
var bool	Flg_uwind;	/* Compute moisture flux in u-wind direction */
var bool	Flg_wmoQuiet;	/* suppress verbose complaints for WMO files? */
