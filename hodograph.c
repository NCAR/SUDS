/*
 * Hodograph plotting module
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

static char *rcsid = "$Id: hodograph.c,v 1.18 1998-02-13 18:29:46 burghart Exp $";

# include <math.h>
# include <string.h>
# include <time.h>

# include <ui_param.h>
# include <ui_date.h>
# include <met_formulas.h>
# include "globals.h"
# include "color.h"
# include "fields.h"
# include "keywords.h"
# include "flags.h"

# define DEG_TO_RAD(x)	((x) * .017453292)

/*
 * Overlays for the background and the data
 */
static overlay	Hodo_bg_ov, Hodo_ov;

/*
 * Screen size in hodograph coordinates
 */
static float	S_width, S_height;

/*
 * Current text position for the bottom and top annotations
 */
static float	Xtxt_top, Ytxt_top;

/*
 * Mark increment and top altitude
 */
static int	HodoMarkInc;
static int	Top_alt;

/*
 * ``Stepped'' plot?
 */
static int	HodoStepInc;
static char	Stepping;

/*
 * Forward declarations
 */
void	hd_do_winds (), hd_background (), hd_abort (), hd_top_text (); 
void	hd_reset_annot (), hd_ov_check (), hd_coords (), hd_annotate ();





void
hd_plot (cmds)
struct ui_command	*cmds;
/*
 * Plot a skewt of the soundings in the command list
 */
{
	char	*id_name;
	int	plot_ndx = 0, nplots;
	overlay ovlist[2];
	char	*snd_default ();
/*
 * Initialize
 */
	HodoMarkInc = 1000;
	Top_alt = 20000;
	Stepping = FALSE;
/*
 * Handle the qualifiers
 */
	for (; cmds[0].uc_ctype == UTT_KW; cmds++)
	{
		switch (UKEY (cmds[0]))
		{
		    case KW_MARK:
			cmds++;
			HodoMarkInc = (int)(UFLOAT (cmds[0]) * 1000);
			break;
		    case KW_STEP:
			Stepping = TRUE;
			cmds++;
			HodoStepInc = (int)(UFLOAT (cmds[0]) * 1000);
			break;
		    case KW_TOP:
			cmds++;
			Top_alt = (int)(UFLOAT (cmds[0]) * 1000);
			break;
		    default:
			ui_error ("BUG! Can't handle keyword %d in hd_plot!",
				UKEY (cmds[0]));
		}
	}
/*
 * Make sure the graphics stuff is ready
 */
	out_ws_check ();
	hd_ov_check ();
/*
 * Let edit know about this plot
 */
	ovlist[0] = Hodo_ov;
	ovlist[1] = Hodo_bg_ov;
	edit_set_plot (hd_plot, hd_coords, cmds, ovlist, 2);
/*
 * Get the data overlay ready
 */
	G_clear (Hodo_ov);
	G_clip_window (Hodo_ov, -W_scale, -W_scale, W_scale, W_scale);
/*
 * Plot the background
 */
	hd_background ();
/*
 * Reset the annotation position
 */
	hd_reset_annot ();
/*
 * Count how many soundings we're plotting.  Set it to one if no soundings
 * were specified, and don't plot more than three.
 */
	for (nplots = 0; cmds[nplots].uc_ctype != UTT_END; nplots++)
		/* nothing */;

	if (! nplots)
		nplots = 1;
	else if (nplots > 3)
		nplots = 3;
/*
 * Loop until we plot all of the requested soundings
 * (Plot the default sounding if none were requested)
 */
	for (plot_ndx = 0; plot_ndx < nplots; plot_ndx++)
	{
	/*
	 * Check for an interrupt
	 */
		if (Interrupt)
			hd_abort ();
	/*
	 * Get the name of the next sounding (use the default if necessary)
	 */
		if (cmds->uc_ctype != UTT_END)
			id_name = UPTR (cmds[plot_ndx]);
		else
			id_name = snd_default ();
	/*
	 * Plot the data
	 */
		hd_do_winds (id_name, plot_ndx);
	}
/*
 * Update the display
 */
	G_update (Wkstn);
/*
 * Make the last sounding our default
 */
	snd_set_default (id_name);
}




