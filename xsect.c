/*
 * Vertical cross-sectioning
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

static char *rcsid = "$Id: xsect.c,v 1.17 1991-10-21 21:58:47 burghart Exp $";

# include <math.h>
# include <ui_param.h>
# include <ui_date.h>
# include "globals.h"
# include "keywords.h"
# include "sounding.h"
# include "color.h"

# define DEG_TO_RAD(x)	((x) * .017453292)
# define MAX(a,b)	((a) > (b) ? (a) : (b))

/*
 * The cross-section plane array, its length, its height, 
 * its dimensions, and a macro to reference it two-dimensionally
 */
static float	*Plane, *P_wgt, P_len, P_hgt, P_bot;

# define HDIM	50
# define VDIM	50

# define PLANE(i,j)	(Plane[((i)*VDIM)+(j)])
# define P_WGT(i,j)	(P_wgt[((i)*VDIM)+(j)])

/*
 * Floor and ceiling arrays and their associated weight arrays, so we 
 * know where data should not be placed
 */
static float	*Floor, *Ceiling;
static float	*F_wgt, *C_wgt;

/*
 * The endpoints of the plane for a spatial cross-section, and the starting
 * time for a time-height cross-section
 */
static float	X0 = 0.0, X1 = 0.0, Y0 = 0.0, Y1 = 0.0;
static date	T0;

/*
 * Macro to identify points in the plot area
 */
# define INPLOT(x,y)	((x)>=0.0 && (x)<=P_len && ((y)*((y)-P_hgt)<=0.0))

/*
 * The overlays for the plot
 */
static overlay	Xs_ov, Xs_bg_ov;

/*
 * Altitude or pressure on the vertical scale?
 */
static int	Use_alt = TRUE;

/*
 * Time-height plot?
 */
static int	Time_height;

/*
 * The soundings and field to be used
 */
static fldtype	Fld;
static char	*S_id[20];
static int	Nsnd = 0;

/*
 * Current text position for the side annotation
 */
static float	Xtxt_pos, Ytxt_pos;

/*
 * Points for a sounding trace
 */
static float	Xtrace[BUFLEN], Ytrace[BUFLEN];
static int	Tracelen = 0;

/*
 * Forward declarations
 */
void	xs_abort (), xs_ov_check (), xs_background (), xs_side_text ();
void	xs_reset_annot (), xs_plot (), xs_vscale ();
void	xs_from_to (), xs_pos (), xs_timepos (), xs_time_height ();
void	xs_spatial (), xs_special_data (), xs_extend_trace ();
void	xs_draw_trace (), xs_center_angle (), xs_add_to_level ();
void	xs_build_limits ();




void
xs_xsect (cmds)
struct ui_command	*cmds;
/*
 * Handle the xsect command in its many forms
 */
{
	int	keyword = 0;
/*
 * If the first command is a keyword, extract it
 */
	if (cmds[0].uc_ctype == UTT_KW)
	{
		keyword = UKEY (cmds[0]);
		cmds++;
	}
/*
 * Switch on the keyword
 */
	switch (keyword)
	{
	    case KW_TIMHGT:
		xs_time_height ();
		xs_plot (cmds);
		break;
	    case KW_USE:
		Nsnd = 0;
		for (; cmds->uc_ctype != UTT_END; cmds++)
		{
			S_id[Nsnd] = (char *) malloc (1+sizeof (UPTR (*cmds)));
			strcpy (S_id[Nsnd], UPTR (*cmds));
			Nsnd++;
		}

		if (Nsnd < 2)
		{
			Nsnd = 0;
			ui_error ("At least two soundings must be used!");
		}

		break;
	    case KW_FROM:
		xs_from_to (cmds);
		break;
	    case KW_CENTER:
		xs_center_angle (cmds);
		break;
	    case KW_VSCALE:
		xs_vscale (cmds);
		break;
	    default:
		xs_spatial ();
		xs_plot (cmds);
	}
	return;
}




