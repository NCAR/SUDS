/*
 * x-y plotting module
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

static char *rcsid = "$Id: xyplot.c,v 1.5 1992-08-14 22:08:10 case Exp $";

# include <math.h>
# include <ui_param.h>
# include <ui_date.h>
# include "globals.h"
# include "color.h"
# include "fields.h"
# include "flags.h"

# define T_K	273.15
# define DEG_TO_RAD(x)	((x) * 0.017453292)
# define RAD_TO_DEG(x)	((x) * 57.29577951)
# define BETWEEN(x,lim0,lim1)	((((x)-(lim0))*((x)-(lim1))) <= 0)

/*
 * Overlay for the plot
 */
static overlay	XYplot_ov = (overlay) 0;

/*
 * X and Y fields and their limits
 */
static fldtype	Xfld, Yfld;
static float	Xmin, Xmax, Ymin, Ymax;

/*
 * Current text position for the bottom and top annotations
 */
static float	Xtxt_top, Ytxt_top;

/*
 * Prototypes
 */
# ifdef __STDC__
	void	xy_plot (struct ui_command *);
	void	xy_background (void);
	void	xy_abort (void);
	void	xy_coords (fldtype, fldtype, double, double, float *, float *);
	void	xy_ov_check (void);
	void	xy_do_plot (char *, int);
	void	xy_top_text (char *, int, int);
# else
	void	xy_plot ();
	void	xy_background ();
	void	xy_abort ();
	void	xy_coords ();
	void	xy_ov_check ();
	void	xy_do_plot ();
	void	xy_top_text ();
# endif




