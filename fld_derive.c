/*
 * Fields derivation module
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
 */

static char *rcsid = "$Id: fld_derive.c,v 1.13 1992-07-30 20:41:10 burghart Exp $";

# include <math.h>
# include <varargs.h>
# include <met_formulas.h>
# include "fields.h"

# define TRUE	1
# define FALSE	0

# define T_K	273.15
# define R_D	287.
# define G_0	9.80665

# define DEG_TO_RAD(x)	((x) * 0.017453292)
# define RAD_TO_DEG(x)	((x) * 57.29577951)

/*
 * Table of field legal field derivations
 * and a boolean to tell if the table has been built yet
 */
typedef struct derive_entry
{
	int	nflds;
	fldtype	*flist;
	void	(*func)();
	struct derive_entry	*next;
} d_entry;

static d_entry	*Derive_table[TOTAL_FLDS];
static int	Derive_init = FALSE;




int
fdd_derive (fld, ndx, dflds, ndflds, dfunc)
fldtype	fld, **dflds;
int	ndx, *ndflds;
void	(**dfunc)();
/*
 * Return the ndx'th (ndx >= 0) derivation entry for field fld.
 * The function returns a boolean telling whether the entry exists.
 *
 * ON ENTRY:
 *	fld	field to be derived
 *	ndx	the number of the derivation entry to return
 * ON EXIT:
 *   (success) return TRUE and
 *	dflds	points to the derivation field array
 *	ndflds	contains the number of derivation fields
 *	dfunc	points to the derivation function
 *   (failure) return FALSE, parameters unchanged
 */
{
	int	i;
	d_entry	*entry;
/*
 * Make sure the derivation table has been built
 */
	if (! Derive_init)
		fdd_dt_init ();
/*
 * Find the ndx'th entry
 */
	entry = Derive_table[fld];

	for (i = 0; entry && i < ndx; i++)
		entry = entry->next;
/*
 * Return the derivation for this field, if any
 */
	if (entry)
	{
		*dflds = entry->flist;
		*ndflds = entry->nflds;
		*dfunc = entry->func;
		return (TRUE);
	}
	else
		return (FALSE);
}





fdd_dt_init ()
/*
 * Initialize the field derivation table
 */
{
	int	i;
	void	fdd_mr (), fdd_u_wind (), fdd_v_wind (), fdd_theta ();
	void	fdd_theta_e (), fdd_alt (), fdd_dp (), fdd_wspd ();
	void	fdd_wdir (), fdd_vt (), fdd_ascent (), fdd_rh ();
	void	fdd_theta_v ();
	void	fdd_add_derivation ();
/*
 * Start out with null lists for each field
 */
	for (i = 0; i < TOTAL_FLDS; i++)
		Derive_table[i] = (d_entry *) 0;
/*
 * Put in the known derivations
 */
	fdd_add_derivation (f_mr, fdd_mr, 2, f_dp, f_pres);
	fdd_add_derivation (f_u_wind, fdd_u_wind, 2, f_wspd, f_wdir);
	fdd_add_derivation (f_v_wind, fdd_v_wind, 2, f_wspd, f_wdir);
	fdd_add_derivation (f_theta, fdd_theta, 2, f_temp, f_pres);
	fdd_add_derivation (f_theta_e, fdd_theta_e, 3, f_temp, f_dp, f_pres);
	fdd_add_derivation (f_alt, fdd_alt, 3, f_dp, f_temp, f_pres);
	fdd_add_derivation (f_dp, fdd_dp, 2, f_temp, f_rh);
	fdd_add_derivation (f_wspd, fdd_wspd, 2, f_u_wind, f_v_wind);
	fdd_add_derivation (f_wdir, fdd_wdir, 2, f_u_wind, f_v_wind);
	fdd_add_derivation (f_vt, fdd_vt, 3, f_temp, f_pres, f_rh);
	fdd_add_derivation (f_ascent, fdd_ascent, 2, f_time, f_alt);
	fdd_add_derivation (f_rh, fdd_rh, 2, f_temp, f_dp);
	fdd_add_derivation (f_theta_v, fdd_theta_v, 3, f_temp, f_pres, f_rh);
/*
 * Mark the initialization as done
 */
	Derive_init = TRUE;
}




