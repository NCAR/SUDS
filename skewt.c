/*
 * Skew-t plotting module
 *
 * $Revision: 1.6 $ $Date: 1989-08-14 15:08:54 $ $Author: burghart $
 */
# include <math.h>
# include <ui_date.h>
# include "globals.h"
# include "derive.h"
# include "color.h"
# include "fields.h"
# include "keywords.h"
# include "flags.h"

# define T_K	273.15
# define DEG_TO_RAD(x)	((x) * .017453292)

/*
 * Plot limits (initialized in skt_init below)
 */
static float	Pmin, Pmax;
static int	Pstep;
static float	Tmin, Tmax;
static int	Tstep;

/*
 * Slope of the isotherms
 */
# define SKEWSLOPE	1.0

/*
 * Mapping functions from pres to y and from (temp,y) to x
 */
# define YPOS(p)	(log(p/Pmax) / log(Pmin/Pmax))
# define XPOS(t,y)	((t-Tmin)/(Tmax-Tmin) + y/SKEWSLOPE)

/*
 * Colors to use for temperature and dewpoint and their order
 */
static int Temp_color[] = { C_TRACE1, C_TRACE3, C_TRACE5, 0 };
static int Dp_color[] = { C_TRACE2, C_TRACE4, C_TRACE6, 0 };

/*
 * Overlays for the skew-t background and the data
 */
static overlay	Skewt_bg_ov, Skewt_ov, Winds_ov;

/*
 * True aspect ratio for the rectangle in the winds overlay with
 * corners at (0.0,0.0) (1.0,1.0) in world coordinates
 */
static float	W_aspect;

/*
 * Current text position for the bottom and top annotations
 */
static float	Xtxt_bot, Ytxt_bot, Xtxt_top, Ytxt_top;

/*
 * Do we need to draw a new background?
 */
static int	Redraw = TRUE;

/*
 * Forward declarations
 */
void	skt_skewt (), skt_abort (), skt_background (), skt_bot_text ();
void	skt_reset_annot (), skt_annotate (), skt_top_text (), skt_plot ();
void	skt_ov_check (), skt_coords (), skt_lift (), skt_thermo ();
void	skt_winds ();




void
skt_handler (cmds)
struct ui_command	*cmds;
/*
 * UI dispatch handler for skewt commands
 */
{
/*
 * Command switch table
 */
	if (cmds->uc_ctype == UTT_KW)
	{
	    switch (UKEY (cmds[0]))
	    {
		case KW_PLIMITS:
			skt_limits (cmds);
			return;
		case KW_TLIMITS:
			skt_limits (cmds);
			return;
		default:
			for (;; cmds++)
			{
				main_dump_cmd (cmds);
				if (cmds->uc_ctype == UTT_END)
					return;
			}
	    }
	}
/*
 * No command.  Assume it's a plot command
 */
	else
	{
		skt_plot (cmds);
		return;
	}
}