void
xs_plot (cmds)
struct ui_command	*cmds;
/*
 * Actually do the cross-section plot
 */
{
	int	i, j;
	fldtype	vfld = Use_alt ? f_alt : f_pres;
	void	contour (), contour_init ();
	overlay ovlist[2];
/*
 * Get the field to plot
 */
	Fld = fd_num (UPTR (*cmds));
/*
 * Get the vertical limits
 */
	P_bot = fd_bot (vfld);
	P_hgt = fd_top (vfld) - P_bot;
/*
 * If we're using altitude, convert to km
 */
	if (Use_alt)
	{
		P_bot *= 0.001;
		P_hgt *= 0.001;
	}
/*
 * Reset the annotation position
 */
	xs_reset_annot ();
/*
 * Make sure everything necessary has been specified
 */
	if (Nsnd == 0)
		ui_error ("No soundings have been specified!");

	if (! Time_height && X0 == X1 && Y0 == Y1)
		ui_error ("The cross-section endpoints must be specified!");
/*
 * Make sure the graphics stuff is ready
 */
	out_ws_check ();
	xs_ov_check ();
/*
 * Let edit know about this plot, even though we can't edit it
 */
	ovlist[0] = Xs_ov;
	ovlist[1] = Xs_bg_ov;
	edit_set_plot (xs_plot, 0, cmds, ovlist, 2);
/*
 * Allocate space for the plane and weight arrays
 */
	Plane = (float *) malloc (HDIM * VDIM * sizeof (float));
	P_wgt = (float *) malloc (HDIM * VDIM * sizeof (float));
/*
 * Get the floor and ceiling arrays and their associated weight arrays
 */
	Floor = (float *) malloc (HDIM * sizeof (float));
	F_wgt = (float *) malloc (HDIM * sizeof (float));
	Ceiling = (float *) malloc (HDIM * sizeof (float));
	C_wgt = (float *) malloc (HDIM * sizeof (float));
/*
 * Get the overlays ready and draw the background
 */
	G_clear (Xs_ov);
	G_set_coords (Xs_ov, -BORDER * P_len, -BORDER * P_hgt, 
		P_len * (1+BORDER), P_hgt * (1+BORDER));
	G_clip_window (Xs_ov, 0.0, 0.0, P_len, P_hgt);

	xs_background ();
/*
 * Fill the plane with BADVALs and set the weights to zero.
 * Initialize the floor and ceiling arrays also.
 */
	for (i = 0; i < HDIM; i++)
	{
		for (j = 0; j < VDIM; j++)
		{
			PLANE (i, j) = BADVAL;
			P_WGT (i, j) = 0.0;
		}

		Floor[i] = BADVAL;
		F_wgt[i] = 0.0;
		Ceiling[i] = BADVAL;
		C_wgt[i] = 0.0;
	}
/*
 * Fill the data plane
 */
	xs_put_data ();
/*
 * Draw the contours
 */
	contour_init (C_DATA5, 9, C_BG1, TRUE, BADVAL);
	contour (Plane, HDIM, VDIM, Xs_ov, 0.0, 0.0, P_len, P_hgt, 
		fd_center (Fld), fd_step (Fld));
/*
 * Update the display
 */
	G_update (Wkstn);
/*
 * Release the arrays and exit
 */
	free (Plane);
	free (P_wgt);
	free (Floor);
	free (F_wgt);
	free (Ceiling);
	free (C_wgt);
	return;
}