void
fdd_add_derivation (va_alist)
va_dcl		/* variable args declaration, no semicolon! */
/*
 * Add a derivation function for fld to the table of legal derivations
 * The arguments to this routine are (in order):
 *
 *	fld	the field which can be derived
 *	func	the derivation function
 *	nflds	the number of fields used in the derivation
 *	fld[0]	the first field in the derivation
 *       .
 *	 .
 *	fld[nflds-1]	the last field in the derivation
 */
{
	fldtype fld;
	d_entry	*entry, *tail;
	int	nflds;
	void	(*func)();
	int	i;
	va_list	args;

	va_start (args);
/*
 * Get the first three arguments
 */
	fld = va_arg (args, fldtype);
	func = (void (*)()) va_arg (args, void*);
	nflds = va_arg (args, int);
/*
 * Allocate a new derivation table entry and fill it
 */
	entry = (d_entry *) malloc (sizeof (d_entry));
	entry->nflds = nflds;
	entry->func = func;
	entry->next = (d_entry *) 0;
	entry->flist = (fldtype *) malloc (nflds * sizeof (fldtype));

	for (i = 0; i < nflds; i++)
		entry->flist[i] = va_arg (args, fldtype);
/*
 * Add this entry to the list of derivations for this field
 */
	if (! Derive_table[fld])
		Derive_table[fld] = entry;
	else
	{
		tail = Derive_table[fld];
		while (tail->next)
			tail = tail->next;

		tail->next = entry;
	}
/*
 * Finish up
 */
	va_end (args);
}




void
fdd_mr (buf, dbufs, npts, badval)
float	*buf, **dbufs, badval;
int	npts;
/*
 * Mixing ratio derivation routine
 */
{
	float	*dp = dbufs[0], *pres = dbufs[1];
	int	i;

	for (i = 0; i < npts; i++)
	{
		if (dp[i] == badval || pres[i] == badval)
			buf[i] = badval;
		else
			buf[i] = w_sat (dp[i] + T_K, pres[i]);
	}
}




void
fdd_u_wind (buf, dbufs, npts, badval)
float	*buf, **dbufs, badval;
int	npts;
/*
 * u_wind derivation routine
 */
{
	float	*wspd = dbufs[0], *wdir = dbufs[1];
	float	ang;
	int	i;

	for (i = 0; i < npts; i++)
	{
		if (wspd[i] == badval || wdir[i] == badval)
			buf[i] = badval;
		else
		{
			ang = DEG_TO_RAD (270.0 - wdir[i]);
			buf[i] = wspd[i] * cos (ang);
		}
	}
}




void
fdd_v_wind (buf, dbufs, npts, badval)
float	*buf, **dbufs, badval;
int	npts;
/*
 * v_wind derivation routine
 */
{
	float	*wspd = dbufs[0], *wdir = dbufs[1];
	float	ang;
	int	i;

	for (i = 0; i < npts; i++)
	{
		if (wspd[i] == badval || wdir[i] == badval)
			buf[i] = badval;
		else
		{
			ang = DEG_TO_RAD (270.0 - wdir[i]);
			buf[i] = wspd[i] * sin (ang);
		}
	}
}




void
fdd_theta (buf, dbufs, npts, badval)
float	*buf, **dbufs, badval;
int	npts;
/*
 * potential temperature derivation routine
 */
{
	float	*temp = dbufs[0], *pres = dbufs[1];
	int	i;

	for (i = 0; i < npts; i++)
	{
		if (temp[i] == badval || pres[i] == badval)
			buf[i] = badval;
		else
			buf[i] = theta_dry (temp[i] + T_K, pres[i]);
	}
}




void
fdd_theta_e (buf, dbufs, npts, badval)
float	*buf, **dbufs, badval;
int	npts;
/*
 * equivalent potential temperature derivation routine
 */
{
	float	*t = dbufs[0], *dp = dbufs[1], *pres = dbufs[2];
	int	i;

	for (i = 0; i < npts; i++)
	{
		if (t[i] == badval || dp[i] == badval || pres[i] == badval)
			buf[i] = badval;
		else
			buf[i] = theta_e (t[i] + T_K, dp[i] + T_K, pres[i]);
	}
}




void
fdd_alt (buf, dbufs, npts, badval)
float	*buf, **dbufs, badval;
int	npts;
/*
 * altitude derivation routine
 *
 * IMPORTANT NOTE:  The altitudes returned are not MSL!  The heights returned
 * are relative to the first good point (essentially AGL).
 */
{
	float	*dp = dbufs[0], *temp = dbufs[1], *pres = dbufs[2];
	float	e, vt, pres_prev = badval, alt_prev, vt_prev;
	int	i;
/* 
 * This derivation is adapted from equation 2.24 of Wallace and
 * Hobbs' "Atmospheric Science" (1977) using a simple estimation
 * of the integral.
 */
	for (i = 0; i < npts; i++)
	{
		if (dp[i] == badval || temp[i] == badval || pres[i] == badval)
			buf[i] = badval;
		else
		{
		/*
		 * Get the vapor pressure and virtual temperature
		 */
			e = e_from_dp (dp[i] + T_K);
			vt = t_v (temp[i] + T_K, pres[i], e);
		/*
		 * Assign the altitude (the first good point is set to
		 * 0 km altitude
		 */
			if (pres_prev == badval)
				buf[i] = 0.0;
			else
				buf[i] = alt_prev + R_D / G_0 * 0.5 * 
					(vt / pres[i] + vt_prev / pres_prev) * 
					(pres_prev - pres[i]);

			pres_prev = pres[i];
			vt_prev = vt;
			alt_prev = buf[i];
		}
	}
}