void
skt_plot (cmds)
struct ui_command	*cmds;
/*
 * Plot a skewt of the soundings in the command list
 */
{
	char	*id_name;
	int	plot_ndx = 0, nplots;
	overlay ovlist[3];
	char	*snd_default ();
/*
 * Make sure the graphics stuff is ready
 */
	out_ws_check ();
	skt_ov_check ();
/*
 * Let edit know about this plot
 */
	ovlist[0] = Skewt_ov;
	ovlist[1] = Skewt_bg_ov;
	ovlist[2] = Winds_ov;
	edit_set_plot (skt_plot, skt_coords, cmds, ovlist, 3);
/*
 * Get the skew-t and winds overlays ready
 */
	G_clear (Skewt_ov);
	G_clip_window (Skewt_ov, 0.0, 0.0, 1.0, 1.0);
	G_clear (Winds_ov);
/*
 * Plot the background
 */
	skt_background ();
/*
 * Reset the annotation position
 */
	skt_reset_annot ();
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
			skt_abort ();
	/*
	 * Get the name of the next sounding (use the default if necessary)
	 */
		if (cmds->uc_ctype != UTT_END)
			id_name = UPTR (cmds[plot_ndx]);
		else
			id_name = snd_default ();
	/*
	 * Plot the thermo data and the winds data if requested
	 */
		skt_thermo (id_name, plot_ndx);
		if (Flg_winds)
			skt_winds (id_name, plot_ndx, nplots);
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
skt_background ()
/*
 * Draw the background for a skew T, log p diagram
 */
{
	int	i;
	float	x[128], y[128];
	char	string[64];
	float	t, pt, p;
	static float	mr[] = {0.1, 0.2, 0.4, 1.0, 2.0, 3.0, 5.0, 8.0, 
			12.0, 20.0, 0.0};
/*
 * Return if our old background is still good
 */
	if (! Redraw)
		return;
/*
 * Clear the overlay and get the clipping right
 */
	G_clear (Skewt_bg_ov);
	G_clip_window (Skewt_bg_ov, 0.0, 0.0, 1.0, 1.0);
/*
 * Draw the outside rectangle
 */
	x[0] = 0.0; y[0] = 0.0;
	x[1] = 1.0; y[1] = 0.0;
	x[2] = 1.0; y[2] = 1.0;
	x[3] = 0.0; y[3] = 1.0;
	x[4] = 0.0; y[4] = 0.0;
	G_polyline (Skewt_bg_ov, GPLT_SOLID, Colorbase + C_BG2, 5, x, y);
/*
 * Isotherms
 */
	y[0] = 0.0;
	y[1] = 1.0;
	for (t = -120; t <= 50; t += Tstep)
	{
	/*
	 * Check for an interrupt
	 */
		if (Interrupt)
			skt_abort ();
	/*
	 * Draw the isotherm
	 */
		x[0] = XPOS (t, y[0]);
		x[1] = XPOS (t, y[1]);
		if (x[1] < 0.0)
			continue;

		G_polyline (Skewt_bg_ov, GPLT_SOLID, Colorbase + C_BG2, 
			2, x, y);
	/*
	 * Unclip so we can annotate outside the border
	 */
		G_clip_window (Skewt_bg_ov, -BORDER, -BORDER, 
			1.0 + 2 * BORDER, 1.0 + BORDER);
	/*
	 * Write the number either on the top or on the right side depending
	 * on the isotherm
	 */
		sprintf (string, "%d\0", (int) t);

		if (x[1] <= 1.0)
			G_text (Skewt_bg_ov, Colorbase + C_WHITE, 
				GTF_MINSTROKE, 0.025, GT_CENTER, GT_BOTTOM, 
				x[1], 1.01, string);
		else
		{
			float	intercept = SKEWSLOPE * (1.0 - x[0]);
			if (intercept > 0.0)
				G_text (Skewt_bg_ov, Colorbase + C_WHITE, 
					GTF_MINSTROKE, 0.025, GT_LEFT, 
					GT_CENTER, 1.01, intercept, string);
		}
		G_clip_window (Skewt_bg_ov, 0.0, 0.0, 1.0, 1.0);
	}
/*
 * Isobars
 */
	G_clip_window (Skewt_bg_ov, -BORDER, -BORDER, 1.0 + 2 * BORDER, 
		1.0 + BORDER);
	x[0] = 0.0;
	x[1] = 1.0;
	for (p = Pmax; p >= Pmin; 
		p -= ((int) p % Pstep) ? ((int) p % Pstep) : Pstep)
	{
	/*
	 * Check for an interrupt
	 */
		if (Interrupt)
			skt_abort ();
	/*
	 * Draw the isobar
	 */
		y[0] = y[1] = YPOS (p);

		G_polyline (Skewt_bg_ov, GPLT_SOLID, Colorbase + C_BG2, 
			2, x, y);
	/*
	 * Annotate along the left side
	 */
		sprintf (string, "%d\0", (int) p);
		G_text (Skewt_bg_ov, Colorbase + C_WHITE, GTF_MINSTROKE, 
			0.025, GT_RIGHT, GT_CENTER, -0.01, y[0], string);
	}
/*
 * Saturation mixing ratio lines
 */
	G_clip_window (Skewt_bg_ov, 0.0, 0.0, 1.0, 1.0);
	y[0] = 0.0;
	y[1] = YPOS ((Pmin + Pmax) / 2.0);
	for (i = 0; mr[i] > 0.0; i++)
	{
	/*
	 * Check for an interrupt
	 */
		if (Interrupt)
			skt_abort ();
	/*
	 * The lines go from Pmax to (Pmax + Pmin) / 2
	 */
		x[0] = XPOS (t_mr (Pmax, mr[i]) - T_K, y[0]);
		x[1] = XPOS (t_mr ((Pmin + Pmax) / 2.0, mr[i]) - T_K, y[1]);
	/*
	 * Plot the line and annotate just above the top of the line
	 */
		G_polyline (Skewt_bg_ov, GPLT_DASH, Colorbase + C_BG2, 
			2, x, y);
		sprintf (string, "%03.1f\0", mr[i]);
		G_text (Skewt_bg_ov, Colorbase + C_BG2, GTF_MINSTROKE, 
			0.02, GT_CENTER, GT_BOTTOM, x[1], y[1] + 0.01, string);
	}
/*
 * Saturated adiabats
 */
	for (t = 0; t <= 32; t += 4)
	{
		int	npts = 0;
		float	ept;
	/*
	 * Check for an interrupt
	 */
		if (Interrupt)
			skt_abort ();
	/*
	 * Find the equivalent potential temperature which corresponds
	 * to a saturated parcel at the surface at temperature t
	 */
		ept = theta_w (t + T_K, 1000.);
	/*
	 * Build the "iso-ept" curve
	 */
		for (p = Pmin + 100; p < Pmax + 15; p += 15)
		{
			float	temp = t_sat (ept, p) - T_K;
			y[npts] = YPOS (p);
			x[npts] = XPOS (temp, y[npts]);
			npts++;
		}
	/*
	 * Plot the curve and annotate just above the (Pmin + 100) isobar
	 */
		G_polyline (Skewt_bg_ov, GPLT_DOT, Colorbase + C_BG3, 
			npts, x, y);

		if (x[0] > 0.0 && x[0] < 1.0)
		{
			sprintf (string, "%d\0", (int) t);
			G_text (Skewt_bg_ov, Colorbase + C_BG3, GTF_MINSTROKE, 
				0.02, GT_CENTER, GT_BOTTOM, x[0], y[0] + 0.01,
				string);
		}
	}
/*
 * Dry adiabats
 */
 	for (pt = Tmin - (int) Tmin % 10; pt < Tmax + 200; pt += 10)
	{
		int	npts = 0;
	/*
	 * Check for an interrupt
	 */
		if (Interrupt)
			skt_abort ();
	/*
	 * Build a constant potential temperature curve, assuming a dry parcel
	 */
		for (p = Pmin; p < Pmax + 15; p += 15)
		{
			float	temp = theta_to_t (pt + T_K, p) - T_K;
			y[npts] = YPOS (p);
			x[npts] = XPOS (temp, y[npts]);
			npts++;
		}
	/*
	 * Plot the curve and annotate just inside either the left or top
	 * boundary
	 */
		G_polyline (Skewt_bg_ov, GPLT_DOT, Colorbase + C_BG4, npts, 
			x, y);

		sprintf (string, "%d\0", (int) pt);
		if (x[0] > 0.0 && x[0] <= 1.0)
			G_text (Skewt_bg_ov, Colorbase + C_BG4, GTF_MINSTROKE, 
				0.02, GT_LEFT, GT_TOP, x[0], y[0] - 0.01, 
				string);
		else if (x[0] <= 0.0)
		{
			float	slope, intercept;

			for (i = 0; i < npts; i++)
				if (x[i] > 0.0)
					break;

			if (i != npts)
			{
				slope = (y[i] - y[i-1]) / (x[i] - x[i-1]);
				intercept = y[i] - slope * x[i];
				G_text (Skewt_bg_ov, Colorbase + C_BG4, 
					GTF_MINSTROKE, 0.02, GT_LEFT, 
					GT_CENTER, 0.01, 
					intercept + 0.01 * slope,
					string);
			}
		}
	}
/*
 * Reset the redraw flag
 */
	Redraw = FALSE;
}



skt_limits (cmds)
struct ui_command	*cmds;
/*
 * Handle a "skewt tlimits" or "skewt plimits" command
 */
{
	float	min, max;

	if (UKEY (cmds[0]) == KW_TLIMITS)
	{
		min = UFLOAT (cmds[1]);
		max = UFLOAT (cmds[2]);

		if (min >= max)
			ui_error ("The min must be less than the max");

		Tmin = min;
		Tmax = max;
	}
	else
	{
		min = UFLOAT (cmds[2]);
		max = UFLOAT (cmds[1]);

		if (min >= max)
			ui_error ("The min must be less than the max");

		Pmin = min;
		Pmax = max;
	}
/*
 * We need to redraw the background
 */
	Redraw = TRUE;
}



void
skt_abort ()
{
/*
 * Abort the plot for an interrupt.  Make the overlays invisible, turn
 * off the interrupt flag, and bail out.
 */
	G_visible (Skewt_ov, FALSE);
	G_visible (Skewt_bg_ov, FALSE);
	G_visible (Winds_ov, FALSE);
	Interrupt = FALSE;
	ui_bailout ("Skew-t plot aborted");
}



void
skt_bot_text (string, color, newline)
char	*string;
int	color, newline;
/*
 * Add annotation to the bottom of the plot
 * newline is true if we want to start a new line
 */
{
	float	x0, y0, x1, y1;
/*
 * Set the clipping so we can annotate in the bottom margin
 */
	G_clip_window (Skewt_ov, -BORDER, -BORDER, 1.0 + 2 * BORDER, 0.0);
/*
 * Find out how the string fits on the current line
 */
	G_tx_box (Skewt_ov, GTF_MINSTROKE, 0.025, GT_LEFT, GT_TOP,
		Xtxt_bot, Ytxt_bot, string, &x0, &y0, &x1, &y1);
/*
 * Start a new line if necessary or requested
 */
	if (Xtxt_bot > 0.0 && (x1 > 1.0 + 2 * BORDER || newline))
	{
		Xtxt_bot = -0.5 * BORDER;
		Ytxt_bot -= 0.03;
	}
/*
 * Write in the annotation
 */
	G_text (Skewt_ov, Colorbase + color, GTF_MINSTROKE, 0.025, GT_LEFT,
		GT_TOP, Xtxt_bot, Ytxt_bot, string);
/*
 * Update the location for the next annotation
 */
	Xtxt_bot += x1 - x0;
/*
 * Reset the clipping
 */
	G_clip_window (Skewt_ov, 0.0, 0.0, 1.0, 1.0);
}



void
skt_top_text (string, color, newline)
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
	G_clip_window (Skewt_ov, -BORDER, 1.0, 1.0 + 2 * BORDER, 1.0 + BORDER);
/*
 * Find out how the string fits on the current line
 */
	G_tx_box (Skewt_ov, GTF_MINSTROKE, 0.025, GT_LEFT, GT_TOP,
		Xtxt_top, Ytxt_top, string, &x0, &y0, &x1, &y1);
/*
 * Start a new line if necessary or requested
 */
	if (Xtxt_top > 0.0 && (x1 > 1.0 + 2 * BORDER || newline))
	{
		Xtxt_top = -0.5 * BORDER;
		Ytxt_top -= 0.03;
	}
/*
 * Write in the annotation
 */
	G_text (Skewt_ov, Colorbase + color, GTF_MINSTROKE, 0.025, GT_LEFT, 
		GT_TOP, Xtxt_top, Ytxt_top, string);
/*
 * Update the location for the next annotation
 */
	Xtxt_top += x1 - x0;
/*
 * Reset the clipping
 */
	G_clip_window (Skewt_ov, 0.0, 0.0, 1.0, 1.0);
}



