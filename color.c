/*
 * Handle initialization and changing of colors
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

static char *rcsid = "$Id: color.c,v 1.7 1991-10-21 21:32:26 burghart Exp $";

# include <stdio.h>
# include "globals.h"
# include "color.h"
# include "keywords.h"

/*
 * Overlay for the color list plot
 */
static overlay	C_ov = 0;

/*
 * The current color values in RGB and a table of index names with
 * their current color names
 */
static float 	Red[NCOLORS];
static float 	Green[NCOLORS];
static float	Blue[NCOLORS];

static struct
{
	char	*ndxname, *colorname;
} C_current[NCOLORS];

/*
 * Forward declarations
 */
void	color_list (), color_build_ov (), color_set (), color_new ();
void	color_init (), color_loadmap ();




void
color_change (cmds)
struct ui_command	*cmds;
{
	char	*ndxname, *colorname;
	int	i, devmax, visible;
/*
 * Catch the COLOR LIST command
 */
	if (cmds[0].uc_ctype == UTT_KW)
	{
		color_list ();
		return;
	}
/*
 * Get the index name
 */
	ndxname = UPTR (cmds[0]);
/*
 * Find the index corresponding to ndxname
 */
	for (i = 0; i < NCOLORS; i++)
		if (! strcmp (ndxname, C_current[i].ndxname))
			break;

	if (i == NCOLORS)
		ui_error ("Unknown index name '%s'", ndxname);
/*
 * Deal with a named color or with RGB values (custom color)
 */
	if (cmds[1].uc_vptype == SYMT_STRING)
	{
	/*
	 * Named color
	 */
		colorname = UPTR (cmds[1]);
		color_set (i, colorname);
	}
	else
	{
	/*
	 * Custom color
	 */
		Red[i] = UFLOAT (cmds[1]);
		Green[i] = UFLOAT (cmds[2]);
		Blue[i] = UFLOAT (cmds[3]);
		if (C_current[i].colorname)
			free (C_current[i].colorname);
		C_current[i].colorname = (char *) malloc (7);
		strcpy (C_current[i].colorname, "custom");
	}
/*
 * If we have an output device, reload the colormap and make sure
 * the color list overlay is up-to-date
 */
	if (Wkstn)
	{
		G_w_inquire (Wkstn, GIW_NCOLOR, &devmax);
		G_set_color_map (Wkstn, Colorbase, MIN (devmax, NCOLORS), Red, 
			Green, Blue);
	/*
	 * Update the overlay if it's currently visible
	 */
		visible = C_ov && G_visible (C_ov, FALSE);
		if (visible)
		{
			color_build_ov ();
			G_visible (C_ov, TRUE);
			G_update (Wkstn);
		}
	}
}




void
color_set (index, colorname)
int	index;
char	*colorname;
/*
 * Change the current color at the selected index to the named color
 */
{
	double	r, g, b;
	int	status;
/*
 * Get the rgb values for the named color
 */
	status = G_name_to_rgb (colorname, &r, &g, &b);

	if (status == GE_BAD_FILE)
		ui_error ("Unable to open color name file");
	if (status == GE_BAD_COLOR)
		ui_error ("Unknown color '%s'", colorname);
/*
 * Put this color name into the current name table
 */
	if (C_current[index].colorname)
		free (C_current[index].colorname);
	C_current[index].colorname = (char *) malloc (1 + strlen (colorname));
	strcpy (C_current[index].colorname, colorname);
/*
 * Save the RGB values
 */
	Red[index] = (float) r;
	Green[index] = (float) g;
	Blue[index] = (float) b;
}



	
void
color_init ()
{
	char	string[50];
/*
 * Build the current color table
 */
	color_new (C_BLACK - Colorbase, "black", "black");
	color_new (C_WHITE - Colorbase, "white", "white");
	color_new (C_BG1 - Colorbase, "bg1", "gray60");
	color_new (C_BG2 - Colorbase, "bg2", "gray45");
	color_new (C_BG3 - Colorbase, "bg3", "steel blue");
	color_new (C_BG4 - Colorbase, "bg4", "sienna");
	color_new (C_DATA1 - Colorbase, "data1", "red");
	color_new (C_DATA2 - Colorbase, "data2", "cyan");
	color_new (C_DATA3 - Colorbase, "data3", "magenta");
	color_new (C_DATA4 - Colorbase, "data4", "medium slate blue");
	color_new (C_DATA5 - Colorbase, "data5", "yellow");
	color_new (C_DATA6 - Colorbase, "data6", "green");
	color_new (C_DATA7 - Colorbase, "data7", "gold");
	color_new (C_DATA8 - Colorbase, "data8", "dark orchid");
	color_new (C_DATA9 - Colorbase, "data9", "pale green");
}




void
color_new (index, ndxname, colorname)
int	index;
char	*ndxname, *colorname;
/*
 * Make a new entry into the current color list (the important thing done
 * here is the initialization of the ndxname entry in the table)
 */
{
	C_current[index].ndxname = (char *) malloc (1 + strlen (ndxname));
	strcpy (C_current[index].ndxname, ndxname);
	color_set (index, colorname);
}




