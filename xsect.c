/*
 * Vertical cross-sectioning
 *
 * $Revision: 1.9 $ $Date: 1990-10-26 11:05:14 $ $Author: burghart $
 */
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
static float	*Plane, P_len, P_hgt = 12.0, P_bot = 0.0;

# define HDIM	50
# define VDIM	50

# define PDATA(i,j)	(Plane[(i*VDIM)+j])

/*
 * The endpoints of the plane for a spatial cross-section, and the starting
 * time for a time-height cross-section
 */
static float	X0 = 0.0, X1 = 0.0, Y0 = 0.0, Y1 = 0.0;
static date	T0;

/*
 * Macro to identify points in the plot area
 */
# define INPLOT(x,y)	((x)>=0.0 && (x)<=P_len && (y)>=0.0 && (y)<=P_hgt)

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
void	xs_draw_trace (), xs_center_angle ();




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
	void	contour (), contour_init ();
	overlay ovlist[2];
/*
 * Get the field to plot
 */
	Fld = fd_num (UPTR (*cmds));
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
 * Allocate space for the plane array
 */
	Plane = (float *) malloc (HDIM * VDIM * sizeof (float));
/*
 * Get the overlays ready and draw the background
 */
	G_clear (Xs_ov);
	G_set_coords (Xs_ov, -BORDER * P_len, -BORDER * P_hgt, 
		P_len * (1+BORDER), P_hgt * (1+BORDER));
	G_clip_window (Xs_ov, 0.0, 0.0, P_len, P_hgt);

	xs_background ();
/*
 * Fill the arrays with BADVALs, then put the real data in
 */
	for (i = 0; i < HDIM; i++)
		for (j = 0; j < VDIM; j++)
			PDATA (i, j) = BADVAL;

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
	return;
}




xs_put_data ()
/*
 * Fill the cross-section array with data from the chosen soundings
 */
{
	int	i, j, snd, pt, npts, ok, ngood, level;
	float	zstep, w, fval[VDIM], weight[VDIM], hdis[VDIM];
	float	*fdata, *sval, *spos;
	float	*xpos, *ypos, *zpos, *tpos;
	float	site_alt, plane_ang, val, x, y, dis, hlen, pt_ang;
	float	snd_s_alt (), spline_eval ();
/*
 * Array allocation
 */
	fdata = (float *) malloc (BUFLEN * sizeof (float));
	xpos = tpos = (float *) malloc (BUFLEN * sizeof (float));
	ypos = (float *) malloc (BUFLEN * sizeof (float));
	zpos = (float *) malloc (BUFLEN * sizeof (float));
/*
 * Get the angle of the plane
 */
	if (! Time_height)
		plane_ang = atan2 (Y1 - Y0, X1 - X0);
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
	 * Get the vertical (altitude or pressure) data
	 */
		if (Use_alt)
		{
		/*
		 * Get the altitude and convert to km MSL
		 */
			npts = snd_get_data (S_id[snd], zpos, BUFLEN, f_alt, 
				BADVAL);
			site_alt = snd_s_alt (S_id[snd]);
			for (pt = 0; pt < npts; pt++)
				if (zpos[pt] != BADVAL)
				{
					zpos[pt] += site_alt;
					zpos[pt] *= 0.001;
				}
		}
		else
		/*
		 * Get the pressure data
		 */
			npts = snd_get_data (S_id[snd], zpos, BUFLEN, f_pres,
				BADVAL);
	/*
	 * We want the vertical data to be relative to the bottom 
	 * value of the plane
	 */
		for (pt = 0; pt < npts; pt++)
			if (zpos[pt] != BADVAL)
				zpos[pt] -= P_bot;
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
	 * Initialize the value and weight arrays to zero
	 */
		for (j = 0; j < VDIM; j++)
		{
			fval[j] = 0.0;
			weight[j] = 0.0;
		}
	/*
	 * Loop through the points
	 */
		for (pt = 0; pt < npts; pt++)
		{
		/*
		 * Bag this point if the datum or position is bad
		 */
			if (fdata[pt] == BADVAL || xpos[pt] == BADVAL ||
				ypos[pt] == BADVAL || zpos[pt] == BADVAL)
				continue;
		/*
		 * Project this data point onto the plane, finding the 
		 * horizontal distance along the plane and the distance
		 * from the point to the plane
		 */
			if (Time_height)
			{
				hlen = tpos[pt] / 3600.0; /* convert to hrs */
				dis = 0.0;
			}
			else
			{
				x = xpos[pt];
				y = ypos[pt];

				if (x != 0.0 || y != 0.0)
					pt_ang = atan2 (y, x) - plane_ang;
				else
					pt_ang = 0.0;

				hlen = hypot (x, y) * cos (pt_ang);
				dis = fabs (hypot (x, y) * sin (pt_ang));
			}
		/*
		 * Add a point to the trace for this sounding
		 */
			xs_extend_trace (hlen, zpos[pt]);
		/*
		 * Find the closest vertical grid level to the z position
		 * of this point
		 */
			level = (int)(zpos[pt] / zstep + 0.5);
		/*
		 * If the grid level is reasonable, add this point to
		 * the weighted average for the level
		 */
			if (level >= 0 && level < VDIM)
			{
				w = 1.0 - 2.0 * 
					fabs (zpos[pt] - level*zstep) / zstep;

				fval[level] = (fval[level] * weight[level] +
					fdata[pt] * w) / (weight[level] + w);
				hdis[level] = (hdis[level] * weight[level] +
					hlen * w) / (weight[level] + w);
				weight[level] += w;
			}
		}
	/*
	 * Move the weighted averages into the grid
	 */
		for (j = 0; j < VDIM; j++)
		{
			if (weight[j] == 0.0)
				continue;
		/*
		 * Find our horizontal grid location
		 */
			i = (int)((HDIM - 1) * hdis[j] / P_len + 0.5);
		/*
		 * Put the value in the grid
		 */
			if (i >= 0 && i < HDIM)
				PDATA (i, j) = fval[j];
		}
	/*
	 * Draw the trace for this sounding
	 */
		xs_draw_trace (snd);
	}