void
skt_reset_annot ()
/*
 * Reset the annotation locations
 */
{
	Xtxt_top = -0.5 * BORDER;
	Ytxt_top = 1.0 + 0.95 * BORDER;
	Xtxt_bot = -0.5 * BORDER;
	Ytxt_bot = -0.05 * BORDER;
}



void
skt_annotate (id_name, plot_ndx)
char	*id_name;
int	plot_ndx;
/*
 * Annotate the skew-t
 */
{
	char	temp[50], string[100], *site, *snd_site ();
	date	sdate, snd_time ();
/*
 * Top annotation
 */
	skt_top_text ("SITE: ", C_WHITE, TRUE);
	site = snd_site (id_name);
	skt_top_text (site, C_WHITE, FALSE);
	skt_top_text ("  TIME: ", C_WHITE, FALSE);
	sdate = snd_time (id_name);
	ud_format_date (temp, &sdate, UDF_FULL);
	strcpyUC (string, temp);
	skt_top_text (string, C_WHITE, FALSE);
	strcpy (string, "  (");
	strcpyUC (string + 3, id_name);
	strcat (string, ")  ");
	skt_top_text (string, C_WHITE, FALSE);
	skt_top_text ("TEMP/WINDS ", Temp_color[plot_ndx], FALSE);
	skt_top_text ("DEWPOINT  ", Dp_color[plot_ndx], FALSE);
}