xs_put_data ()
/*
 * Fill the cross-section array with data from the chosen soundings
 */
{
	int	snd, pt, npts, iz, zndx, zndx_prev, ih, iv, zlo, zhi;
	float	val, x, y, z, t, val_prev, x_prev, y_prev, z_prev, t_prev;
	float	zstep, hlen, frac;
	float	xhighest, yhighest, zhighest, xlowest, ylowest, zlowest;
	float	*fdata, *xpos, *ypos, *zpos, *tpos;
	float	snd_s_alt ();
/*
 * Array allocation
 */
	fdata = (float *) malloc (BUFLEN * sizeof (float));
	xpos = tpos = (float *) malloc (BUFLEN * sizeof (float));
	ypos = (float *) malloc (BUFLEN * sizeof (float));
	zpos = (float *) malloc (BUFLEN * sizeof (float));
/*
 * Vertical grid spacing
 */
	zstep = P_hgt / (float)(VDIM - 1);
/*
 * Loop through the soundings
 */
	for (snd = 0; snd < Nsnd; snd++)
	{
	/*
	 * Check for an interrupt
	 */
		if (Interrupt)
			xs_abort ();
	/*
	 * Initialize for keeping track of the highest and lowest points
	 */
		zhighest = (P_hgt > 0.0) ? -99999.0 : 99999.0;
		zlowest = (P_hgt > 0.0) ? 99999.0 : -99999.0;
	/*
	 * Get the vertical (altitude or pressure) data
	 */
		if (Use_alt)
		{
		/*
		 * Get the altitude and convert to km
		 */
			npts = snd_get_data (S_id[snd], zpos, BUFLEN, f_alt, 
				BADVAL);

			for (pt = 0; pt < npts; pt++)
				if (zpos[pt] != BADVAL)
					zpos[pt] *= 0.001;
		}
		else
		/*
		 * Get the pressure data
		 */
			npts = snd_get_data (S_id[snd], zpos, BUFLEN, f_pres, 
				BADVAL);
	/*
	 * Put together the x,y position data
	 */
		if (Time_height)
			xs_timepos (S_id[snd], tpos);
		else
			xs_pos (S_id[snd], xpos, ypos);
	/*
	 * Get the data for the field to be contoured
	 */
		if (Fld == f_u_prime || Fld == f_v_prime)
			xs_special_data (S_id[snd], fdata, Fld);
		else
			snd_get_data (S_id[snd], fdata, BUFLEN, Fld, BADVAL);
	/*
	 * Loop through the points
	 */
		val_prev = BADVAL;

		for (pt = 0; pt < npts; pt++)
		{
		/*
		 * Bag this point if the datum or position is bad
		 */
			if (Time_height)
			{
				if (tpos[pt] == BADVAL)
					continue;
			}
			else
			{
				if (xpos[pt] == BADVAL || ypos[pt] == BADVAL)
					continue;
			}

			if (fdata[pt] == BADVAL || zpos[pt] == BADVAL)
				continue;
		/*
		 * Keep track of the highest and lowest points
		 */
			if ((P_hgt > 0.0 && zpos[pt] > zhighest) ||
				(P_hgt < 0.0 && zpos[pt] < zhighest))
			{
				xhighest = xpos[pt];
				yhighest = ypos[pt];
				zhighest = zpos[pt];
			}

			if ((P_hgt > 0.0 && zpos[pt] < zlowest) ||
				(P_hgt < 0.0 && zpos[pt] > zlowest))
			{
				xlowest = xpos[pt];
				ylowest = ypos[pt];
				zlowest = zpos[pt];
			}
		/*
		 * Special treatment for the first good point
		 */
			if (val_prev == BADVAL)
			{
			/*
			 * Assign the previous point values
			 */
				val_prev = fdata[pt];
				z_prev = zpos[pt];
				zndx_prev = xs_zindex (zpos[pt]);

				if (Time_height)
					t_prev = tpos[pt];
				else
				{
					x_prev = xpos[pt];
					y_prev = ypos[pt];
				}
			/*
			 * Go on to the next point
			 */
				continue;
			}
		/*
		 * Find the index of the next grid height at or above zpos[pt]
		 */
			zndx = xs_zindex (zpos[pt]);
		/*
		 * Assign values at grid levels between this point and the
		 * previous one
		 */
			if (zndx_prev < zndx)
			{
				zlo = MAX (zndx_prev, 0);
				zhi = MIN (zndx, VDIM - 1);
			}
			else
			{
				zlo = MAX (zndx, 0);
				zhi = MIN (zndx_prev, VDIM - 1);
			}

			for (iz = zlo; iz < zhi; iz++)
			{
			/*
			 * Find the height of this grid index and interpolate
			 * the data and position to this height
			 */
				z = iz * zstep + P_bot;
				frac = (z - z_prev) / (zpos[pt] - z_prev);

				val = val_prev + frac * (fdata[pt] - val_prev);

				if (Time_height)
					t = (t_prev + frac * 
						(tpos[pt] - t_prev)) / 3600.0;
				else
				{
					x = x_prev + frac * (xpos[pt]-x_prev);
					y = y_prev + frac * (ypos[pt]-y_prev);
				}
			/*
			 * Add this datum in at the current height index
			 */
				if (Time_height)
					xs_add_to_level (iz, t, 0.0, val);
				else
					xs_add_to_level (iz, x, y, val);
			}
		/*
		 * Project this point onto the plane, and add a point to 
		 * the trace for this sounding
		 */
			if (Time_height)
				hlen = tpos[pt] / 3600.0;	/* to hours */
			else if (xpos[pt] == X0 && ypos[pt] == Y0)
				hlen = 0.0;
			else
				hlen = hypot (xpos[pt] - X0, ypos[pt] - Y0) *
					cos (atan2 (ypos[pt]-Y0, xpos[pt]-X0) -
					atan2 (Y1-Y0, X1-X0));

			xs_extend_trace (hlen, zpos[pt] - P_bot);
		/*
		 * Make this the previous point
		 */
			val_prev = fdata[pt];
			z_prev = zpos[pt];
			zndx_prev = zndx;
			if (Time_height)
				t_prev = tpos[pt];
			else
			{
				x_prev = xpos[pt];
				y_prev = ypos[pt];
			}
		}
	/*
	 * Use the lowest point of this sounding to help build the floor array
	 */
		xs_build_limits (Floor, F_wgt, xlowest, ylowest, zlowest);
	/*
	 * Use the highest point to help build the ceiling array
	 */
		xs_build_limits (Ceiling, C_wgt, xhighest, yhighest, zhighest);
	/*
	 * Draw the trace for this sounding
	 */
		xs_draw_trace (snd);
	}
/*
 * Remove data below the floor and above the ceiling
 */
	for (ih = 0; ih < HDIM; ih++)
	{
		for (iv = 0; iv < xs_zindex (Floor[ih]); iv++)
			PLANE(ih,iv) = BADVAL;

		for (iv = xs_zindex (Ceiling[ih]); iv < VDIM; iv++)
			PLANE(ih,iv) = BADVAL;
	}
/*
 * The plane array is populated, free the allocated memory and return
 */
	free (fdata);
	free (xpos);
	free (ypos);
	free (zpos);

	return;
}




int
xs_zindex (z)
float	z;
/*
 * Return the index of the first grid level at or above z
 */
{
	float	fndx = (z - P_bot) / P_hgt * (VDIM - 1);

	if ((float)((int) fndx) == fndx)
		return ((int) fndx);	
	else
		return ((int) fndx + 1);
}




void
xs_add_to_level (iz, xdat, ydat, vdat)
int	iz;
float	vdat, xdat, ydat;
/*
 * Apply the point with value vdat located at (xdat,ydat) to the grid 
 * at height index iz.  (For time-height plots, xdat should be the time 
 * position and ydat should be zero)
 */
{
	int	ih;
	float	x, y, d, xstep, ystep, tstep, wgt;
/*
 * Sanity check
 */
	if (iz < 0 || iz >= VDIM)
		ui_error ("*BUG* Bad vertical index in xs_add_to_level");
/*
 * Step through the grid horizontally at height index iz and use a distance
 * weighting scheme to apply the given point
 */
	if (Time_height)
		tstep = P_len / (HDIM - 1);
	else
	{
		xstep = (X1 - X0) / (HDIM - 1);
		ystep = (Y1 - Y0) / (HDIM - 1);
	}


	for (ih = 0; ih < HDIM; ih++)
	{
		if (Time_height)
		{
			x = ih * tstep;
			y = 0.0;
		}
		else
		{
			x = X0 + ih * xstep;
			y = Y0 + ih * ystep;
		}

		d = sqrt ((xdat - x) * (xdat - x) + (ydat - y) * (ydat - y));
	/*
	 * Use a 1/d^2 weighting scheme, truncated at a weight of 100
	 */
		if (d < 0.1)
			wgt = 100;
		else
			wgt = 1.0 / (d * d);
	/*
	 * Apply the point
	 */
		PLANE(ih,iz) = (PLANE(ih,iz) * P_WGT(ih,iz) + vdat * wgt) /
			(P_WGT(ih,iz) + wgt);
		P_WGT(ih,iz) = P_WGT(ih,iz) + wgt;
	}
}