void
fdd_dp (buf, dbufs, npts, badval)
float	*buf, **dbufs, badval;
int	npts;
/*
 * dewpoint derivation routine
 */
{
	float	*temp = dbufs[0], *rh = dbufs[1];
	float	e;
	int	i;

	for (i = 0; i < npts; i++)
	{
		if (temp[i] == badval || rh[i] == badval)
			buf[i] = badval;
		else
		{
			e = rh[i] / 100.0 * e_sw (temp[i] + T_K);
			buf[i] = dewpoint (e) - T_K;
		}
	}
}




void
fdd_wdir (buf, dbufs, npts, badval)
float	*buf, **dbufs, badval;
int	npts;
/*
 * wind direction derivation routine
 */
{
	float	*u_wind = dbufs[0], *v_wind = dbufs[1];
	float	ang;
	int	i;

	for (i = 0; i < npts; i++)
	{
		if (u_wind[i] == badval || v_wind[i] == badval)
			buf[i] = badval;
		else
			buf[i] = -90.0 - 
				RAD_TO_DEG (atan2 (v_wind[i], u_wind[i]));
	}
}




void
fdd_wspd (buf, dbufs, npts, badval)
float	*buf, **dbufs, badval;
int	npts;
/*
 * wind speed derivation routine
 */
{
	float	*u_wind = dbufs[0], *v_wind = dbufs[1];
	float	ang;
	int	i;

	for (i = 0; i < npts; i++)
	{
		if (u_wind[i] == badval || v_wind[i] == badval)
			buf[i] = badval;
		else
			buf[i] = hypot (u_wind[i], v_wind[i]);
	}
}




void
fdd_vt (buf, dbufs, npts, badval)
float	*buf, **dbufs, badval;
int	npts;
/*
 * virtual temperature derivation routine
 */
{
	float	*temp = dbufs[0], *pres = dbufs[1], *rh = dbufs[2];
	int	i;

	for (i = 0; i < npts; i++)
	{
		if (temp[i] == badval || pres[i] == badval || rh[i] == badval)
			buf[i] = badval;
		else
			buf[i] = t_v (temp[i] + T_K, pres[i], 
				0.01 * rh[i] * e_sw (temp[i] + T_K));
	}
}




void
fdd_ascent (buf, dbufs, npts, badval)
float	*buf, **dbufs, badval;
int	npts;
/*
 * ascent rate derivation routine
 */
{
	float	*time = dbufs[0], *alt = dbufs[1];
	float	prevtime, prevalt = badval;
	int	i;

	for (i = 0; i < npts; i++)
	{
		if (time[i] == badval || alt[i] == badval)
		{
			buf[i] = badval;
			continue;
		}
		else if (prevalt != badval)
			buf[i] = (alt[i] - prevalt) / (time[i] - prevtime);

		prevtime = time[i];
		prevalt = alt[i];
	}
}




void
fdd_rh (buf, dbufs, npts, badval)
float	*buf, **dbufs, badval;
int	npts;
/*
 * derive relative humidity from temperature and dewpoint
 */
{
	float	*temp = dbufs[0], *dp = dbufs[1];
	int	i;

	for (i = 0; i < npts; i++)
	{
		if (temp[i] == badval || dp[i] == badval)
		{
			buf[i] = badval;
			continue;
		}
		else
			buf[i] = 100.0 * e_from_dp (dp[i] + T_K) / 
				e_sw (temp[i] + T_K);
	}
}




void
fdd_theta_v (buf, dbufs, npts, badval)
float	*buf, **dbufs, badval;
int	npts;
/*
 * virtual potential temperature derivation routine
 */
{
	float	*temp = dbufs[0], *pres = dbufs[1], *rh = dbufs[2], theta;
	int	i;

	for (i = 0; i < npts; i++)
	{
		if (temp[i] == badval || pres[i] == badval || rh[i] == badval)
			buf[i] = badval;
		else
		{
			theta = theta_dry (temp[i] + T_K, pres[i]);
			buf[i] = t_v (theta, pres[i], 
				0.01 * rh[i] * e_sw (temp[i] + T_K));
		}
	}
}