void
skt_coords (xfld, yfld, xval, yval, x, y)
fldtype	xfld, yfld;
float	xval, yval, *x, *y;
/*
 * Translate the data values to an (x,y) position in the 0.0-1.0 range 
 * w.r.t. the whole screen
 */
{
/*
 * Check for winds, since the conversion is different
 */
	if (xfld == f_wspd || xfld == f_wdir)
	{
		*y = (YPOS (yval) + BORDER) / (1.0 + 2 * BORDER);
		*x = (1.0 + 2 * BORDER) / (1.0 + 3 * BORDER);
		return;
	}
/*
 * It isn't winds, so it must be temperature and pressure (the temp. may
 * be either dry bulb temp. or dewpoint)
 */
	yval = YPOS (yval);
	xval = XPOS (xval, yval);
/*
 * Return the screen relative position (0.0 to 1.0 in x and y).  
 * Temp and pres are currently in overlay coordinates, which run from
 * -BORDER to 1.0 + 2 * BORDER in x and -BORDER to 1.0 + BORDER in y.
 */
	*x = (xval + BORDER) / (1.0 + 3 * BORDER);
	*y = (yval + BORDER) / (1.0 + 2 * BORDER);
}



void
skt_ov_check ()
/*
 * Make sure our overlays exist and are associated with the 
 * current workstation
 */
{
/*
 * Do we have an overlay for the background?
 */
	if (! Skewt_bg_ov ||  G_ov_to_ws (Skewt_bg_ov) != Wkstn)
	{
		Redraw = TRUE;	/* Make sure we draw a new background */
		Skewt_bg_ov = G_new_overlay (Wkstn, 0);
		G_set_coords (Skewt_bg_ov, -BORDER, -BORDER, 1.0 + 2 * BORDER,
			1.0 + BORDER);
	}
/*
 * Do we have an overlay for the data?
 */
	if (! Skewt_ov || G_ov_to_ws (Skewt_ov) != Wkstn) 
	{
		Skewt_ov = G_new_overlay (Wkstn, 0);
		G_set_coords (Skewt_ov, -BORDER, -BORDER, 1.0 + 2 * BORDER,
			1.0 + BORDER);
	}
/*
 * Do we have a winds overlay?
 */
	if (! Winds_ov || G_ov_to_ws (Winds_ov) != Wkstn)
	{
		int	xres, yres;

		Winds_ov = G_new_overlay (Wkstn, 0);
	/*
	 * Set up the overlay so that the coordinates 0.0-1.0 in x and y
	 * plot a window to the right of the skew-t plot
	 */
		G_set_coords (Winds_ov, -0.5 * (1.0/BORDER + 1), -BORDER, 
			1.0, 1.0 + BORDER);
	/*
	 * True aspect ratio of the 0.0-1.0 window in the winds overlay
	 */
		G_w_inquire (Wkstn, GIW_XRES, &xres);
		G_w_inquire (Wkstn, GIW_YRES, &yres);
		W_aspect = (float) xres / (float) yres * 
			(1.0 + 2 * BORDER) / (1.0 + 0.5 * (1.0/BORDER + 1));
	}
}