void
xs_build_limits (array, weight, xdat, ydat, zdat)
float	*array, *weight;
float	xdat, ydat, zdat;
/*
 * Use the given point to help build either the given ceiling or floor
 * array.  For time-height plots, xdat should be the time position and 
 * ydat should be zero.
 */
{
	int	ih;
	float	pos, d, hstep, wgt;
/*
 * Horizontal step length in the plane
 */
	hstep = P_len / (HDIM - 1);
/*
 * Find the distance from the left endpoint of the plane to the projection
 * of the given point onto the plane.
 */
	if (Time_height)
		pos = xdat;
	else if (xdat == X0 && ydat == Y0)
		pos = 0.0;
	else
		pos = hypot (xdat - X0, ydat - Y0) * 
			cos (atan2 (ydat-Y0, xdat-X0) - atan2 (Y1-Y0, X1-X0));
/*
 * Step through the array and use a distance weighting scheme to apply 
 * the given point.  The point is projected onto the plane before distances
 * are calculated. 
 */
	for (ih = 0; ih < HDIM; ih++)
	{
	/*
	 * Find the distance from this array point to the projection of
	 * the input point
	 */
		d = fabs (ih * hstep - pos);
	/*
	 * Use a 1/d^2 weighting scheme, truncated at a weight of 100
	 */
		if (d < 0.1)
			wgt = 100;
		else
			wgt = 1.0 / (d * d);
	/*
	 * Apply the point
	 */
		array[ih] = (array[ih] * weight[ih] + zdat * wgt) /
			(weight[ih] + wgt);
		weight[ih] = weight[ih] + wgt;
	}
}




void
xs_ov_check ()
/*
 * Make sure our overlays exist and are associated with the 
 * current workstation
 */
{
/*
 * Do we have an overlay for the background?
 */
	if (! Xs_bg_ov ||  G_ov_to_ws (Xs_bg_ov) != Wkstn)
		Xs_bg_ov = G_new_overlay (Wkstn, 0);
/*
 * Do we have an overlay for the data?
 */
	if (! Xs_ov || G_ov_to_ws (Xs_ov) != Wkstn) 
		Xs_ov = G_new_overlay (Wkstn, 0);

	return;
}