void
hd_do_winds (id_name, plot_ndx)
char	*id_name;
int	plot_ndx;
/*
 * Draw the actual data part of the hodograph
 */
{
	float	u[BUFLEN], v[BUFLEN], udraw[BUFLEN], vdraw[BUFLEN];
	float	alt[BUFLEN], altdraw[BUFLEN], mark_alt, next_alt;
	float	site_alt, frac, xmark, ymark;
	float	snd_s_alt ();
	int	i, npts, drawpts, ndx;
	int	color = C_DATA1 + plot_ndx;
	char	string[10];
/*
 * Grab the data
 */
	npts = snd_get_data (id_name, u, BUFLEN, f_u_wind, BADVAL);
	snd_get_data (id_name, v, BUFLEN, f_v_wind, BADVAL);
	snd_get_data (id_name, alt, BUFLEN, f_alt, BADVAL);

	site_alt = snd_s_alt (id_name);
/*
 * Compress out levels with any bad data
 */
	ndx = 0;
	for (i = 0; i < npts; i++)
	{
		if (u[i] == BADVAL || v[i] == BADVAL || alt[i] == BADVAL)
			continue;

		u[ndx] = u[i];
		v[ndx] = v[i];
		alt[ndx] = alt[i];
		ndx++;
	}

	npts = ndx;
/*
 * Convert altitudes to AGL if necessary
 */
	if (! Flg_hodo_msl)
		for (i = 0; i < npts; i++)
			alt[i] -= site_alt;
/*
 * Build the arrays of points to draw.  If we are not stepping, then we
 * just use every point up to Top_alt.  Otherwise we interpolate to the
 * chosen step increments (overwriting points in the existing data arrays)
 */
	if (!Stepping)
	{
	    for (i = 0; i < npts && alt[i] <= Top_alt; i++)
	    {
		udraw[i] = u[i];
		vdraw[i] = v[i];
		altdraw[i] = alt[i];
		drawpts = i;
	    }
	}
	else
	{
	/*
	 * Always use the first point when stepping
	 */
	    udraw[0] = u[0];
	    vdraw[0] = v[0];
	    altdraw[0] = alt[0];
	    drawpts = 1;
	/*
	 * What's the next altitude to draw?
	 */
	    next_alt = Flg_hodo_msl ? 
		(float)HodoStepInc * ((int) site_alt / HodoStepInc + 1) : 0.0;
	/*
	 * Run through the remaining points, stepping next_alt as necessary
	 */
	    for (i = 1; i < npts && next_alt <= Top_alt; i++)
	    {
		while (alt[i] > next_alt)
		{
		/*
		 * Interpolate to the step altitude
		 */
		    frac = (next_alt - alt[i-1]) / (alt[i] - alt[i-1]);
		    udraw[drawpts] = u[i-1] + frac * (u[i] - u[i-1]);
		    vdraw[drawpts] = v[i-1] + frac * (v[i] - v[i-1]);
		    altdraw[drawpts] = next_alt;
		    drawpts++;
			
		    next_alt += (float) HodoStepInc;
		}
	    }
	}
	    
/*
 * Draw the hodograph
 */
	G_polyline (Hodo_ov, GPLT_SOLID, color, drawpts, udraw, vdraw);
/*
 * Find the first mark altitude
 */
	mark_alt = Flg_hodo_msl ? 
	    (float)(HodoMarkInc) * ((int) site_alt / HodoMarkInc + 1) : 0.0;
/*
 * Put in the altitude marks, stepping by the mark increment
 */
	for (i = 1; i < drawpts && (int) mark_alt <= Top_alt; i++)
	{
	/*
	 * Put in a mark if we passed the mark altitude
	 */
		while (altdraw[i] > mark_alt)
		{
		/*
		 * Interpolate to the mark altitude
		 */
			frac = (mark_alt - altdraw[i-1]) / 
			    (altdraw[i] - altdraw[i-1]);
			xmark = udraw[i-1] + frac * (udraw[i] - udraw[i-1]);
			ymark = vdraw[i-1] + frac * (vdraw[i] - vdraw[i-1]);
		/*
		 * Draw the mark
		 */
			G_write (Hodo_ov, color, GTF_MINSTROKE, 0.05 * W_scale,
				 GT_CENTER, GT_CENTER, xmark, ymark, 0.0, "+");

			sprintf (string, "%.1f", mark_alt / 1000.0);
			G_write (Hodo_ov, color, GTF_DEV, 0.05 * W_scale,
				 GT_LEFT, GT_CENTER, xmark + 0.02 * W_scale, 
				 ymark, 0.0, string);
		/*
		 * Increment the mark altitude
		 */
			mark_alt += (float) HodoMarkInc;
		}
	}
/*
 * Annotate
 */
	hd_annotate (id_name, color);
/*
 * Tell edit about this trace if it's not a stepped plot
 */
	if (! Stepping)
		edit_set_trace (id_name, f_wspd, f_wdir, color);
/*
 * Done
 */
	return;
}	