void
skt_lift (ndata, pres, temp, dp)
int	ndata;
float	*pres, *temp, *dp;
/*
 * Draw the lines for a lifted parcel for the given pressure, temperature,
 * and dp data
 */
{
	int	npts, ndx_700, do_700;
	float	x[200], y[200];
	float	p_lcl, t_lcl, pt, w, t_sfc, p_sfc, dp_sfc, ept;
	float	t_700, dp_700, p_lcl700, t_lcl700, ept_700, pt_700;
	float	p, pstep = 10, t, min_lcl;
	void	an_surface ();
/*
 * Find the first good point and use it as the surface point
 */
	an_surface (temp, pres, dp, ndata, &t_sfc, &p_sfc, &dp_sfc);
	an_700 (temp, pres, dp, ndata, p_sfc, dp_sfc, &t_700, &dp_700, 
		&ndx_700);

	t_sfc += T_K;	t_700 += T_K;
	dp_sfc += T_K;	dp_700 += T_K;
/*
 * Find the mixing ratio for our surface point
 */
	w = w_sat (dp_sfc, p_sfc);
/*
 * Get the potential temperature, LCL pressure and temperature, and the 
 * eqivalent potential temperature at the surface
 */
	pt = theta_dry (t_sfc, p_sfc);
	p_lcl = lcl_pres (t_sfc, dp_sfc, p_sfc);
	t_lcl = lcl_temp (t_sfc, dp_sfc);
	ept = theta_w (t_lcl, p_lcl);
/*
 * Get the forecasted (700 mb) LCL pressure, temp, and ept
 */
	if (t_700 > dp_700)
	{
		pt_700 = theta_dry (t_700, 700.0);
		p_lcl700 = lcl_pres (t_700, dp_700, 700.0);
		t_lcl700 = lcl_temp (t_700, dp_700);
		ept_700 = theta_w (t_lcl700, p_lcl700);
		do_700 = TRUE;
	}
	else
	{
		do_700 = FALSE;
		ui_warning ("Unable to calculate forecasted lifted parcel");
	}
/*
 * Draw the saturated adiabat from the LCL up
 */
	npts = 0;

	for (p = p_lcl; p >= Pmin - pstep; p -= pstep)
	{
		t = t_sat (ept, p) - T_K;
		y[npts] = YPOS (p);
		x[npts] = XPOS ((float) t, y[npts]);
		npts++;
	}

	G_polyline (Skewt_ov, GPLT_DOT, Colorbase + C_BG1, npts, x, y);
/*
 * Draw the saturated adiabat from the forecasted LCL up
 */
	if (do_700)
	{
		npts = 0;

		for (p = p_lcl700; p >= Pmin - pstep; p -= pstep)
		{
			t = t_sat (ept_700, p) - T_K;
			y[npts] = YPOS (p);
			x[npts] = XPOS ((float) t, y[npts]);
			npts++;
		}

		G_polyline (Skewt_ov, GPLT_DOT, Colorbase + C_BG1, npts, x, y);
	}
/*
 * Draw the dry adiabat from the LCL down
 */
	npts = 0;

	for (p = p_lcl; p < p_sfc + pstep; p += pstep)
	{
	/*
	 * Stop at the surface pressure
	 */
		if (p > p_sfc)
			p = p_sfc;
	/*
	 * Get the temp corresponding to our theta at this pressure
	 */
		t = theta_to_t (pt, p) - T_K;
	/*
	 * Translate into overlay coordinates
	 */
		y[npts] = YPOS (p);
		x[npts] = XPOS ((float) t, y[npts]);
		npts++;
	}

	G_polyline (Skewt_ov, GPLT_DOT, Colorbase + C_BG1, npts, x, y);
/*
 * Draw the dry adiabat from the forecasted LCL down
 */
	if (do_700)
	{
		npts = 0;

		for (p = p_lcl700; p < 700.0 + pstep; p += pstep)
		{
		/*
		 * Stop at 700 mb
		 */
			if (p > 700.0)
				p = 700.0;
		/*
		 * Get the temp corresponding to our theta at this pressure
		 */
			t = theta_to_t (pt_700, p) - T_K;
		/*
		 * Translate into overlay coordinates
		 */
			y[npts] = YPOS (p);
			x[npts] = XPOS ((float) t, y[npts]);
			npts++;
		}

		G_polyline (Skewt_ov, GPLT_DOT, Colorbase + C_BG1, npts, x, y);
	}
/*
 * Draw the saturation mixing ratio line from the surface up to the LCL
 * with the lower pressure
 */
	if (do_700)
		min_lcl = (p_lcl < p_lcl700 ? p_lcl : p_lcl700);
	else
		min_lcl = p_lcl;

	t = t_mr (min_lcl, w) - T_K;
	y[0] = YPOS ((float) min_lcl);
	x[0] = XPOS ((float) t, y[0]);

	t = t_mr (p_sfc, w) - T_K;
	y[1] = YPOS ((float) p_sfc);
	x[1] = XPOS ((float) t, y[1]);

	npts = 2;

	G_polyline (Skewt_ov, GPLT_DOT, Colorbase + C_BG1, npts, x, y);
/*
 * Done
 */
	return;
}




