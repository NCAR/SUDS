/*
 * Hodograph plotting module
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.2  89/07/11  14:51:44  burghart
 * All annotation now uses GTF_MINSTROKE font
 * 
 * Revision 1.1  89/06/26  09:57:56  burghart
 * Initial revision
 * 
 */
# include <math.h>
# include <ui_date.h>
# include "globals.h"
# include "derive.h"
# include "color.h"
# include "fields.h"

# define DEG_TO_RAD(x)	((x) * .017453292)

/*
 * Colors to use and their order
 */
static int H_color[] = { C_TRACE1, C_TRACE2, C_TRACE3, 0 };

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
	int	i, plot_ndx = 0, nplots;
	overlay ovlist[2];
	char	*snd_default ();
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
	float	wspd[BUFLEN], wdir[BUFLEN], alt[BUFLEN];
	float	x[BUFLEN], y[BUFLEN], mark_alt, site_alt, prev_alt;
	float	frac, xmark, ymark;
	float	snd_s_alt ();
	int	i, npts, goodpts;
	int	color = Colorbase + H_color[plot_ndx % 3];
	char	string[10];
/*
 * Grab the data
 */
	npts = snd_get_data (id_name, wspd, BUFLEN, f_wspd, BADVAL);
	snd_get_data (id_name, wdir, BUFLEN, f_wdir, BADVAL);
	snd_get_data (id_name, alt, BUFLEN, f_alt, BADVAL);
/*
 * Build the x and y arrays from the wind speed and direction
 * and put a mark at every 1km MSL
 */
	site_alt = snd_s_alt (id_name);
	mark_alt = 1000.0 * ((int) site_alt / 1000 + 1);

	goodpts = 0;

	for (i = 0; i < npts; i++)
	{
		if (wspd[i] == BADVAL || wdir[i] == BADVAL || alt[i] == BADVAL)
			continue;
	/*
	 * Break the wind into x and y components
	 */
		x[goodpts] = wspd[i] * cos (DEG_TO_RAD (90.0 - wdir[i]));
		y[goodpts] = wspd[i] * sin (DEG_TO_RAD (90.0 - wdir[i]));
	/*
	 * Put in a mark if we passed the mark altitude
	 */
		alt[i] += site_alt;
		if (alt[i] > mark_alt)
		{
		/*
		 * Interpolate to the mark altitude
		 */
			frac = (mark_alt - prev_alt) / (alt[i] - prev_alt);
			xmark = x[goodpts - 1] + 
				frac * (x[goodpts] - x[goodpts - 1]);
			ymark = y[goodpts - 1] + 
				frac * (y[goodpts] - y[goodpts - 1]);
		/*
		 * Draw the mark
		 */
			G_text (Hodo_ov, color, GTF_MINSTROKE, 0.05 * W_scale,
				GT_CENTER, GT_CENTER, xmark, ymark, "+");

			sprintf (string, "%.1f", mark_alt / 1000.0);
			G_text (Hodo_ov, color, GTF_MINSTROKE, 0.05 * W_scale,
				GT_LEFT, GT_CENTER, xmark + 0.02 * W_scale, 
				ymark, string);
		/*
		 * Increment the mark altitude
		 */
			mark_alt += 1000.0;
		}
	/*
	 * Save the last good altitude and increment the count of good points
	 */
		prev_alt = alt[i];
		goodpts++;
	}
/*
 * Draw the hodograph from the x and y arrays we just built
 */
	G_polyline (Hodo_ov, GPLT_SOLID, color, goodpts, x, y);
/*
 * Annotate
 */
	hd_annotate (id_name, plot_ndx);
/*
 * Tell edit about this trace
 */
	edit_set_trace (id_name, f_wspd, f_wdir, H_color[plot_ndx]);
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
	G_polyline (Hodo_bg_ov, GPLT_SOLID, Colorbase + C_BG2, 5, x, y);