void
hd_background ()
/*
 * Draw the background for the hodograph
 */
{
	int	max = (int) W_scale, tick;
	float	x[5], y[5];
	char	string[10];

	G_clear (Hodo_bg_ov);
/*
 * Unclip so we can annotate outside the border
 */
	G_clip_window (Hodo_bg_ov, -1.25 * W_scale, -1.25 * W_scale, 
		1.25 * W_scale, 1.25 * W_scale);
/*
 * Draw the rectangle
 */
	x[0] = -W_scale;	y[0] = -W_scale;
	x[1] = W_scale;		y[1] = -W_scale;
	x[2] = W_scale;		y[2] = W_scale;
	x[3] = -W_scale;	y[3] = W_scale;
	x[4] = -W_scale;	y[4] = -W_scale;
	G_polyline (Hodo_bg_ov, GPLT_SOLID, C_BG2, 5, x, y);
/*
 * Cross through 0,0
 */
	x[0] = -W_scale;	y[0] = 0.0;
	x[1] = W_scale;		y[1] = 0.0;
	G_polyline (Hodo_bg_ov, GPLT_SOLID, C_BG2, 2, x, y);

	x[0] = 0.0;	y[0] = -W_scale;
	x[1] = 0.0;	y[1] = W_scale;
	G_polyline (Hodo_bg_ov, GPLT_SOLID, C_BG2, 2, x, y);
/*
 * Label the horizontal axes
 */
	for (tick = -max; tick <= max; tick++)
	{
	/*
	 * Check for an interrupt
	 */
		if (Interrupt)
			hd_abort ();
	/*
	 * Draw the tick
	 */
		x[0] = x[1] = (float) tick;
		y[0] = -W_scale;
		y[1] = ((tick % 5) == 0) ? -0.95 * W_scale : -0.97 * W_scale;
		G_polyline (Hodo_bg_ov, GPLT_SOLID, C_BG2, 2, x, y);

		y[0] = W_scale;
		y[1] = ((tick % 5) == 0) ? 0.95 * W_scale : 0.97 * W_scale;
		G_polyline (Hodo_bg_ov, GPLT_SOLID, C_BG2, 2, x, y);
	/*
	 * Label every fifth tick
	 */
		if ((tick % 5) == 0)
		{
			sprintf (string, "%d", (int) tick);
			G_write (Hodo_bg_ov, C_WHITE, GTF_DEV, 
				0.05 * W_scale, GT_CENTER, GT_TOP, 
				(float) tick, -1.02 * W_scale, 0.0, string);
		}
	}

	G_write (Hodo_bg_ov, C_WHITE, GTF_MINSTROKE, 0.05 * W_scale,
		GT_CENTER, GT_TOP, 0.0, -1.1 * W_scale, 0.0, "U (M/S)");
/*
 * Label the vertical axes
 */
	for (tick = -max; tick <= max; tick++)
	{
	/*
	 * Check for an interrupt
	 */
		if (Interrupt)
			hd_abort ();
	/*
	 * Draw the ticks
	 */
		y[0] = y[1] = (float) tick;
		x[0] = -W_scale;
		x[1] = ((tick % 5) == 0) ? -0.95 * W_scale : -0.97 * W_scale;
		G_polyline (Hodo_bg_ov, GPLT_SOLID, C_BG2, 2, x, y);

		x[0] = W_scale;
		x[1] = ((tick % 5) == 0) ? 0.95 * W_scale : 0.97 * W_scale;
		G_polyline (Hodo_bg_ov, GPLT_SOLID, C_BG2, 2, x, y);
	/*
	 * Label every fifth tick
	 */
		if ((tick % 5) == 0)
		{
			sprintf (string, "%d", tick);
			G_write (Hodo_bg_ov, C_WHITE, GTF_DEV, 
				0.05 * W_scale, GT_RIGHT, GT_CENTER, 
				-1.02 * W_scale, (float) tick, 0.0, string);
		}
	}

	G_write (Hodo_bg_ov, C_WHITE, GTF_MINSTROKE, 0.05 * W_scale, GT_CENTER,
		GT_BOTTOM, -1.1 * W_scale, 0.0, 90.0, "V (M/S)");
/*
 * Restore the clipping
 */
	G_clip_window (Hodo_bg_ov, -W_scale, -W_scale, W_scale, W_scale);
}




void
hd_abort ()
/*
 * Abort the plot for an interrupt.  Make the overlays invisible, turn
 * off the interrupt flag, and bail out.
 */
{
	G_visible (Hodo_ov, FALSE);
	G_visible (Hodo_bg_ov, FALSE);
	Interrupt = FALSE;
	ui_bailout ("Hodograph plot aborted");
}




