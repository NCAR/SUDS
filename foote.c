/*
 * Foote chart stuff
 *
 * $Revision: 1.7 $ $Date: 1989-10-09 15:52:33 $ $Author: burghart $
 * 
 */
# include <ui_date.h>
# include "globals.h"
# include "derive.h"
# include "color.h"
# include "fields.h"
# include "flags.h"

# define R_D	287.
# define G_0	9.80665
# define T_3	273.15
# define MAX_LI	10.0

/*
 * The overlay for the plot
 */
static overlay	Foote_ov = 0;

/*
 * How high are we going?
 */
static float	Foote_height = 3.0;

/*
 * Current text position for top annotation
 */
static float	Xtxt_top = -0.5 * BORDER, Ytxt_top = 1.0 + 0.95 * BORDER;

/*
 * Forward declarations
 */
void	ft_background (), ft_annotate (), ft_top_text (), ft_abort ();
float	ft_alt ();



void
ft_plot (cmds)
struct ui_command *cmds;
/*
 * Plot a Foote chart for the given sounding
 */
{
	char	*id_name;
	int	i, maxpts, pt = 0, nplots, plt, color;
	float	x[200], yli[200], ydist[200];
	float	pres[BUFLEN], temp[BUFLEN], dp[BUFLEN], alt[BUFLEN];
	float	li, p_lcl, t_lcl, theta_e_lcl, lfc_pres;
	float	an_li (), snd_s_alt (), an_lfc_calc ();
	char	*snd_default ();
	void	ft_reset_annot ();
/*
 * Make sure the graphics stuff is ready
 */
	out_ws_check ();

	if (! Foote_ov || G_ov_to_ws (Foote_ov) != Wkstn) 
	{
		Foote_ov = G_new_overlay (Wkstn, 0);
		G_set_coords (Foote_ov, -BORDER, -BORDER, 1.0 + BORDER,
			1.0 + BORDER);
	}
/*
 * Let edit know about this plot
 */
	edit_set_plot (ft_plot, 0, cmds, &Foote_ov, 1);
/*
 * Get the overlay ready
 */
	G_clear (Foote_ov);
	G_clip_window (Foote_ov, 0.0, 0.0, 1.0, 1.0);
/*
 * Plot the background
 */
	ft_background ();
/*
 * Reset the annotation position
 */
	ft_reset_annot ();
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
 * Loop through the soundings
 */
	for (plt = 0; plt < nplots; plt++)
	{
	/*
	 * Set the color
	 */
		color = C_TRACE1 + plt;
	/*
	 * Get the name of the sounding (use the default if necessary)
	 */
		if (cmds->uc_ctype != UTT_END)
			id_name = UPTR (cmds[plt]);
		else
			id_name = snd_default ();
	/*
	 * Get the data
	 */
		maxpts = snd_get_data (id_name, pres, BUFLEN, f_pres, BADVAL);
		snd_get_data (id_name, temp, BUFLEN, f_temp, BADVAL);
		snd_get_data (id_name, dp, BUFLEN, f_dp, BADVAL);
		snd_get_data (id_name, alt, BUFLEN, f_alt, BADVAL);
	/*
	 * Plot the annotation
	 */
		ft_annotate (id_name, color);
	/*
	 * Loop through the points
	 */
		pt = 0;

		for (i = 0; i < maxpts; i++)
		{
		/*
		 * Check for an interrupt
		 */
			if (Interrupt)
				ft_abort ();
		/*
		 * Don't use bad points
		 */
			if (pres[i] == BADVAL || temp[i] == BADVAL || 
				dp[i] == BADVAL)
				continue;
		/*
		 * Break out when we get above the max height for the plot
		 */
			if (alt[i] > Foote_height * 1000.0)
				break;
		/*
		 * Calculate the x plot coordinate for this height
		 */
			x[pt] = alt[i] / (Foote_height * 1000.0);
		/*
		 * Get LCL info
		 */
			p_lcl = lcl_pres (temp[i] + T_K, dp[i] + T_K, pres[i]);
			t_lcl = lcl_temp (temp[i] + T_K, dp[i] + T_K);
			theta_e_lcl = theta_w (t_lcl, p_lcl);
		/*
		 * Get the parcel lifted index and calculate the y coordinate
		 * for this point
		 */
			li = an_li (pres, temp, maxpts, theta_e_lcl, FALSE);
			yli[pt] = li / (2.0 * MAX_LI) + 0.5;
		/*
		 * Find the LFC pressure for this parcel
		 */
			lfc_pres = an_lfc_calc (temp, pres, dp, maxpts, 
				temp[i], pres[i], dp[i], 350.0);

			if (lfc_pres != BADVAL)
				ydist[i] = (ft_alt (lfc_pres, temp, pres, dp, 
					alt, maxpts) - 
					alt[i]) / (Foote_height * 1000.0);
			else
				ydist[i] = 1.0;
		/*
		 * Increment the number of plot points
		 */
			pt++;
		}
	/*
	 * Draw the lines
	 */
		G_polyline (Foote_ov, GPLT_DOT, color, pt, x, yli);
		G_polyline (Foote_ov, GPLT_SOLID, color, pt, x, ydist);
	}
/*
 * Update the display
 */
	G_update (Wkstn);
}