void
xs_background ()
/*
 * Draw the background for this cross-section
 */
{
	float	x[5], y[5], tick, tickinc, lat, lon, lolim, hilim, charsize;
	char	string[80], ctime0[20], ctime1[20];
	int	i, dolabel, seconds;
	char	*snd_site ();
	date	sdate, del_time, end_time, snd_time ();

	G_clear (Xs_bg_ov);
	G_set_coords (Xs_bg_ov, -BORDER * P_len, -BORDER * P_hgt, 
		P_len * (1+BORDER), P_hgt * (1+BORDER));
/*
 * Character size
 */
	charsize = 0.025 * P_hgt;
/*
 * Title
 */
	sprintf (string, "CROSS-SECTION OF %s (%s)", fd_desc (Fld),
		fd_units (Fld));
	strcpyUC (string, string);

	G_write (Xs_bg_ov, C_WHITE, GTF_DEV, charsize, GT_CENTER, 
		GT_BOTTOM, 0.5 * P_len, 1.10 * P_hgt, 0.0, string);
/*
 * Horizontal limits (space or time)
 */
	if (Time_height)
	{
	/*
	 * Get the end time in date struct format
	 */
		seconds = (int) (P_len * 3600);

		del_time.ds_yymmdd = seconds / 86400;
		seconds %= 86400;

		del_time.ds_hhmmss = (seconds / 3600) * 10000 + 
			((seconds / 60) % 60) * 100 +
			(seconds % 60);

		ud_add_date (&T0, &del_time, &end_time);
	/*
	 * Write the begin and end times into string
	 */
		ud_format_date (ctime0, &T0, UDF_FULL);
		ud_format_date (ctime1, &end_time, UDF_FULL);
		sprintf (string, "FROM %s TO %s", ctime0, ctime1);
		strcpyUC (string, string);
	}
	else
	/*
	 * Write the start and end points into string
	 */
		sprintf (string, "FROM (%.1f,%.1f) KM TO (%.1f,%.1f) KM", 
			X0, Y0, X1, Y1);

	G_write (Xs_bg_ov, C_WHITE, GTF_DEV, charsize, GT_CENTER,
		GT_BOTTOM, 0.5 * P_len, 1.07 * P_hgt, 0.0, string);
/*
 * Origin
 */
	if (! Time_height)
	{
		cvt_get_origin (&lat, &lon);
		sprintf (string, 
			"RELATIVE TO LATITUDE: %.3f\027 LONGITUDE: %.3f\027",
			lat, lon);
		G_write (Xs_bg_ov, C_WHITE, GTF_DEV, charsize, GT_CENTER,
			GT_BOTTOM, 0.5 * P_len, 1.04 * P_hgt, 0.0, string);
	}
/*
 * Soundings used
 */
	for (i = 0; i < Nsnd; i++)
	{
		xs_side_text ("SITE: ", C_WHITE, TRUE);
		xs_side_text (snd_site (S_id[i]), C_WHITE, FALSE);
		xs_side_text ("  TIME: ", C_WHITE, FALSE);
		sdate = snd_time (S_id[i]);
		ud_format_date (string, &sdate, UDF_FULL);
		strcpyUC (string, string);
		xs_side_text (string, C_WHITE, FALSE);
		strcpy (string, "  (");
		strcpyUC (string + 3, S_id[i]);
		strcat (string, ")");
		xs_side_text (string, C_WHITE, FALSE);
	}
/*
 * Draw a box
 */
	x[0] = 0.0;	y[0] = 0.0;
	x[1] = P_len;	y[1] = 0.0;
	x[2] = P_len;	y[2] = P_hgt;
	x[3] = 0.0;	y[3] = P_hgt;
	x[4] = 0.0;	y[4] = 0.0;

	G_polyline (Xs_bg_ov, GPLT_SOLID, C_WHITE, 5, x, y);
/*
 * Figure out where to put ticks on horizontal axes
 */
	if (Time_height)
	{
		tickinc = pow (10.0, floor (log10 (P_len)));

		if ((P_len / tickinc) < 1.5)
			tickinc *= 0.1;
		else if ((P_len / tickinc) < 3.0)
			tickinc *= 0.2;
		else if ((P_len / tickinc) < 8.0)
			tickinc *= 0.5;
	/*
	 * Don't make tick increment finer than 1/2 hour
	 */
		if (Time_height && tickinc < 0.5)
			tickinc = 0.5;
	}
	else
		tickinc = P_len / 8.0;
/*
 * Label the horizontal axes
 */
	dolabel = TRUE;

	for (tick = 0.0; tick <= 1.005 * P_len; tick += tickinc)
	{
	/*
	 * Check for an interrupt
	 */
		if (Interrupt)
			xs_abort ();
	/*
	 * Draw the tick
	 */
		x[0] = x[1] = tick;
		y[0] = 0.0;
		y[1] = 0.025 * P_hgt;
		G_polyline (Xs_bg_ov, GPLT_SOLID, C_WHITE, 2, x, y);

		y[0] = 0.975 * P_hgt;
		y[1] = P_hgt;
		G_polyline (Xs_bg_ov, GPLT_SOLID, C_WHITE, 2, x, y);
	/*
	 * Label every other tick
	 */
		if (dolabel)
		{
			if (Time_height)
				sprintf (string, "%d", (int) tick);
			else
				sprintf (string, "(%.1f,%.1f)", 
					X0 + tick / P_len * (X1 - X0),
					Y0 + tick / P_len * (Y1 - Y0));

			G_write (Xs_bg_ov, C_WHITE, GTF_DEV, charsize, 
				GT_CENTER, GT_TOP, (float) tick, 
				-0.01 * P_hgt, 0.0, string);
		}
		dolabel = ! dolabel;
	}

	if (Time_height)
		G_write (Xs_bg_ov, C_WHITE, GTF_DEV, charsize, GT_CENTER,
			GT_TOP, 0.5 * P_len, -0.06 * P_hgt, 0.0, 
			"TIME FROM START (HOURS)");
	else
		G_write (Xs_bg_ov, C_WHITE, GTF_DEV, charsize, GT_CENTER,
			GT_TOP, 0.5 * P_len, -0.06 * P_hgt, 0.0, "POS (KM)");
/*
 * Get the lower and upper limits of the vertical axis
 */
	if (P_hgt > 0.0)
	{
		lolim = P_bot;
		hilim = P_bot + P_hgt;
	}
	else
	{
		lolim = P_bot + P_hgt;
		hilim = P_bot;
	}
/*
 * Figure out where to put ticks on the vertical axes
 */
	tickinc = pow (10.0, floor (log10 (fabs (P_hgt))));

	if (fabs (P_hgt / tickinc) < 1.5)
		tickinc *= 0.1;
	else if (fabs (P_hgt / tickinc) < 3.0)
		tickinc *= 0.2;
	else if (fabs (P_hgt / tickinc) < 8.0)
		tickinc *= 0.5;
/*
 * Adjust the lower limit to a reasonable label value
 */
	lolim = tickinc * ceil (lolim / tickinc);
/*
 * Loop to put tick marks and labels on vertical axes
 */
	dolabel = ((int)(lolim / tickinc) % 2) == 0;

	for (tick = lolim; tick <= hilim; tick += tickinc)
	{
	/*
	 * Check for an interrupt
	 */
		if (Interrupt)
			xs_abort ();
	/*
	 * Draw the tick
	 */
		y[0] = y[1] = tick - P_bot;
		x[0] = 0.0;
		x[1] = 0.025 * P_len;
		G_polyline (Xs_bg_ov, GPLT_SOLID, C_WHITE, 2, x, y);

		x[0] = 0.975 * P_len;
		x[1] = P_len;
		G_polyline (Xs_bg_ov, GPLT_SOLID, C_WHITE, 2, x, y);
	/*
	 * Label every other tick
	 */
		if (dolabel)
		{
			sprintf (string, "%d", (int) tick);
			G_write (Xs_bg_ov, C_WHITE, GTF_DEV, 
				charsize, GT_RIGHT, GT_CENTER,
				-0.01 * P_len, tick - P_bot, 0.0, string);
		}
		dolabel = ! dolabel;
	}

	if (Use_alt)
		G_write (Xs_bg_ov, C_WHITE, GTF_MINSTROKE, charsize, 
			GT_CENTER, GT_BOTTOM, -0.06 * P_len, 0.5 * P_hgt, 
			90.0, "ALTITUDE (KM MSL)");
	else
		G_write (Xs_bg_ov, C_WHITE, GTF_MINSTROKE, charsize,
			GT_CENTER, GT_BOTTOM, -0.1 * P_len, 0.5 * P_hgt,
			90.0, "PRESSURE (MB)");
}