void
hd_top_text (string, color, newline)
char	*string;
int	color, newline;
/*
 * Add annotation to the bottom of the plot
 * newline is true if we want to start a new line
 */
{
	float	x0, y0, x1, y1;
/*
 * Set the clipping so we can annotate in the top margin
 */
	G_clip_window (Hodo_ov, -1.25 * W_scale, W_scale, 1.25 * W_scale, 
		1.25 * W_scale);
/*
 * Find out how the string fits on the current line
 */
	G_tx_box (Hodo_ov, GTF_MINSTROKE, 0.025 * 2 * W_scale, GT_LEFT, GT_TOP,
		Xtxt_top, Ytxt_top, string, &x0, &y0, &x1, &y1);
/*
 * Start a new line if necessary or requested
 */
	if (Xtxt_top > -W_scale && (x1 > 1.25 * W_scale || newline))
	{
		Xtxt_top = -W_scale;
		Ytxt_top -= 0.03 * 2 * W_scale;
	}
/*
 * Write in the annotation
 */
	G_write (Hodo_ov, color, GTF_DEV, 0.025 * 2 * W_scale, GT_LEFT, 
		GT_TOP, Xtxt_top, Ytxt_top, 0.0, string);
/*
 * Update the location for the next annotation
 */
	Xtxt_top += x1 - x0;
/*
 * Reset the clipping
 */
	G_clip_window (Hodo_ov, -W_scale, -W_scale, W_scale, W_scale);
}



void
hd_reset_annot ()
/*
 * Reset the annotation locations
 */
{
	Xtxt_top = -W_scale;
	Ytxt_top = 1.24 * W_scale;
}



void
hd_ov_check ()
/*
 * Make sure our overlays exist and are associated with the 
 * current workstation
 */
{
	int	xres, yres;
/*
 * Find the screen aspect ratio so we get a square plot
 */
	G_w_inquire (Wkstn, GIW_XRES, &xres);
	G_w_inquire (Wkstn, GIW_YRES, &yres);
	if (xres > yres)
	{
		S_height = 2.5 * W_scale;
		S_width = (float) xres / (float) yres * S_height;
	}
	else
	{
		S_width = 2.5 * W_scale;
		S_height = (float) yres / (float) xres * S_width;
	}
/*
 * Do we have an overlay for the background?
 */
	if (! Hodo_bg_ov ||  G_ov_to_ws (Hodo_bg_ov) != Wkstn)
		Hodo_bg_ov = G_new_overlay (Wkstn, 0);

	G_set_coords (Hodo_bg_ov, -0.5 * S_width, -0.5 * S_height, 
		0.5 * S_width, 0.5 * S_height);
/*
 * Do we have an overlay for the data?
 */
	if (! Hodo_ov || G_ov_to_ws (Hodo_ov) != Wkstn) 
		Hodo_ov = G_new_overlay (Wkstn, 0);

	G_set_coords (Hodo_ov, -0.5 * S_width, -0.5 * S_height, 
		0.5 * S_width, 0.5 * S_height);

	return;
}




void
hd_coords (a, b, wspd, wdir, x, y)
fldtype	a, b;
float	wspd, wdir, *x, *y;
/*
 * Translate the data values to an (x,y) position in the 0.0-1.0 range 
 * w.r.t. the whole screen.  We known a and b will always be f_wspd
 * and f_wdir, respectively, so they are ignored.
 */
{
	*x = (wspd * cos (DEG_TO_RAD(270.0-wdir)) + S_width / 2.0) / S_width;
	*y = (wspd * sin (DEG_TO_RAD(270.0-wdir)) + S_height / 2.0) / S_height;
	return;
}




void
hd_annotate (id_name, color)
char	*id_name;
int	color;
/*
 * Annotate the hodograph
 */
{
	char	temp[50], string[100], *site, *snd_site ();
	date	sdate, snd_time ();
	long	curtime;
/*
 * Title
 */
	hd_top_text ("HODOGRAPH ", C_WHITE, TRUE);
	hd_top_text (
		Flg_hodo_msl ? "(ALTITUDES KM MSL)" : "(ALTITUDES KM AGL)", 
		C_WHITE, FALSE);
/*
 * Site name
 */
	hd_top_text ("SITE: ", C_WHITE, TRUE);
	site = snd_site (id_name);
	hd_top_text (site, C_WHITE, FALSE);
/*
 * Data time
 */
	hd_top_text ("  TIME: ", C_WHITE, FALSE);
	sdate = snd_time (id_name);
	ud_format_date (temp, &sdate, UDF_FULL);
	strcpyUC (string, temp);
	hd_top_text (string, C_WHITE, FALSE);
/*
 * Sounding ID
 */
	strcpy (string, "  (");
	strcpyUC (string + 3, id_name);
	strcat (string, ")");
	hd_top_text (string, color, FALSE);
/*
 * Plot generation time
 */
	hd_top_text ("PLOT GENERATED: ", C_WHITE, TRUE);
	time (&curtime);
	strftime (string, sizeof (string), "%e-%b-%Y,%T", gmtime (&curtime));
	strcpyUC (string, string);
	hd_top_text (string, C_WHITE, FALSE);
}