void
xy_plot (cmds)
struct ui_command	*cmds;
/*
 * Plot a skewt of the soundings in the command list
 */
{
	char	*id_name, string[80];
	int	plot_ndx = 0, nplots;
	float	width, height;
	char	*snd_default ();
	long	curtime;
/*
 * Make sure the graphics stuff is ready
 */
	out_ws_check ();
	xy_ov_check ();
	G_clear (XYplot_ov);

	Xtxt_top = Xmin - 0.5 * BORDER * width;
/*
 * Let edit know about this plot
 */
	edit_set_plot (xy_plot, xy_coords, cmds, &XYplot_ov, 1);
/*
 * Get the two fields and their limits
 */
	Xfld = fd_num (UPTR (*cmds++));
	Xmin = (float) fd_bot (Xfld);
	Xmax = (float) fd_top (Xfld);

	Yfld = fd_num (UPTR (*cmds++));
	Ymin = (float) fd_bot (Yfld);
	Ymax = (float) fd_top (Yfld);
/*
 * Set the plot limits and plot the background
 */
	width = Xmax - Xmin;
	height = Ymax - Ymin;

	G_set_coords (XYplot_ov, Xmin - BORDER * width, Ymin - BORDER * height,
		Xmax + BORDER * width, Ymax + BORDER * height);
	xy_background ();
/*
 * Initialize annotation position
 */
	Xtxt_top = Xmin - 0.5 * BORDER * width;
	Ytxt_top = Ymax + 0.95 * BORDER * height;
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
 * Plot title
 */
	sprintf (string, "X-Y plot of %s vs. %s     Plot generated: ", 
		fd_name (Xfld), fd_name (Yfld));
	xy_top_text (string, C_WHITE, FALSE);

	time (&curtime);
	strftime (string, sizeof (string), "%e-%b-%Y,%T", gmtime (&curtime));
	strcpyUC (string, string);
	xy_top_text (string, C_WHITE, FALSE);
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
			xy_abort ();
	/*
	 * Get the name of the next sounding (use the default if necessary)
	 */
		if (cmds->uc_ctype != UTT_END)
			id_name = UPTR (cmds[plot_ndx]);
		else
			id_name = snd_default ();
	/*
	 * Do the actual plotting
	 */
		xy_do_plot (id_name, plot_ndx);
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
xy_background ()
/*
 * Draw the background for an X-Y plot
 */
{
	float	charsize, start, step, factor, tick;
	float	height = Ymax - Ymin, width = Xmax - Xmin;
	float	x[5], y[5];
	bool	dolabel;
	char	string[64];
/*
 * Character size
 */
	charsize = 0.025 * height;
/*
 * Unclip
 */
	G_clip_window (XYplot_ov, Xmin - BORDER * width, 
		Ymin - BORDER * height, Xmax + BORDER * width, 
		Ymax + BORDER * height);
/*
 * Draw the outside rectangle
 */
	x[0] = Xmin; y[0] = Ymin;
	x[1] = Xmax; y[1] = Ymin;
	x[2] = Xmax; y[2] = Ymax;
	x[3] = Xmin; y[3] = Ymax;
	x[4] = Xmin; y[4] = Ymin;
	G_polyline (XYplot_ov, GPLT_SOLID, C_BG2, 5, x, y);
/*
 * Find the step for horizontal ticks
 */
	if (width > 0.0)
		step = pow (10.0, floor (log10 (width)));
	else
		step = -pow (10.0, floor (log10 (-width)));

	factor = width / step;
	if (factor < 1.5)
		step *= 0.1;
	else if (factor < 3.0)
		step *= 0.2;
	else if (factor < 8.0)
		step *= 0.5;
/*
 * Put on horizontal ticks, with a label on every other tick
 */

	start = step * (int)(Xmin / step);
	if (! BETWEEN (start, Xmin, Xmax))
		start += step;

	dolabel = ((int)(start / step) % 2) == 0;

	for (tick = start; BETWEEN (tick, Xmin, Xmax); tick += step)
	{
	/*
	 * Check for an interrupt
	 */
		if (Interrupt)
			xy_abort ();
	/*
	 * Draw the ticks (draw all the way across if the value is zero)
	 */
		x[0] = x[1] = tick;
		y[0] = Ymin;
		y[1] = (tick == 0.0) ? Ymax : Ymin + 0.025 * height;
		G_polyline (XYplot_ov, GPLT_SOLID, C_WHITE, 2, x, y);

		y[0] = Ymin + 0.975 * height;
		y[1] = Ymax;
		G_polyline (XYplot_ov, GPLT_SOLID, C_WHITE, 2, x, y);
	/*
	 * Label every other tick
	 */
		if (dolabel)
		{
			sprintf (string, "%.1f", tick);
			G_write (XYplot_ov, C_WHITE, GTF_DEV, charsize, 
				GT_CENTER, GT_TOP, tick, 
				Ymin - 0.01 * height, 0.0, string);
		}

		dolabel = ! dolabel;
	}

	sprintf (string, "%s (%s)", fd_desc (Xfld), fd_units (Xfld));
	G_write (XYplot_ov, C_WHITE, GTF_DEV, charsize, GT_CENTER, GT_TOP, 
		Xmin + 0.5 * width, Ymin - 0.06 * height, 0.0, string);
/*
 * Find the step for vertical ticks
 */
	if (height > 0.0)
		step = pow (10.0, floor (log10 (height)));
	else
		step = -pow (10.0, floor (log10 (-height)));

	factor = height / step;
	if (factor < 1.5)
		step *= 0.1;
	else if (factor < 3.0)
		step *= 0.2;
	else if (factor < 8.0)
		step *= 0.5;
/*
 * Put on vertical ticks, with a label on every other tick
 */
	start = step * (int)(Ymin / step);
	if (! BETWEEN (start, Ymin, Ymax))
		start += step;

	dolabel = ((int)(start / step) % 2) == 0;

	for (tick = start; BETWEEN (tick, Ymin, Ymax); tick += step)
	{
	/*
	 * Check for an interrupt
	 */
		if (Interrupt)
			xy_abort ();
	/*
	 * Draw the ticks (draw all the way across if the value is zero)
	 */
		y[0] = y[1] = tick;
		x[0] = Xmin;
		x[1] = (tick == 0.0) ? Xmax : Xmin + 0.025 * width;
		G_polyline (XYplot_ov, GPLT_SOLID, C_WHITE, 2, x, y);

		x[0] = Xmin + 0.975 * width;
		x[1] = Xmax;
		G_polyline (XYplot_ov, GPLT_SOLID, C_WHITE, 2, x, y);
	/*
	 * Label every other tick
	 */
		if (dolabel)
		{
			sprintf (string, "%.1f", tick);
			G_write (XYplot_ov, C_WHITE, GTF_DEV, charsize, 
				GT_RIGHT, GT_CENTER, Xmin - 0.01 * width, 
				tick, 0.0, string);
		}

		dolabel = ! dolabel;
	}

	sprintf (string, "%s (%s)", fd_desc (Yfld), fd_units (Yfld));
	G_write (XYplot_ov, C_WHITE, GTF_DEV, charsize, GT_CENTER, GT_BOTTOM, 
		Xmin - 0.15 * width, Ymin + 0.5 * height, 90.0, string);
}




void
xy_abort ()
{
/*
 * Abort the plot for an interrupt.  Make the overlays invisible, turn
 * off the interrupt flag, and bail out.
 */
	G_visible (XYplot_ov, FALSE);
	Interrupt = FALSE;
	ui_bailout ("X-Y plot aborted");
}



void
xy_coords (xfld, yfld, xval, yval, x, y)
fldtype	xfld, yfld;
float	xval, yval, *x, *y;
/*
 * Translate the data values to an (x,y) position in the 0.0-1.0 range 
 * w.r.t. the whole screen
 */
{
	*x = (BORDER + (xval - Xmin) / (Xmax - Xmin)) / (1.0 + 2 * BORDER);
	*y = (BORDER + (yval - Ymin) / (Ymax - Ymin)) / (1.0 + 2 * BORDER);
}




void
xy_ov_check ()
/*
 * Make sure our overlay exists and is associated with the current workstation
 */
{
/*
 * Get a new overlay if necessary
 */
	if (! XYplot_ov || G_ov_to_ws (XYplot_ov) != Wkstn)
		XYplot_ov = G_new_overlay (Wkstn, 0);
}




void
xy_do_plot (id_name, plot_ndx)
char	*id_name;
int	plot_ndx;
/*
 * Do the x-y plot for the given sounding
 */
{
	float	xpts[BUFLEN], ypts[BUFLEN];
	int	i, npts, goodpts = 0;
	char	string[32], *snd_site ();
	date	sdate, snd_time ();
/*
 * Grab the data
 */
	npts = snd_get_data (id_name, xpts, BUFLEN, Xfld, BADVAL);
	snd_get_data (id_name, ypts, BUFLEN, Yfld, BADVAL);
/*
 * Compress out bad points
 */
	for (i = 0; i < npts; i++)
	{
		if (xpts[i] == BADVAL || ypts[i] == BADVAL)
			continue;

		xpts[goodpts] = xpts[i];
		ypts[goodpts] = ypts[i];

		goodpts++;
	}
/*
 * Clip
 */
	G_clip_window (XYplot_ov, Xmin, Ymin, Xmax, Ymax);
/*
 * Draw the line
 */
	G_polyline (XYplot_ov, GPLT_SOLID, C_DATA1 + plot_ndx, goodpts, 
		xpts, ypts);
/*
 * Annotate
 */
	xy_top_text ("Site: ", C_DATA1 + plot_ndx, TRUE);
	xy_top_text (snd_site (id_name), C_DATA1 + plot_ndx, FALSE);
	xy_top_text ("  Time: ", C_DATA1 + plot_ndx, FALSE);
	sdate = snd_time (id_name);
	ud_format_date (string, &sdate, UDF_FULL);
	strcpyUC (string, string);
	xy_top_text (string, C_DATA1 + plot_ndx, FALSE);
	strcpy (string, "  (");
	strcpyUC (string + 3, id_name);
	strcat (string, ")  ");
	xy_top_text (string, C_DATA1 + plot_ndx, FALSE);
/*
 * Tell edit about this trace
 */
	edit_set_trace (id_name, Xfld, Yfld, C_DATA1 + plot_ndx);
/*
 * Done
 */
	return;
}




void
xy_top_text (string, color, newline)
char	*string;
int	color, newline;
/*
 * Add annotation to the top of the plot
 * newline is true if we want to start a new line
 */
{
	float	x0, y0, x1, y1;
	float	width = (Xmax - Xmin), height = (Ymax - Ymin);
	float	leftedge = Xmin - 0.5 * BORDER * width;
	float	rightedge = Xmax + BORDER * width;
/*
 * Set the clipping so we can annotate in the top margin
 */
	G_clip_window (XYplot_ov, leftedge, Ymax, rightedge, 
		Ymax + BORDER * height);
/*
 * Find out how the string fits on the current line
 */
	G_tx_box (XYplot_ov, GTF_DEV, 0.025 * height, GT_LEFT, GT_TOP,
		Xtxt_top, Ytxt_top, string, &x0, &y0, &x1, &y1);
/*
 * Start a new line if necessary or requested
 */
	if (Xtxt_top != leftedge && 
		(! BETWEEN (x1, leftedge, rightedge) || newline))
	{
		Xtxt_top = leftedge;
		Ytxt_top -= 0.03 * height;
	}
/*
 * Write in the annotation
 */
	G_write (XYplot_ov, color, GTF_DEV, 0.025 * height, GT_LEFT, GT_TOP, 
		Xtxt_top, Ytxt_top, 0.0, string);
/*
 * Update the location for the next annotation
 */
	Xtxt_top += x1 - x0;
/*
 * Reset the clipping
 */
	G_clip_window (XYplot_ov, Xmin, Ymin, Xmax, Ymax);
}