void
xs_abort ()
/*
 * Abort the plot for an interrupt.  Make the overlays invisible, turn
 * off the interrupt flag, and bail out.
 */
{
	G_visible (Xs_ov, FALSE);
	G_visible (Xs_bg_ov, FALSE);
	Interrupt = FALSE;
	ui_bailout ("Cross-section plot aborted");
}




void
xs_side_text (string, color, newline)
char	*string;
int	color, newline;
/*
 * Add annotation to the right side of the plot
 * 'newline' is true if we want to start a new line
 */
{
	float	x0, y0, x1, y1;
	float	charsize = 0.02 * P_hgt;
	int	too_long;
/*
 * Find out how the string fits on the current line
 */
	G_wr_box (Xs_bg_ov, GTF_MINSTROKE, charsize, GT_LEFT, GT_TOP,
		Xtxt_pos, Ytxt_pos, -90.0, string, &x0, &y0, &x1, &y1);
/*
 * Start a new line if necessary or requested
 */
	too_long = (P_hgt > 0.0) ? (y1 < -BORDER*P_hgt) : (y1 > -BORDER*P_hgt);
	if (Ytxt_pos != P_hgt && (too_long || newline))
	{
		Ytxt_pos = P_hgt;
		Xtxt_pos -= 0.024 * P_len;
	}
/*
 * Write in the annotation
 */
	G_write (Xs_bg_ov, color, GTF_MINSTROKE, charsize, GT_LEFT, 
		GT_TOP, Xtxt_pos, Ytxt_pos, -90.0, string);
/*
 * Update the location for the next annotation
 */
	Ytxt_pos += y1 - y0;
}




void
xs_reset_annot ()
/*
 * Reset the annotation locations
 */
{
	Xtxt_pos = (1+BORDER - 0.01) * P_len;
	Ytxt_pos = P_hgt;
}



void
xs_from_to (cmds)
struct ui_command	*cmds;
/*
 * Set the endpoints of the cross-section plane
 */
{
	float	lat0, lon0, lat1, lon1;
	int	cnum = 0;
	float	snd_s_lat (), snd_s_lon ();
/*
 * First endpoint.  Look for a sounding name first, otherwise get x and y
 */
	if (cmds[cnum].uc_vptype == SYMT_STRING)
	{
		lat0 = snd_s_lat (UPTR (cmds[cnum]));
		lon0 = snd_s_lon (UPTR (cmds[cnum]));
		cvt_to_xy (lat0, lon0, &X0, &Y0);
		cnum++;
	}
	else
	{
		X0 = UFLOAT (cmds[cnum++]);
		Y0 = UFLOAT (cmds[cnum++]);
	}
/*
 * Second endpoint.  Same deal.
 */
	if (cmds[cnum].uc_vptype == SYMT_STRING)
	{
		lat1 = snd_s_lat (UPTR (cmds[cnum]));
		lon1 = snd_s_lon (UPTR (cmds[cnum]));
		cvt_to_xy (lat1, lon1, &X1, &Y1);
		cnum++;
	}
	else
	{
		X1 = UFLOAT (cmds[cnum++]);
		Y1 = UFLOAT (cmds[cnum++]);
	}
/*
 * Sanity check
 */
	if (X0 == X1 && Y0 == Y1)
		ui_error ("The endpoints must be different!");
}




void
xs_center_angle (cmds)
struct ui_command	*cmds;
/*
 * Locate the cross section plane with a center, angle, and length
 */
{
	float	xc, yc, angle, length;

	xc = UFLOAT (cmds[0]);
	yc = UFLOAT (cmds[1]);
	angle = DEG_TO_RAD (90.0 - UFLOAT (cmds[2]));
	length = UFLOAT (cmds[3]);

	if (length == 0.0)
		ui_error ("The length must be greater than zero!");

	X0 = xc - length / 2 * cos (angle);
	X1 = xc + length / 2 * cos (angle);

	Y0 = yc - length / 2 * sin (angle);
	Y1 = yc + length / 2 * sin (angle);
}




void
xs_vscale (cmds)
struct ui_command	*cmds;
/*
 * Choose the field to use for the vertical scale
 */
{
	fldtype	fld;

	fld = fd_num (UPTR (cmds[0]));

	if (fld != f_alt && fld != f_pres)
		ui_error ("Only altitude or pressure can be used!");

	Use_alt = (fld == f_alt);
/*
 * If limits were specified, deal with them now
 */
	if (cmds[1].uc_ctype != UTT_END)
		fd_set_limits (cmds);
}




void
xs_spatial ()
/*
 * Initialize for a spatial cross-section plot
 */
{
	P_len = hypot (X1 - X0, Y1 - Y0);
	Time_height = FALSE;
}