void
ft_reset_annot ()
/*
 * Reset the annotation location
 */
{
	Xtxt_top = -0.5 * BORDER;
	Ytxt_top = 1.0 + 0.95 * BORDER;
}



void
ft_annotate (id_name, color)
char	*id_name;
int	color;
{
	char	site[40], string[100], *snd_site ();
	date	sdate, snd_time ();
/*
 * Unclip so we can annotate outside the main portion
 */
	G_clip_window (Foote_ov, -BORDER, -BORDER, 1.0 + BORDER, 1.0 + BORDER);
/*
 * Site
 */
	ft_top_text ("SITE: ", C_WHITE, TRUE);
	ft_top_text (snd_site (id_name), C_WHITE, FALSE);
/*
 * Time
 */
	ft_top_text ("  TIME: ", C_WHITE, FALSE);
	sdate = snd_time (id_name);
	ud_format_date (string, &sdate, UDF_FULL);
	strcpyUC (string, string);
	ft_top_text (string, C_WHITE, FALSE);
/*
 * ID name
 */
	strcpy (string, "   (");
	strcpyUC (string + 4, id_name);
	strcat (string, ")");
	ft_top_text (string, color, FALSE);
}




void
ft_background ()
/*
 * Draw the background for the Foote chart
 */
{
	float	height, li, x[2], y[2];
	char	string[50];
/*
 * Unclip so we can annotate outside the main portion
 */
	G_clip_window (Foote_ov, -BORDER, -BORDER, 1.0 + BORDER, 1.0 + BORDER);
/*
 * Draw a 500 m grid
 */
	for (height = 0.0; height <= Foote_height; height += 0.5)
	{
	/*
	 * Draw a vertical line for this height and label it
	 */
		x[0] = x[1] = height / Foote_height;
		y[0] = 0.0;
		y[1] = 1.0;
		G_polyline (Foote_ov, GPLT_SOLID, C_BG2, 2, x, y);

		sprintf (string, "%.1f", height);
		G_write (Foote_ov, C_WHITE, GTF_MINSTROKE, 0.025, 
			GT_CENTER, GT_TOP, x[0], -0.01, 0.0, string);
	/*
	 * Draw a horizontal line for this height (up to Foote_height km)
	 */
		if (height > Foote_height)
			continue;

		x[0] = 0.0;
		x[1] = 1.0;
		y[0] = y[1] = height / Foote_height;
		G_polyline (Foote_ov, GPLT_SOLID, C_BG2, 2, x, y);

		G_write (Foote_ov, C_WHITE, GTF_MINSTROKE, 0.025, 
			GT_RIGHT, GT_CENTER, -0.01, y[0], 0.0, string);
	}
/*
 * Text blurbs
 */
	G_write (Foote_ov, C_WHITE, GTF_MINSTROKE, 0.025, GT_CENTER, 
		GT_BOTTOM, -0.1, 0.5, 90.0, "DISTANCE TO LFC (KM)");

	G_write (Foote_ov, C_WHITE, GTF_MINSTROKE, 0.025, GT_CENTER, 
		GT_CENTER, 0.5, -.1, 0.0, "PARCEL HEIGHT (KM)");
/*
 * Lifted index scale
 */
	x[0] = 0.0;
	x[1] = 1.0;

	for (li = -MAX_LI; li <= MAX_LI; li += 2.0)
	{
	/*
	 * Draw a horizontal line for this lifted index
	 * (The extra 0.002 makes sure we don't plot over a distance line)
	 */
		y[0] = y[1] = li / (2.0 * MAX_LI) + 0.5 + 0.002;
		G_polyline (Foote_ov, GPLT_DOT, C_BG4, 2, x, y);

		sprintf (string, "%.1f", li);
		G_write (Foote_ov, C_BG4, GTF_MINSTROKE, 0.025, GT_LEFT, 
			GT_CENTER, 1.01, y[0], 0.0, string);
	}

	sprintf (string, "PARCEL LIFTED INDEX (AT %d MB)", 
		Flg_mli ? 400 : 500);
	G_write (Foote_ov, C_BG4, GTF_MINSTROKE, 0.025, GT_CENTER, GT_BOTTOM, 
		1.1, 0.5, -90.0, string);
/*
 * Restore the original clipping
 */
	G_clip_window (Foote_ov, 0.0, 0.0, 1.0, 1.0);
}
		



