/*
 * Fields information
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
 * $Id: fields.h,v 1.12 1993-04-28 16:18:29 carson Exp $
 */

typedef enum
{
/* 0 */		f_null, f_time, f_lat, f_lon, f_alt,
/* 5 */		f_pres, f_temp, f_dp, f_rh, f_wspd,
/* 10 */	f_wdir, f_u_wind, f_v_wind, f_theta, f_theta_e,
/* 15 */	f_mr, f_qpres, f_qtemp, f_qrh, f_qu,
/* 20 */	f_qv, f_qwind, f_rtype, f_range, f_azimuth,
/* 25 */	f_u_prime, f_v_prime, f_ascent, f_vt, f_theta_v,
/* 30 */	f_ri, f_mflux, f_mflux_uv
} fldtype;

# define TOTAL_FLDS 32

/*
 * prototypes
 */
# ifdef __STDC__
	fldtype	fd_num (char *);
	char	*fd_name (fldtype);
	char	*fd_desc (fldtype);
	char	*fd_units (fldtype);
	double	fd_bot (fldtype);
	double	fd_top (fldtype);
	double	fd_center (fldtype);
	double	fd_step (fldtype);
	double  fd_ibot (fldtype);
	double  fd_itop (fldtype);
	double  fd_istep (fldtype);
	double  fd_igap (fldtype);
# else
	fldtype fd_num ();
	char	*fd_name ();
	char	*fd_desc ();
	char	*fd_units ();
	double	fd_bot ();
	double	fd_top ();
	double	fd_center ();
	double	fd_step ();
	double  fd_ibot ();
	double  fd_itop ();
	double  fd_istep ();
	double  fd_igap ();
# endif