void
xs_time_height ()
/*
 * Initialize for a time-height plot
 */
{
	int	snd, sec_delta, pt, npts;
	float	*tdata;
	date	stime, diff, latest, earliest;
	date	snd_time ();
/*
 * Find the earliest and latest times from all of the soundings used
 */
	earliest.ds_yymmdd = 99991231;
	earliest.ds_hhmmss = 235959;
	latest.ds_yymmdd = 0;
	latest.ds_hhmmss = 0;

	for (snd = 0; snd < Nsnd; snd++)
	{
		stime = snd_time (S_id[snd]);
	/*
	 * See if this is the latest sounding by start time.
	 * If so, put the length of the sounding into sec_delta
	 */
		if (stime.ds_yymmdd > latest.ds_yymmdd || 
			(stime.ds_yymmdd == latest.ds_yymmdd && 
			 stime.ds_hhmmss > latest.ds_hhmmss))
		{
			latest = stime;

			if (snd_has_field (S_id[snd], f_time))
			{
				tdata = (float *) 
					malloc (BUFLEN * sizeof (float));
				npts = snd_get_data (S_id[snd], tdata, BUFLEN, 
					f_time, BADVAL);
				for (pt = npts-1; pt >= 0; pt--)
					if (tdata[pt] != BADVAL)
					{
						sec_delta = 
							(int)(tdata[pt] + 0.5);
						break;
					}

				free (tdata);
			}
			else
				sec_delta = 0;
		}
					
	/*
	 * See if this is the earliest sounding
	 */
		if (stime.ds_yymmdd < earliest.ds_yymmdd ||
			(stime.ds_yymmdd == earliest.ds_yymmdd &&
			 stime.ds_hhmmss < earliest.ds_hhmmss))
			earliest = stime;
	}
/*
 * Make sure we have a time range
 */
	if (earliest.ds_yymmdd == latest.ds_yymmdd && 
		earliest.ds_hhmmss == latest.ds_hhmmss)
		ui_error ("All soundings used are at the same time!");
/*
 * Set the plot start time
 */
	T0 = earliest;
/*
 * Find the length of the plane (the total time range in hours)
 */
	ud_sub_date (&latest, &earliest, &diff);
	P_len = (diff.ds_yymmdd % 100) * 86400 + 	/* days    */
		(diff.ds_hhmmss / 10000) * 3600 + 	/* hours   */
		((diff.ds_hhmmss / 100) % 100) * 60 + 	/* minutes */
		(diff.ds_hhmmss % 100) + sec_delta;	/* seconds */

	P_len /= 3600.0;
/*
 * Set the time-height flag
 */
	Time_height = TRUE;
}

	



void
xs_pos (sid, xpos, ypos)
char	*sid;
float	*xpos, *ypos;
/*
 * Return the x and y positions of the data for sounding 'sid'
 */
{
	float	*lat, *lon, *wspd, *wdir, *time, *dummy;
	float	ws, wd, t, dt, site_x, site_y, x, y;
	int	pt, npts;
	float	snd_s_lat (), snd_s_lon ();
/*
 * Get the site location in (x,y) coordinates
 */
	cvt_to_xy (snd_s_lat (sid), snd_s_lon (sid), &site_x, &site_y);
/*
 * Derive the (x,y) positions from (lat,lon) if possible
 */
	if (snd_has_field (sid, f_lat) && snd_has_field (sid, f_lon))
	{
	/*
	 * Lat and lon are available (can be converted directly to x,y)
	 */
		lat = (float *) malloc (BUFLEN * sizeof (float));
		lon = (float *) malloc (BUFLEN * sizeof (float));

		npts = snd_get_data (sid, lat, BUFLEN, f_lat, BADVAL);
		snd_get_data (sid, lon, BUFLEN, f_lon, BADVAL);

		for (pt = 0; pt < npts; pt++)
			if (lat[pt] == BADVAL || lon[pt] == BADVAL)
				xpos[pt] = ypos[pt] = BADVAL;
			else
			{
				cvt_to_xy (lat[pt], lon[pt], &x, &y);
				xpos[pt] = x;
				ypos[pt] = y;
			}
	/*
	 * Free the data arrays
	 */
		free (lat);
		free (lon);
	}
	else if (snd_has_field (sid, f_wspd) && snd_has_field (sid, f_wdir) && 
		snd_has_field (sid, f_time))
	{
	/*
	 * Derive the (x,y) positions from wind speed, wind direction, 
	 * and time
	 */
		wspd = (float *) malloc (BUFLEN * sizeof (float));
		wdir = (float *) malloc (BUFLEN * sizeof (float));
		time = (float *) malloc (BUFLEN * sizeof (float));

		npts = snd_get_data (sid, wspd, BUFLEN, f_wspd, BADVAL);
		snd_get_data (sid, wdir, BUFLEN, f_wdir, BADVAL);
		snd_get_data (sid, time, BUFLEN, f_time, BADVAL);

		xpos[0] = site_x;
		ypos[0] = site_y;

		ws = wd = t = dt = 0.0;

		for (pt = 0; pt < npts - 1; pt++)
		{
			if (wspd[pt] != BADVAL && wdir[pt] != BADVAL)
			{
				ws = wspd[pt] * 0.001;	/* km/s */
				wd = wdir[pt];
			}

			if (time[pt] != BADVAL)
			{
				dt = time[pt] - t;
				t = time[pt];
			}
			else
				dt = 0.0;

			xpos[pt+1] = xpos[pt] + dt * ws * 
				cos (DEG_TO_RAD (-90.0 - wd));
			ypos[pt+1] = ypos[pt] + dt * ws *
				sin (DEG_TO_RAD (-90.0 - wd));
		}
	/*
	 * Free our data arrays
	 */
		free (wspd);
		free (wdir);
		free (time);
	}
	else
	{
	/*
	 * We have no position data, just assume the sounding
	 * goes straight up
	 *
	 * Find out how many data points we need to do, then fill the 
	 * position arrays with site_x and site_y
	 */
		dummy = (float *) malloc (BUFLEN * sizeof (float));
		npts = snd_get_data (sid, dummy, BUFLEN, Fld, BADVAL);
		free (dummy);

		for (pt = 0; pt < npts; pt++)
		{
			xpos[pt] = site_x;
			ypos[pt] = site_y;
		}
	}
}