void
color_list ()
/*
 * List the current color map values
 */
{
	int	i;
	struct ui_command	dummy;
/*
 * Tabular listing to the terminal
 */
	ui_printf ("Index   Color\n");
	ui_printf ("Name    Name                 Red  Green  Blue\n");
	ui_printf ("---------------------------------------------\n");
	for (i = 0; i < NCOLORS; i++)
		ui_printf ("%-8s%-20s %3.2f  %3.2f  %3.2f\n", 
			C_current[i].ndxname, C_current[i].colorname,
			Red[i], Green[i], Blue[i]);
/*
 * Graphic output (if we have an output device)
 */
	if (Wkstn)
	{
	/*
	 * Make the graphical color listing
	 */
		color_build_ov ();
	/*
	 * Declare this plot (necessary to remove the previous plot)
	 */
		dummy.uc_ctype = UTT_END;
		edit_set_plot (color_list, (void (*)) 0, &dummy, &C_ov, 1);

		G_update (Wkstn);
	}
}



void
color_build_ov ()
/*
 * Build the graphics overlay which shows the current colors
 */
{
	int	i, j, color, npoints;
	float	x, y, x0, x1, y0, y1, pl_x[200], pl_y[200];
	char	string[10];
/*
 * Get the overlay if necessary
 */
	if (! C_ov || G_ov_to_ws (C_ov) != Wkstn)
	{
		C_ov = G_new_overlay (Wkstn, 0);
		G_set_coords (C_ov, 0.0, 0.0, 2.0, 8.0);
	}

	G_clear (C_ov);
/*
 * Write the index name and color name for each color onto the graphics
 * screen
 */
	for (i = 0; i < NCOLORS; i++)
	{
	/*
	 * Start a new line every other color
	 */
		if (! (i % 2))
		{
			x = 0.5;
			if (i == 0)
				y = 7.5;
			else
				y -= 1.0;
		}
	/*
	 * Color 0 is the screen background color, so we need a different
	 * color behind it for it to show up
	 */
		if (i == 0)
		{
		/*
		 * White rectangle for the index name
		 *
		 * Build a dense polyline, drawing back and forth to cover 
		 * the rectangle.  (we do this as a polyline instead of the 
		 * more sensible pixmap because the polyline is much faster 
		 * on most devices)
		 */
			G_tx_box (C_ov, GTF_STROKE, 0.3, GT_CENTER, 
				GT_CENTER, x, y + 0.16, C_current[i].ndxname,
				&x0, &y0, &x1, &y1);

			x0 -= 0.02; y0 -= 0.02;
			x1 += 0.02; y1 += 0.02;
			npoints = 0;

			for (; y0 < y1; y0 += 0.01)
			{
				pl_x[npoints] = x0;
				pl_y[npoints] = y0;
				npoints++;

				pl_x[npoints] = x1;
				pl_y[npoints] = y0;
				npoints++;
			}

			G_polyline (C_ov, GPLT_SOLID, C_WHITE, npoints, pl_x, 
				pl_y);
		/*
		 * White rectangle for the color name
		 */
			G_tx_box (C_ov, GTF_STROKE, 0.3, GT_CENTER, 
				GT_CENTER, x, y - 0.16, C_current[i].colorname,
				&x0, &y0, &x1, &y1);

			x0 -= 0.02; y0 -= 0.02;
			x1 += 0.02; y1 += 0.02;
			npoints = 0;
			for (; y0 < y1; y0 += 0.01)
			{
				pl_x[npoints] = x0;
				pl_y[npoints] = y0;
				npoints++;

				pl_x[npoints] = x1;
				pl_y[npoints] = y0;
				npoints++;
			}

			G_polyline (C_ov, GPLT_SOLID, C_WHITE, npoints, pl_x, 
				pl_y);
		}
	/*
	 * Print the index name and color name on the screen
	 */
		color = Colorbase + i;

		G_write (C_ov, color, GTF_DEV, 0.3, GT_CENTER, 
			GT_CENTER, x, y + 0.16, 0.0, C_current[i].ndxname);
		G_write (C_ov, color, GTF_DEV, 0.3, GT_CENTER, 
			GT_CENTER, x, y - 0.16, 0.0, C_current[i].colorname);
	/*
	 * Move the text position
	 */
		x += 1.0;
	}
}




void
color_loadmap ()
/*
 * Initialize the colors on our workstation
 */
{
	int	devmax;

	G_w_inquire (Wkstn, GIW_NCOLOR, &devmax);
/*
 * Allocate color space and set the color map
 */
	G_get_color (Wkstn, MIN (devmax, NCOLORS), &Colorbase);
	G_set_color_map (Wkstn, Colorbase, MIN (devmax, NCOLORS), Red, 
		Green, Blue);
}