/*
 * Cross through 0,0
 */
	x[0] = -W_scale;	y[0] = 0.0;
	x[1] = W_scale;		y[1] = 0.0;
	G_polyline (Hodo_bg_ov, GPLT_SOLID, Colorbase + C_BG2, 2, x, y);

	x[0] = 0.0;	y[0] = -W_scale;
	x[1] = 0.0;	y[1] = W_scale;
	G_polyline (Hodo_bg_ov, GPLT_SOLID, Colorbase + C_BG2, 2, x, y);
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
		G_polyline (Hodo_bg_ov, GPLT_SOLID, Colorbase + C_BG2, 2, 
			x, y);

		y[0] = W_scale;
		y[1] = ((tick % 5) == 0) ? 0.95 * W_scale : 0.97 * W_scale;
		G_polyline (Hodo_bg_ov, GPLT_SOLID, Colorbase + C_BG2, 2, 
			x, y);
	/*
	 * Label every fifth tick
	 */
		if ((tick % 5) == 0)
		{
			sprintf (string, "%d", (int) tick);
			G_text (Hodo_bg_ov, Colorbase + C_WHITE, 
				GTF_MINSTROKE, 0.05 * W_scale, GT_CENTER, 
				GT_TOP, (float) tick, -1.02 * W_scale, string);
		}
	}

	G_text (Hodo_bg_ov, Colorbase + C_WHITE, GTF_MINSTROKE, 0.05 * W_scale,
		GT_CENTER, GT_TOP, 0.0, -1.1 * W_scale, "X (M/S)");
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
		G_polyline (Hodo_bg_ov, GPLT_SOLID, Colorbase + C_BG2, 2, 
			x, y);

		x[0] = W_scale;
		x[1] = ((tick % 5) == 0) ? 0.95 * W_scale : 0.97 * W_scale;
		G_polyline (Hodo_bg_ov, GPLT_SOLID, Colorbase + C_BG2, 2, 
			x, y);
	/*
	 * Label every fifth tick
	 */
		if ((tick % 5) == 0)
		{
			sprintf (string, "%d", tick);
			G_text (Hodo_bg_ov, Colorbase + C_WHITE, 
				GTF_MINSTROKE, 0.05 * W_scale, GT_RIGHT, 
				GT_CENTER, -1.02 * W_scale, (float) tick, 
				string);
		}
	}

	G_text (Hodo_bg_ov, Colorbase + C_WHITE, GTF_MINSTROKE, 0.05 * W_scale,
		GT_CENTER, GT_BOTTOM, -1.15 * W_scale, 0.01 * W_scale, "Y");
	G_text (Hodo_bg_ov, Colorbase + C_WHITE, GTF_MINSTROKE, 0.05 * W_scale,
		GT_CENTER, GT_TOP, -1.15 * W_scale, -0.01 * W_scale, "(M/S)");
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
	G_text (Hodo_ov, Colorbase + color, GTF_MINSTROKE, 
		0.025 * 2 * W_scale, GT_LEFT, GT_TOP, Xtxt_top, Ytxt_top, 
		string);
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
	*x = (wspd * cos (DEG_TO_RAD(90.0-wdir)) + S_width / 2.0) / S_width;
	*y = (wspd * sin (DEG_TO_RAD(90.0-wdir)) + S_height / 2.0) / S_height;
	return;
}




void
hd_annotate (id_name, plot_ndx)
char	*id_name;
int	plot_ndx;
/*
 * Annotate the hodograph
 */
{
	char	temp[50], string[100], *site, *snd_site ();
	date	sdate, snd_time ();
/*
 * Top annotation
 */
	hd_top_text ("SITE: ", H_color[plot_ndx], TRUE);
	site = snd_site (id_name);
	hd_top_text (site, H_color[plot_ndx], FALSE);
	hd_top_text ("  TIME: ", H_color[plot_ndx], FALSE);
	sdate = snd_time (id_name);
	ud_format_date (temp, &sdate, UDF_FULL);
	strcpyUC (string, temp);
	hd_top_text (string, H_color[plot_ndx], FALSE);
	strcpy (string, "  (");
	strcpyUC (string + 3, id_name);
	strcat (string, ")");
	hd_top_text (string, H_color[plot_ndx], FALSE);
}



