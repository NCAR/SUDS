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

static char *rcsid = "$Id: fld_derive.c,v 1.16 1993-09-21 20:21:32 burghart Exp $";

# include <math.h>
# include <met_formulas.h>
# include "fields.h"
# include "flags.h"

/*
 * cflow barfs on varargs stuff, and we want to avoid that
 * (note that "cflow -Dcflow ..." is required for this to work)
 */
# ifndef cflow
#	include <varargs.h>
# else
#	define va_dcl	int va_alist;
# endif

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
	void	fdd_theta_v (), fdd_ri(), fdd_mflux(), fdd_mflux_uv();
	void	fdd_t_wet ();
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
	fdd_add_derivation (f_ri, fdd_ri, 4, f_u_wind, f_v_wind, f_theta, 
			    f_alt);
	fdd_add_derivation (f_mflux, fdd_mflux, 2, f_mr, f_wspd);
	fdd_add_derivation (f_mflux_uv, fdd_mflux_uv, 3, f_mr, f_u_wind, 
			    f_v_wind);
	fdd_add_derivation (f_t_wet, fdd_t_wet, 3, f_temp, f_pres, f_rh);
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


void
fdd_ri (buf, dbufs, npts, badval)
float	*buf, **dbufs, badval;
int	npts;
/*
 * Richardson Number derivation routine
 */
{
	float	*u_wind = dbufs[0], *v_wind = dbufs[1], *theta = dbufs[2];
	float	*alt = dbufs[3];
	double	 g, dz, dz2, du2, dtheta, theta_avg;
	int	i;

	g = 9.81;
	for (i = 1; i < npts-1; i++)
	{
		if (u_wind[i+1] == badval || v_wind[i+1] == badval || 
		    theta[i+1] == badval  || alt[i+1] == badval ||
		    u_wind[i-1] == badval || v_wind[i-1] == badval || 
		    theta[i-1] == badval || alt[i-1] == badval )
			buf[i] = badval;
		else
		{
			dz = alt[i+1] - alt[i-1];
			dz2 = dz*dz;
			du2 =((u_wind[i+1]-u_wind[i-1])*(u_wind[i+1]-u_wind[i-1]) +
			      (v_wind[i+1]-v_wind[i-1])*(v_wind[i+1]-v_wind[i-1]) ) ;
			dtheta = (theta[i+1]-theta[i-1]);
			theta_avg = ((theta[i+1]+theta[i-1])/2.0) ;

			/* This is very sensitive to noisy winds data */
			buf[i] = du2*theta_avg / (g*dtheta*dz) ;
		}
	}
	buf[0] = badval;
	buf[npts-1] = badval;
}


void
fdd_mflux (buf, dbufs, npts, badval)
float	*buf, **dbufs, badval;
int	npts;
/*
 * moisture flux derivation routine
 */
{
	float	*mr = dbufs[0], *wspd = dbufs[1];
	int	i;

	for (i = 0; i < npts; i++)
	{
		if (mr[i] == badval || wspd[i] == badval )
			buf[i] = badval;
		else
		{
			/* Wind speed times the specific humidity */
			buf[i] = wspd[i] * ( mr[i]/(1.0+mr[i]) );
		}
	}
}

void
fdd_mflux_uv (buf, dbufs, npts, badval)
float	*buf, **dbufs, badval;
int	npts;
/*
 * moisture flux (with respect to u or v) derivation routine
 */
{
	float	*mr = dbufs[0], *u_wind = dbufs[1], *v_wind = dbufs[2];
	int	i;

	for (i = 0; i < npts; i++)
	{
		if (mr[i] == badval || u_wind[i] == badval || v_wind[i] == badval )
			buf[i] = badval;
		else
		{
			/* Wind speed times the specific humidity */
			if ( Flg_uwind )
				buf[i] = u_wind[i] * ( mr[i]/(1.0+mr[i]) );
			else
				buf[i] = v_wind[i] * ( mr[i]/(1.0+mr[i]) );
		}
	}
}




void
fdd_t_wet (buf, dbufs, npts, badval)
float	*buf, **dbufs, badval;
int	npts;
/*
 * wet bulb temperature derivation routine
 */
{
	float	*temp = dbufs[0], *pres = dbufs[1], *rh = dbufs[2];
	int	i;

	for (i = 0; i < npts; i++)
	{
		if (temp[i] == badval || pres[i] == badval || rh[i] == badval)
			buf[i] = badval;
		else
			buf[i] = t_wet (temp[i] + T_K, pres[i], rh[i]) - T_K;
	}
}