/*
 * We have the "raw" data in the array, now apply splines horizontally
 * to fill in missing data areas
 */
	sval = (float *) malloc (MAX (HDIM, VDIM) * sizeof (float));
	spos = (float *) malloc (MAX (HDIM, VDIM) * sizeof (float));

	for (j = 0; j < VDIM; j++)
	{
		ngood = 0;
	/*
	 * Check for an interrupt
	 */
		if (Interrupt)
			xs_abort ();
	/*
	 * Build vectors of good data points and their positions in this row
	 */
		for (i = 0; i < HDIM; i++)
		{
			if (PDATA (i,j) != BADVAL)
			{
				sval[ngood] = PDATA (i,j);
				spos[ngood] = (float) i;
				ngood++;
			}
		}
	/*
	 * Don't do the spline if we don't have enough points
	 */
		if (ngood < 2)
			continue;
	/*
	 * Do the cubic spline fit for this row
	 */
		spline (spos, sval, ngood);
	/*
	 * Evaluate the spline at the bad value points in this row
	 */
		for (i = 0; i < HDIM; i++)
		{
			if (PDATA (i,j) == BADVAL)
			{
				val = spline_eval ((float) i, &ok);
				if (ok)
					PDATA (i,j) = val;
			}
		}
	}
/*
 * Repeat the above steps to interpolate vertically
 */
	for (i = 0; i < HDIM; i++)
	{
		ngood = 0;
	/*
	 * Check for an interrupt
	 */
		if (Interrupt)
			xs_abort ();
	/*
	 * Build vectors of good data points and their positions in this column
	 */
		for (j = 0; j < VDIM; j++)
		{
			if (PDATA (i,j) != BADVAL)
			{
				sval[ngood] = PDATA (i,j);
				spos[ngood] = (float) j;
				ngood++;
			}
		}
	/*
	 * Don't do the spline if we don't have enough points
	 */
		if (ngood < 2)
			continue;
	/*
	 * Do the cubic spline fit for this column
	 */
		spline (spos, sval, ngood);
	/*
	 * Evaluate the spline at the bad value points in this row
	 */
		for (j = 0; j < VDIM; j++)
		{
			if (PDATA (i,j) == BADVAL)
			{
				val = spline_eval ((float) j, &ok);
				if (ok)
					PDATA (i,j) = val;
			}
		}
	}
/*
 * The plane array is populated, free the allocated memory and return
 */
	free (fdata);
	free (xpos);
	free (ypos);
	free (zpos);
	free (sval);
	free (spos);

	return;
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
 * Get the lower and upper limits of the vertical axes
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

	ui_printf ("center (%.1f,%.1f), angle %.1f, length %.1f", xc, yc,
		UFLOAT (cmds[2]), length);

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
	if (fld == f_alt)
	{
		Use_alt = TRUE;
	/*
	 * Default bottom and height for the plane (in km MSL)
	 */
		P_bot = 0.0;
		P_hgt = 12.0;
	}
	else if (fld == f_pres)
	{
		Use_alt = FALSE;
	/*
	 * Default bottom and height for the plane (in mb)
	 */
		P_bot = 1000.0;
		P_hgt = 800.0;
	}
	else
		ui_error ("Only altitude or pressure can be used!");
/*
 * See if the user has specified vertical limits
 */
	if (cmds[1].uc_ctype != UTT_END)
	{
		P_bot = UFLOAT (cmds[1]);
		P_hgt = UFLOAT (cmds[2]) - P_bot;
	}
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
 * Return the x and y positions of the data for sounding 'sid' relative 
 * to X0,Y0
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

	site_x -= X0;
	site_y -= Y0;
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
				xpos[pt] = x - X0;
				ypos[pt] = y - Y0;
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
	float	label_x, label_y;
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
 * Draw the label
 */
	string = (char *) malloc ((2 + strlen (S_id[sndx])) * sizeof (char));
	string[0] = ' ';
	strcpyUC (string + 1, S_id[sndx]);
	G_write (Xs_ov, C_BG1, GTF_MINSTROKE, 0.02 * P_hgt, GT_LEFT, 
		GT_CENTER, label_x, label_y, -90.0, string);
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