void
xs_timepos (sid, tpos)
char	*sid;
float	*tpos;
/*
 * Return the time positions of the data points for 'sid' relative to T0
 */
{
	float	*time;
	int	npts, pt, start_sec;
	date	stime, diff, snd_time ();
/*
 * Get the sounding start time and the difference from the start time
 * of the plot
 */
	stime = snd_time (sid);
	ud_sub_date (&stime, &T0, &diff);
/*
 * Convert the time difference into seconds
 */
	start_sec = (diff.ds_yymmdd % 100) * 86400 +
		(diff.ds_hhmmss / 10000) * 3600 +
		((diff.ds_hhmmss / 100) % 100) * 60 + 
		(diff.ds_hhmmss % 100);
/*
 * If there is no time data for this sounding, fill the array with the
 * start time and return
 */
	if (! snd_has_field (sid, f_time))
	{
		for (pt = 0; pt < BUFLEN; pt++)
			tpos[pt] = start_sec;

		return;
	}
/*
 * Get the time data for the sounding
 */
	time = (float *) malloc (BUFLEN * sizeof (float));	

	npts = snd_get_data (sid, time, BUFLEN, f_time, BADVAL);

	for (pt = 0; pt < npts; pt++)
		if (time[pt] != BADVAL)
			tpos[pt] = start_sec + time[pt];
		else
			tpos[pt] = BADVAL;

	free (time);

	return;
}



void
xs_special_data (sid, data, fld)
char	*sid;
float	*data;
fldtype	fld;
/*
 * Return the wind component parallel (u') or perpendicular (v') to
 * the cross-section plane
 */
{
	int	npts, i;
	float	*u, *v;
	float	plane_cos, plane_sin;
/*
 * Sanity check
 */
	if (Time_height)
		ui_error ("u_prime and v_prime only work in spatial plots!");
/*
 * Get the raw u and v wind data
 */
	u = (float *) malloc (BUFLEN * sizeof (float));
	v = (float *) malloc (BUFLEN * sizeof (float));

	npts = snd_get_data (sid, u, BUFLEN, f_u_wind, BADVAL);
	snd_get_data (sid, v, BUFLEN, f_v_wind, BADVAL);
/*
 * Find the sine and cosine of the angle of the plane
 */
	plane_cos = (X1 - X0) / hypot (X1 - X0, Y1 - Y0);
	plane_sin = (Y1 - Y0) / hypot (X1 - X0, Y1 - Y0);
/*
 * Calculate the chosen field
 */
	for (i = 0; i < npts; i++)
	{
		if (u[i] == BADVAL || v[i] == BADVAL)
			data[i] = BADVAL;
		else if (fld == f_u_prime)
			data[i] = u[i] * plane_cos + v[i] * plane_sin;
		else if (fld == f_v_prime)
			data[i] = v[i] * plane_cos - u[i] * plane_sin;
		else
			ui_error ("BUG! Shouldn't be here in xs_special_data");
	}
/*
 * Free the u and v arrays
 */
	free (u);
	free (v);
}




void
xs_extend_trace (x, y)
float	x, y;
/*
 * Add this (x,y) point to the trace for the current sounding
 */
{
	if (Tracelen >= BUFLEN)
		ui_error ("BUG! Too many points in x-section sounding trace!");

	Xtrace[Tracelen] = x;
	Ytrace[Tracelen] = y;
	Tracelen++;
}




void
xs_draw_trace (sndx)
int	sndx;
/*
 * Draw the current sounding's trace
 */
{
	char	*string;
	float	label_x, label_y, rot;
	int	i;
/*
 * Draw the trace
 */
	G_polyline (Xs_ov, GPLT_DOT, C_BG1, Tracelen, Xtrace, Ytrace);
/*
 * Find the first point of the trace which lies in the plot region
 */
	for (i = 0; i < Tracelen; i++)
		if (INPLOT (Xtrace[i], Ytrace[i]))
			break;
/*
 * Put the label at this point (or return if there isn't a point
 * in the plot region
 */
	if (i == Tracelen)
	{
		Tracelen = 0;
		return;
	}
	else
	{
		label_x = Xtrace[i];
		label_y = Ytrace[i];
	}
/*
 * Unclip for the label
 */
	G_clip_window (Xs_ov, -BORDER * P_len, -BORDER * P_hgt, 
		(1.0 + BORDER) * P_len, (1.0 + BORDER) * P_hgt);
/*
 * Rotation we use depends on whether we're towards the top or the
 * bottom of the plot
 */
	rot = (((label_y - P_bot) / P_hgt) < 0.5) ? -90.0 : 90.0;
/*
 * Draw the label
 */
	string = (char *) malloc ((2 + strlen (S_id[sndx])) * sizeof (char));
	string[0] = ' ';
	strcpyUC (string + 1, S_id[sndx]);
	G_write (Xs_ov, C_BG1, GTF_MINSTROKE, 0.02 * P_hgt, GT_LEFT, 
		GT_CENTER, label_x, label_y, rot, string);
	free (string);
/*
 * Restore the clipping
 */
	G_clip_window (Xs_ov, 0.0, 0.0, P_len, P_hgt);
/*
 * Set up to start a new trace
 */
	Tracelen = 0;
}