void
skt_thermo (id_name, plot_ndx)
char	*id_name;
int	plot_ndx;
/*
 * Plot the thermo data for the given sounding
 */
{
	float	p[BUFLEN], t[BUFLEN], d[BUFLEN];
	float	xt[BUFLEN], xd[BUFLEN], yt[BUFLEN], yd[BUFLEN];
	float	y;
	int	i, npts, good_d = 0, good_t = 0;
/*
 * Grab the data
 */
	npts = snd_get_data (id_name, p, BUFLEN, f_pres, BADVAL);
	snd_get_data (id_name, t, BUFLEN, f_temp, BADVAL);
	snd_get_data (id_name, d, BUFLEN, f_dp, BADVAL);
/*
 * Translate to overlay coordinates
 */
	for (i = 0; i < npts; i++)
	{
		if (p[i] == BADVAL)
			continue;
		else
		{
			y = YPOS (p[i]);
			yt[good_t] = y;
			yd[good_d] = y;
		}

		if (t[i] != BADVAL)
			xt[good_t++] = XPOS (t[i], y);

		if (d[i] != BADVAL)
			xd[good_d++] = XPOS (d[i], y);
	}
/*
 * Draw the lines
 */
	G_polyline (Skewt_ov, GPLT_SOLID, Colorbase + Temp_color[plot_ndx], 
		good_t, xt, yt);
	G_polyline (Skewt_ov, GPLT_SOLID, Colorbase + Dp_color[plot_ndx], 
		good_d, xd, yd);
/*
 * Draw the lifted parcel lines if requested
 */
	if (Flg_lift)
		skt_lift (npts, p, t, d);
/*
 * Annotate
 */
	skt_annotate (id_name, plot_ndx);
/*
 * Tell edit about these two traces
 */
	edit_set_trace (id_name, f_temp, f_pres, Temp_color[plot_ndx]);
	edit_set_trace (id_name, f_dp, f_pres, Dp_color[plot_ndx]);
/*
 * Done
 */
	return;
}