float
ft_alt (lfc_pres, temp, pres, dp, alt, npts)
float	lfc_pres, *temp, *pres, *dp, *alt;
int	npts;
/*
 * Find the height of the given LFC
 */
{
	int	i = 0, ceil, floor;
	float	e_ceil, e_floor, vt_ceil, vt_floor, vt_lfc;
/*
 * Find the first good point above this LFC (the 'ceiling')
 */
	while (pres[i] > lfc_pres || temp[i] == BADVAL ||
		pres[i] == BADVAL || dp[i] == BADVAL || 
		alt[i] == BADVAL)
		if (++i == npts)
			ui_error ("*BUG* Ceil error in ft_height");

	ceil = i;
/*
 * Find the first good point below the LFC (the 'floor')
 */
	i--;
	while (temp[i] == BADVAL || pres[i] == BADVAL || 
		dp[i] == BADVAL || alt[i] == BADVAL)
		if (--i == npts)
			ui_error ("*BUG* Floor error in ft_height");

	floor = i;
/*
 * Now find the virtual temperatures at the ceiling and floor points
 */
	e_ceil = e_from_dp (dp[ceil] + T_3);
	e_floor = e_from_dp (dp[floor] + T_3);
	vt_ceil = t_v (temp[ceil], pres[ceil], e_ceil);
	vt_floor = t_v (temp[floor], pres[floor], e_floor);
/*
 * Interpolate to get the virtual temperature at the LFC
 */
	vt_lfc = vt_floor + (vt_ceil - vt_floor) * 
		(lfc_pres - pres[floor]) / (pres[ceil] - pres[floor]);
/*
 * Calculate the thickness of the layer from the floor to the LFC and add 
 * it to the floor altitude
 *
 * This formula is adapted from equations 2.24 of Wallace and Hobbs' 
 * "Atmospheric Science" (1977) using a simple estimation of the 
 * integral.
 */
	return (alt[floor] + R_D / G_0 * 0.5 * 
		(vt_floor / pres[floor] + vt_lfc / lfc_pres) * 
		(pres[floor] - lfc_pres));
}




void
ft_top_text (string, color, newline)
char    *string;
int     color, newline;
/*
 * Add annotation to the bottom of the plot
 * newline is true if we want to start a new line
 */
{
        float   x0, y0, x1, y1;
/*
 * Set the clipping so we can annotate in the top margin
 */
	G_clip_window (Foote_ov, -BORDER, -BORDER, 1.0 + BORDER, 1.0 + BORDER);
/*
 * Find out how the string fits on the current line
 */
        G_tx_box (Foote_ov, GTF_MINSTROKE, 0.025, GT_LEFT, GT_TOP,
                Xtxt_top, Ytxt_top, string, &x0, &y0, &x1, &y1);
/*
 * Start a new line if necessary or requested
 */
        if (Xtxt_top > 0.0 && (x1 > 1.0 + 0.75 * BORDER || newline))
        {
                Xtxt_top = -0.5 * BORDER;
                Ytxt_top -= 0.028;
        }
/*
 * Write in the annotation
 */
        G_write (Foote_ov, color, GTF_MINSTROKE, 0.025, GT_LEFT, GT_TOP, 
		Xtxt_top, Ytxt_top, 0.0, string);
/*
 * Update the location for the next annotation
 */
        Xtxt_top += x1 - x0;
/*
 * Reset the clipping
 */
        G_clip_window (Foote_ov, 0.0, 0.0, 1.0, 1.0);
}




void
ft_abort ()
/*
 * Abort the plot for an interrupt.  Make the overlays invisible, turn
 * off the interrupt flag, and bail out.
 */
{
	G_visible (Foote_ov, FALSE);
	Interrupt = FALSE;
	ui_bailout ("Foote chart aborted");
}
