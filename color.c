/*
 * Handle initialization and changing of colors
 *
 * $Revision: 1.1 $ $Date: 1989-08-03 09:47:01 $ $Author: burghart $
 */
# include <stdio.h>
# include "globals.h"
# include "color.h"
# include "keywords.h"

/*
 * Overlay for the color list plot
 */
static overlay	C_ov;

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
		}
		G_update (Wkstn);
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
	color_new (C_BLACK, "black", "black");
	color_new (C_WHITE, "white", "white");
	color_new (C_BG1, "bg1", "gray60");
	color_new (C_BG2, "bg2", "gray45");
	color_new (C_BG3, "bg3", "steel blue");
	color_new (C_BG4, "bg4", "sienna");
	color_new (C_TRACE1, "trace1", "red");
	color_new (C_TRACE2, "trace2", "cyan");
	color_new (C_TRACE3, "trace3", "magenta");
	color_new (C_TRACE4, "trace4", "medium slate blue");
	color_new (C_TRACE5, "trace5", "yellow");
	color_new (C_TRACE6, "trace6", "green");
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
	 * Make the overlay visible, and declare this plot (necessary to 
	 * remove the previous plot)
	 */
		G_visible (C_ov, TRUE);
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
	int	i, color;
	float	x, y;
	char	string[10];

	if (! C_ov)
	{
		C_ov = G_new_overlay (Wkstn, 0);
		G_set_coords (C_ov, 0.0, 0.0, 2.0, 8.0);
	}

	G_clear (C_ov);

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
	 * Use the associated color except for C_BLACK, since it won't
	 * show up against the background (which is C_BLACK by definition)
	 */
		if (i == C_BLACK)
			color = Colorbase + C_WHITE;
		else
			color = Colorbase + i;
	/*
	 * Print the index name and color name on the screen
	 */
		G_text (C_ov, color, GTF_STROKE, 0.3, GT_CENTER, 
			GT_CENTER, x, y + 0.16, C_current[i].ndxname);
		G_text (C_ov, color, GTF_STROKE, 0.3, GT_CENTER, 
			GT_CENTER, x, y - 0.16, C_current[i].colorname);
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
	int	devmax, base, i;

	G_w_inquire (Wkstn, GIW_NCOLOR, &devmax);
/*
 * Allocate color space and set the color map
 */
	G_get_color (Wkstn, MIN (devmax, NCOLORS), &Colorbase);
	G_set_color_map (Wkstn, Colorbase, MIN (devmax, NCOLORS), Red, 
		Green, Blue);
}