void
skt_winds (id_name, plot_ndx, nplots)
char	*id_name;
int	plot_ndx, nplots;
/*
 * Plot the winds for the given sounding
 */
{
	float	p[BUFLEN], wspd[BUFLEN], wdir[BUFLEN], u[BUFLEN], v[BUFLEN];
	float	xstart, xscale, yscale, xov[2], yov[2];
	int	i, npts;
/*
 * Grab the winds data and convert it to u and v
 */
	npts = snd_get_data (id_name, p, BUFLEN, f_pres, BADVAL);
	snd_get_data (id_name, wspd, BUFLEN, f_wspd, BADVAL);
	snd_get_data (id_name, wdir, BUFLEN, f_wdir, BADVAL);

	for (i = 0; i < npts; i++)
	{
		if (wspd[i] == BADVAL || wdir[i] == BADVAL)
		{
			u[i] = BADVAL;
			v[i] = BADVAL;
		}
		else
		{
			u[i] = -wspd[i] * sin (DEG_TO_RAD (wdir[i]));
			v[i] = -wspd[i] * cos (DEG_TO_RAD (wdir[i]));
		}
	}
/*
 * Calculate the x starting position and the x and y scaling factors
 */
	xstart = (float) (plot_ndx + 0.5) / (float) (nplots);
	xscale = 1.0 / (W_scale * 2 * nplots);
	yscale = W_aspect * xscale;
/*
 * Plot the winds
 */
	for (i = 0; i < npts; i++)
	{
		if (p[i] == BADVAL || p[i] < Pmin || p[i] > Pmax)
			continue;
	/*
	 * Get the starting point
	 */
		xov[0] = xstart;
		yov[0] = YPOS (p[i]);
	/*
	 * Convert to the overlay coordinates
	 */
		if (u[i] != BADVAL)
			xov[1] = xov[0] + u[i] * xscale;
		else
			continue;

		if (v[i] != BADVAL)
			yov[1] = yov[0] + v[i] * yscale;
		else
			continue;
	/*
	 * Draw the wind line
	 */
		G_polyline (Winds_ov, GPLT_SOLID, 
			Colorbase + Temp_color[plot_ndx], 2, xov, yov);
	}
/*
 * Declare the winds fields to edit
 */
	edit_set_trace (id_name, f_wspd, f_pres, C_BG2);
	edit_set_trace (id_name, f_wdir, f_pres, C_BG2);
/*
 * Draw the staff
 */
	xov[0] = xstart;	xov[1] = xstart;
	yov[0] = 0.0;		yov[1] = 1.0;

	G_polyline (Winds_ov, GPLT_SOLID, Colorbase + C_BG2, 2, xov, yov);
/*
 * Annotate on the first one
 */
	if (plot_ndx == 0)
	{
		G_text (Winds_ov, Colorbase + C_WHITE, GTF_MINSTROKE, 0.025, 
			GT_CENTER, GT_TOP, 0.5, -0.01, "WINDS PROFILE");
		G_text (Winds_ov, Colorbase + C_WHITE, GTF_MINSTROKE, 0.02, 
			GT_LEFT, GT_CENTER, 0.5, -0.06, " = 10 M/S");

		xov[0] = 0.5 - (10.0 * xscale);
		xov[1] = 0.5;
		yov[0] = -0.06;
		yov[1] = -0.06;
		G_polyline (Winds_ov, GPLT_SOLID, Colorbase + C_WHITE, 
			2, xov, yov);
	}
/*
 * Done
 */
	return;
}




void
skt_wscale (cmds)
struct ui_command	*cmds;
/*
 * Set the wind scale value
 */
{
	W_scale = UFLOAT (cmds[0]);
}




void
skt_init ()
/*
 * Initialize some stuff
 */
{
	stbl symtbl = usy_g_stbl ("ui$variable_table");
/*
 * Initialize the plot limits
 */
	Pmin = 100.;
	Pmax = 1050.;
	Pstep = 100;
	Tmin = -40.;
	Tmax = 35.;
	Tstep = 10;
/*
 * Create UI indirect variables
 */
	usy_c_indirect (symtbl, "skewt_pmin", &Pmin, SYMT_FLOAT, 0);
	usy_c_indirect (symtbl, "skewt_pmax", &Pmax, SYMT_FLOAT, 0);
	usy_c_indirect (symtbl, "skewt_tmin", &Tmin, SYMT_FLOAT, 0);
	usy_c_indirect (symtbl, "skewt_tmax", &Tmax, SYMT_FLOAT, 0);
}

