/*
 * Editing routines
 * 
 * $Revision: 1.4 $ $Date: 1989-09-21 13:34:40 $ $Author: burghart $
 * 
 */
# include <math.h>
# include "globals.h"
# include "keywords.h"
# include "color.h"
# include "sounding.h"

/*
 * BETWEEN macro: true if a is between b and c (inclusive)
 */
# define BETWEEN(a,b,c)	(((a)<=(b) && (a)>=(c)) || ((a)>=(b) && (a)<=(c)))

/*
 * The sounding-id, field, and current data pointer for the field 
 * being edited
 */
static char	Eid[40];
static fldtype	Efld;
static struct snd_datum	*E_datum = 0;

/*
 * The most recent plot: plot routine, coordinate generator, commands, 
 * overlays, number of overlays
 */
# define MAXOV	5
static struct
{
	void	(*plot) ();
	void	(*coords) ();
	struct ui_command	*cmds;
	overlay	ov[MAXOV];
	int	ovcount;
} Lastplot;

/*
 * Is a replot being done by the editor?
 */
static int	Replot = FALSE;

/*
 * The traces in the most recent plot
 */
static struct
{
	char	*id;		/* The sounding id */
	fldtype	xfld, yfld;	/* x and y variables of the trace */
	int	color;
} Trace[30];
static int	Ntrace = 0;

/*
 * The variables below hold extra information when the field being edited
 * is in the current plot
 */
static int	Etrace = -1;	/* Trace containing the edit field	*/

static int	Editing_x;	/* editing x or y field of the trace?	*/

static float	Xcursor, Ycursor;	/* Position of cursor in screen */
					/* relative coordinates		*/

overlay		Cursor_ov;	/* Overlay for the cursor	*/

				/* Pointers to the primary (edit) datum */
				/* and the secondary (reference) datum	*/
				/* for the current cursor position	*/
static struct snd_datum	*Cursor_pri = 0, *Cursor_sec = 0;

/*
 * Info about the current edit region
 */
static struct
{
	struct snd_datum	*mark;
	float	x[256], y[256];
	int	current, lastdraw;
	overlay	ov;
} E_region;

/*
 * Clipping stuff lifted from clip.c in the RDSS graphics package:
 *
 * A lot of the clipping algorithm used here comes from "Principles of
 * Interactive Computer Graphics," by William Newman and Robert Sproull.
 *
 * The first derivation from that work is this macro, which creates a four
 * bit pattern which describes the relationship of a given point to the
 * clip window.  It divides the world up like this:
 *
 *			|        |
 *		  1001	|  1000  |  1010
 *			|        |
 *		--------+--------+--------
 *			|        |
 *		  0001	|  0000  |  0010
 *			|        |
 *		--------+--------+--------
 *			|        |
 *		  0101	|  0100  |  0110
 *			|        |
 *
 * where the center box is the clip window.
 *
 * The nifty features of this encoding are (1) lines that do not need to be
 * clipped have codes of zero for both endpoints, and (2) lines for which the
 * intersection of the two codes is NOT zero are completely outside the 
 * window, and can be ignored.
 */
# define B_LEFT	0x01
# define B_RIGHT 0x02
# define B_BELOW 0x04
# define B_ABOVE 0x08

# define CLIPCODE(x,y) \
		((x < 0.0 ? B_LEFT : 0) | (x > 1.0 ? B_RIGHT : 0) | \
		 (y < 0.0 ? B_BELOW : 0) | (y > 1.0 ? B_ABOVE : 0))


/*
 * Forward declarations
 */
void	edit_saveloc (), edit_show_region (), edit_draw_cursor ();
void	edit_unpoint ();




void
edit_up_pointer (cmds)
struct ui_command	*cmds;
{
	int	count, i;
	struct snd_datum	*pt;
/*
 * Has the pointer been placed yet?
 */
	if (! E_datum)
		ui_error ("You must do a SELECT before moving the pointer");
/*
 * Get the repeat count (default to 1)
 */
	if (cmds[0].uc_ctype == UTT_END)
		count = 1;
	else
		count = UINT (cmds[0]);
/*
 * Loop through the requested number of pointer movements
 */
	for (i = 0; i < count; i++)
	{
	/*
	 * Break if we're at the top
	 */
		if (! E_datum->next)
			break;
	/*
	 * Move to the next point
	 */
		E_datum = E_datum->next;
	/*
	 * Only do the rest of this if we're dealing with a physical cursor
	 */
		if (Etrace < 0)
			continue;
	/*
	 * Get the physical cursor right
	 */
		for (pt = Cursor_sec; pt; pt = pt->next)
			if (pt->index >= E_datum->index)
				break;
	/*
	 * If we have an index match, save the point; this is the new
	 * physical cursor position
	 */
		if (pt->index == E_datum->index)
			edit_saveloc (E_datum, pt);
	}
/*
 * Redraw the physical cursor and the edit region
 */
	edit_show_region ();
	edit_draw_cursor ();
/*
 * Done
 */
	return;
}




void
edit_down_pointer (cmds)
struct ui_command	*cmds;
{
	int	count, i;
	struct snd_datum	*pt;
/*
 * Has the pointer been placed yet?
 */
	if (! E_datum)
		ui_error ("You must do a SELECT before moving the pointer");
/*
 * Get the repeat count (default to 1)
 */
	if (cmds[0].uc_ctype == UTT_END)
		count = 1;
	else
		count = UINT (cmds[0]);
/*
 * Loop through the requested number of pointer movements
 */
	for (i = 0; i < count; i++)
	{
	/*
	 * Break if we're at the bottom
	 */
		if (! E_datum->prev)
			break;
	/*
	 * Move to the next point
	 */
		E_datum = E_datum->prev;
	/*
	 * Only do the rest of this if we're dealing with a physical cursor
	 */
		if (Etrace < 0)
			continue;
	/*
	 * Get the physical cursor right
	 */
		for (pt = Cursor_sec; pt; pt = pt->prev)
			if (pt->index <= E_datum->index)
				break;
	/*
	 * If we have an index match, save the point; this is the new
	 * physical cursor position
	 */
		if (pt->index == E_datum->index)
			edit_saveloc (E_datum, pt);
	}
/*
 * Redraw the physical cursor and the edit region
 */
	edit_show_region ();
	edit_draw_cursor ();
/*
 * Done
 */
	return;
}



void
edit_saveloc (primary, secondary)
struct snd_datum	*primary, *secondary;
/*
 * Save the physical cursor location
 */
{
	int	old_dis, new_dis;
/*
 * We can just save the cursor data pointers if we don't have an
 * edit region
 */
	if (! E_region.mark)
	{
		Cursor_pri = primary;
		Cursor_sec = secondary;
		return;
	}
/*
 * Get the old and new distances from the mark point
 */
	old_dis = abs (Cursor_pri->index - E_region.mark->index);
	new_dis = abs (primary->index - E_region.mark->index);
/*
 * Save the new cursor position
 */
	Cursor_pri = primary;
	Cursor_sec = secondary;
/*
 * Update the edit region
 */
	if (new_dis > old_dis)
	{
	/*
	 * We're moving away from the mark, add a point to the edit region
	 */
		int	n = ++E_region.current;
		float	xval, yval;
	/*
	 * Set the x and y values based on what we're editing
	 */
		if (Editing_x)
		{
			xval = primary->value;
			yval = secondary->value;
		}
		else
		{
			yval = primary->value;
			xval = secondary->value;
		}
	/*
	 * Get the screen relative coordinates (0.0 - 1.0) based on the 
	 * current plot and put them in the E_region point arrays.
	 */
		(*Lastplot.coords) (Trace[Etrace].xfld, Trace[Etrace].yfld,
			xval, yval, &E_region.x[n], &E_region.y[n]);
	}
	else
	{
	/*
	 * We're moving toward the mark, remove a point from the edit region
	 */
		E_region.current--;
	/*
	 * If we're going back through the mark, draw now
	 */
		if (! E_region.current)
			edit_show_region ();
	}
/*
 * Done
 */
	return;
}



void
edit_show_region ()
/*
 * Highlight the trace between the mark point and the current pointer
 * position.
 */
{
	int	offset, color, npts;
/*
 * If the edit field isn't in the current plot or if the mark
 * has not been set, just return
 */
	if (Etrace < 0 || !E_region.mark)
		return;
/*
 * Different actions based on whether we increased or decreased the edit
 * region since the last draw
 */
	if (E_region.current > E_region.lastdraw)
	{
	/*
	 * We're adding to the region.  Draw the line in white.
	 */
		offset = E_region.lastdraw;
		npts = E_region.current - E_region.lastdraw + 1;
		color = C_WHITE;
	}
	else
	{
	/*
	 * We backed up over points, draw over the old section using the
	 * trace color.
	 */
		offset = E_region.current;
		npts = E_region.lastdraw - E_region.current + 1;
		color = Trace[Etrace].color;
	}
/*
 * Do the actual drawing
 */
	G_polyline (E_region.ov, GPLT_SOLID, color, npts,
		E_region.x + offset, E_region.y + offset);
	G_update (Wkstn);
/*
 * Update lastdraw
 */
	E_region.lastdraw = E_region.current;
}




void
edit_select (cmds)
struct ui_command	*cmds;
/*
 * Handle the SELECT command
 */
{
	int	i;
	char	*id_name;
	fldtype	fld;
	struct snd_datum	*x_pt, *y_pt, *snd_data_ptr ();
	char	*snd_default ();
/*
 * Get the field
 */
	fld = fd_num (UPTR (cmds[0]));
/*
 * Get the sounding name (or the default if none was specified)
 */
	if (cmds[1].uc_ctype != UTT_END)
		id_name = UPTR (cmds[1]);
	else
		id_name = snd_default ();
/*
 * Get a pointer to the data for the given sounding and field
 */
	E_datum = snd_data_ptr (id_name, fld);
/*
 * Save the sounding-id and field
 */
	strcpy (Eid, id_name);
	Efld = fld;
/*
 * See if this field is in the current plot
 */
	Etrace = -1;

	for (i = 0; i < Ntrace; i++)
	{
	/*
	 * Do we have a sounding match?
	 */
		if (strcmp (Trace[i].id, id_name))
			continue;
	/*
	 * Does the x field for this trace match?
	 */
		if (Trace[i].xfld == fld)
		{
		/*
		 * Save the trace number
		 */
			Editing_x = TRUE;
			Etrace = i;
			break;
		}
	/*
	 * Does the y field for this trace match?
	 */
		else if (Trace[i].yfld == fld)
		{
		/*
		 * Save the trace number
		 */
			Etrace = i;
			Editing_x = FALSE;
			break;
		}
	}
/*
 * If we didn't find the field in the current plot, return now
 */
	if (Etrace < 0)
		return;
/*
 * Get pointers to the data linked lists for the x and y fields
 */
	x_pt = snd_data_ptr (id_name, Trace[Etrace].xfld);	
	y_pt = snd_data_ptr (id_name, Trace[Etrace].yfld);
/*
 * Assign the cursor data pointers based on whether we're editing x or y
 */
	if (Editing_x)
	{
		Cursor_pri = x_pt;
		Cursor_sec = y_pt;
	}
	else
	{
		Cursor_pri = y_pt;
		Cursor_sec = x_pt;
	}
/*
 * Find the first plottable point (i.e., good data in both fields)
 */
	while (Cursor_pri->index != Cursor_sec->index)
	{
	/*
	 * Make sure we have points left to test
	 */
		if (! Cursor_pri || ! Cursor_sec)
			return;
	/*
	 * Increment the point with the lower index
	 */
		if (Cursor_pri->index < Cursor_sec->index)
			Cursor_pri = Cursor_pri->next;
		else
			Cursor_sec = Cursor_sec->next;
	}
/*
 * Forget the old edit region
 */
	E_region.mark = 0;

	if (Wkstn)
	{
		if (! E_region.ov || G_ov_to_ws (E_region.ov) != Wkstn)
			E_region.ov = G_new_overlay (Wkstn, 1);
		else
			G_clear (E_region.ov);
	}
/*
 * Our cursor pointers are correct, put up the cursor
 */
	edit_draw_cursor ();
}




void
edit_set_plot (plot, coords, cmds, ovlist, ovcount)
void	(*plot) (), (*coords) ();
struct ui_command	*cmds;
overlay	*ovlist;
int	ovcount;
/*
 * Declare the current plot so we can make use of it for editing.
 */
{
	int	i;
	struct ui_command	*uip_clone_clist ();
/*
 * If we're doing an editor replot, just return
 */
	if (Replot)
		return;
/*
 * Make the overlays from the previous plot invisible
 */
	if (Lastplot.ovcount)
		for (i = 0; i < Lastplot.ovcount; i++)
			if (G_ov_to_ws (Lastplot.ov[i]) == Wkstn)
				G_visible (Lastplot.ov[i], FALSE);
/*
 * Put the new plot and coord routines and the commands into the 
 * Lastplot structure
 */
	Lastplot.plot = plot;
	Lastplot.coords = coords;

	if (Lastplot.cmds)
		uip_release (Lastplot.cmds);

	Lastplot.cmds = uip_clone_clist (cmds);
/*
 * Remember the overlays from the current plot (and make sure
 * they're visible)
 */
	if (ovcount > MAXOV)
		ui_error ("(BUG) Can't handle %d overlays in edit_set_plot\n",
			ovcount);

	Lastplot.ovcount = ovcount;
	for (i = 0; i < ovcount; i++)
	{
		Lastplot.ov[i] = ovlist[i];
		G_visible (ovlist[i], TRUE);
	}
/*
 * Forget the old traces and the selected trace
 */
	Ntrace = 0;
	Etrace = -1;
/*
 * Forget the old editing region
 */
	E_region.mark = 0;
	if (E_region.ov && G_ov_to_ws (E_region.ov) == Wkstn)
		G_clear (E_region.ov);
}



void
edit_set_trace (id_name, xf, yf, color)
char	*id_name;
fldtype	xf, yf;
int	color;
/*
 * Declare a trace, specifying the sounding id, x variable and y variable
 */
{
/*
 * If we're doing an editor replot, just return
 */
	if (Replot)
		return;
/*
 * Get rid of the old sounding id name (if any)
 */
	if (Trace[Ntrace].id)
		free (Trace[Ntrace].id);
/*
 * Put in the sounding name
 */
	Trace[Ntrace].id = (char *) malloc ((1 + strlen (id_name)) * 
		sizeof (char));
	strcpy (Trace[Ntrace].id, id_name);
/*
 * Put in the x and y fields
 */
	Trace[Ntrace].xfld = xf;
	Trace[Ntrace].yfld = yf;
/*
 * Put in the color
 */
	Trace[Ntrace].color = color;
/*
 * See if the current edit field is in this trace
 */
	if (Etrace < 0 && !strcmp (id_name, Eid) && 
		(Efld == xf || Efld == yf))
	{
		Etrace = Ntrace;
	/*
	 * Remove the mark, for now.  In the future, this should draw the 
	 * current edit region
	 */
		E_region.mark = 0;
	/*
	 * Put up the cursor
	 */
	}
			
/*
 * Increment the number of traces
 */
	Ntrace++;
}



void
edit_mark ()
/*
 * Set the mark for the edit region
 */
{
/*
 * Make sure a SELECT has been done
 */
	if (! E_datum)
		ui_error ("You must do a SELECT before setting the mark");
/*
 * Set the mark
 */
	E_region.mark = E_datum;
	E_region.current = 0;
	E_region.lastdraw = 0;
/*
 * If the selected field is not in the current plot, return now
 */
	if (Etrace < 0)
		return;
/*
 * If the pointer is on a plottable point, throw the point into the edit
 * region plotting array
 */
	if (E_datum == Cursor_pri)
	{
		float	xval, yval;
	/*
	 * Get the x and y values
	 */
		if (Editing_x)
		{
			xval = Cursor_pri->value;
			yval = Cursor_sec->value;
		}
		else
		{
			xval = Cursor_sec->value;
			yval = Cursor_pri->value;
		}
	/*
	 * Get the screen relative (0.0-1.0) coordinates of the point and
	 * put them into the E_region plot arrays
	 */
		(*Lastplot.coords) (Trace[Etrace].xfld, Trace[Etrace].yfld,
			xval, yval, &E_region.x[0], &E_region.y[0]);
	}
}



void
edit_cut ()
/*
 * Cut points out of the selected field between the mark and the
 * current pointer position.
 */
{
	struct snd_datum	*start, *finish, *datum, *next;
/*
 * Make sure we have a region
 */
	if (! E_region.mark || E_datum == E_region.mark)
		ui_error ("There is no edit region");
/*
 * Change the links to cut the points
 */
	if (E_datum->index > E_region.mark->index)
	{
	/*
	 * Save the start and finish of the cut region
	 */
		start = E_region.mark->next;
		finish = E_datum;
	/*
	 * Change the links
	 */
		E_region.mark->next = E_datum;
		E_datum->prev = E_region.mark;
	}
	else
	{
	/*
	 * Save the start and finish of the cut region
	 */
		start = E_datum->next;
		finish = E_region.mark;
	/*
	 * Change the links
	 */
		E_region.mark->prev = E_datum;
		E_datum->next = E_region.mark;
	}
/*
 * Free the cut points
 */
	for (datum = start; datum != finish; datum = next)
	{
		next = datum->next;
		cfree (datum);
	}
/*
 * Clear the editing region, leaving the pointer in its current location
 */
	E_region.mark = 0;
	if (Wkstn && G_ov_to_ws (E_region.ov) == Wkstn)
		G_clear (E_region.ov);
/*
 * Redraw the last plot if necessary
 */
	if (Etrace >= 0)
	{
		Replot = TRUE;
		(*Lastplot.plot) (Lastplot.cmds);
		Replot = FALSE;
	}
}




void
edit_erase ()
/*
 * Delete the point under the pointer
 */
{
	struct ui_command	cmd;
	struct snd_datum		*erasept;
/*
 * Make sure a SELECT has been done
 */
	if (! E_datum)
		ui_error ("No edit field has been SELECTed");
/*
 * Save the points under the pointer, since we move the pointer before
 * we use them.
 */
	erasept = E_datum;
/*
 * We need to build a UI command structure to tell edit_up_pointer
 * or edit_down_pointer how far to move.
 */
	cmd.uc_ctype = UTT_VALUE;
	cmd.uc_vptype = SYMT_INT;
	UINT (cmd) = 1;
/*
 * Try to move the pointer up a point.  If that fails, try moving down.
 * Otherwise, we're deleting the last point in the trace, and we don't
 * want to do that.
 */
	edit_up_pointer (&cmd);
	if (erasept == E_datum)
	{
		edit_down_pointer (&cmd);
		if (erasept == E_datum)
			ui_error ("The last point cannot be erased");
	}
/*
 * Move the mark to the current pointer location if it's 
 * on the point to be erased
 */
	if (E_region.mark && E_region.mark->index == erasept->index)
		edit_mark ();
/*
 * Delete the point
 */
	edit_unpoint (Eid, erasept, Efld);
/*
 * Redraw the last plot if necessary
 */
	if (Etrace >= 0)
	{
		Replot = TRUE;
		(*Lastplot.plot) (Lastplot.cmds);
		Replot = FALSE;
	}
/*
 * Done
 */
	return;
}




void
edit_newvalue (cmds)
struct ui_command	*cmds;
/*
 * Put a new value in the point under the pointer
 */
{
	float	oldval, newval = UFLOAT (cmds[0]);
/*
 * Make sure a SELECT has been done
 */
	if (! E_datum)
		ui_error ("No edit field has been SELECTed");
/*
 * Put in the new value
 */
	oldval = E_datum->value;
	E_datum->value = newval;

	ui_printf ("    Replaced value %.2f with %.2f", oldval, newval);
/*
 * Redraw the last plot if necessary
 */
	if (Etrace >= 0)
	{
		Replot = TRUE;
		(*Lastplot.plot) (Lastplot.cmds);
		Replot = FALSE;
	}
/*
 * Make sure the cursor moves if necessary
 */
	if (E_datum == Cursor_pri)
		edit_draw_cursor ();
/*
 * Done
 */
	return;
}




void
edit_examine ()
/*
 * Show the point under the pointer and a few around it
 */
{
	int	i, n_prev = 0;
	struct snd_datum	*pri = E_datum, *sec;
	struct snd_datum	*snd_data_ptr ();
	fldtype	secfld;
	char	*fd_name ();
/*
 * Make sure a SELECT has been done
 */
	if (! E_datum)
		ui_error ("You must do a SELECT before an EXAMINE");
/*
 * Get a secondary field so we have a reference
 */
	if (Etrace >= 0)
	{
	/*
	 * The edit field is in the plot, so use the reference field used
	 * in the plot
	 */
		if (Editing_x)
			secfld = Trace[Etrace].yfld;
		else
			secfld = Trace[Etrace].xfld;
	}
	else
	{
	/*
	 * Use pres or temp since the field isn't in the current plot
	 */
		if (Efld != f_pres)
			secfld = f_pres;
		else
			secfld = f_temp;
	}
/*
 * Get a pointer to the secondary field data
 */
	sec = snd_data_ptr (Eid, secfld);
/*
 * Find our start position.  We want three points before, if possible,
 * otherwise as many as we can get.
 */
	for (i = 0; i < 3; i++)
	{
		if (! pri->next)
			break;
		else
			pri = pri->next;
	}
/*
 * Position the secondary pointer to an index >= the primary index
 */
	while (sec->next && sec->index < pri->index)
		sec = sec->next;
/*
 * Now go backward, printing values until we get to three points before
 * the pointer position or to the beginning, whichever comes first.
 */
	ui_nf_printf ("\n%9s%9s\n", fd_name (Efld), fd_name (secfld));

	while (n_prev < 3)
	{
	/*
	 * Make sure the secondary index is <= the primary index
	 */
		while (sec->prev && sec->index > pri->index)
			sec = sec->prev;
	/*
	 * Print the values
	 */
		if (pri->index == sec->index)
			ui_nf_printf ("%9.2f%9.2f", pri->value, sec->value);
		else
			ui_nf_printf ("%9.2f      BAD", pri->value);
	/*
	 * Write an asterisk if the point is in the edit region
	 */
		if (E_region.mark && BETWEEN (pri->index, 
				E_region.mark->index, E_datum->index))
			ui_nf_printf (" * ");
	/*
	 * Show if this is the pointer position
	 */
		if (pri == E_datum)
			ui_nf_printf (" <-= pointer ");

		ui_nf_printf ("\n");
	/*
	 * If we're below the pointer, increment n_prev
	 */
		if (pri->index < E_datum->index)
			n_prev++;
	/*
	 * Move back a point in the primary data
	 */
		if (pri->prev)
			pri = pri->prev;
		else
			break;
	}

	ui_printf ("\n");
/*
 * Done
 */
	return;
}




void
edit_draw_cursor ()
/*
 * Put the cursor based on the current Cursor_pri and Cursor_sec
 */
{
	int	code, niter = 0;
	float	xprev = Xcursor, yprev = Ycursor, xsc, ysc, xval, yval;
/*
 * If the field being edited isn't in the plot, return now
 */
	if (Etrace < 0)
		return;
/*
 * Get the x and y values
 */
	if (Editing_x)
	{
		xval = Cursor_pri->value;
		yval = Cursor_sec->value;
	}
	else
	{
		xval = Cursor_sec->value;
		yval = Cursor_pri->value;
	}
/*
 * Get the screen relative coordinates (0.0 - 1.0) based on the current plot.
 */
	(*Lastplot.coords) (Trace[Etrace].xfld, Trace[Etrace].yfld,
		xval, yval, &Xcursor, &Ycursor);
/*
 * Make sure we have a good overlay
 */
	if (Wkstn && (! Cursor_ov || G_ov_to_ws (Cursor_ov) != Wkstn))
		Cursor_ov = G_new_overlay (Wkstn, 0);
/*
 * Clip to make sure the cursor will appear on the screen.
 * This code is taken from clip.c in the RDSS graphics package.
 */
	xsc = Xcursor;
	ysc = Ycursor;
	code = CLIPCODE (xsc, ysc);

	while (code)
	{
		float xd = xprev - xsc, yd = yprev - ysc;

		if (xd == 0.0 || yd == 0.0)
			break;
	/*
	 * If we have been around more than four times, we have a segment
	 * that is not really within the clip window.
	 */
	 	if (++niter > 4)
		{
			xsc = 0.0;
			ysc = 0.0;
			break;
		}
	/*
	 * The math is simplified from that in clip.c since we are
	 * working with reals here instead of integers.
	 */
		if (code & B_LEFT)
		{
			ysc += -xsc * yd / xd;
			xsc = 0.0;
		}
		else if (code & B_RIGHT)
		{
			ysc += yd * (1.0 - xsc) / xd;
			xsc = 1.0;
		}
		else if (code & B_BELOW)
		{
			xsc += -ysc * xd / yd;
			ysc = 0.0;
		}
		else if (code & B_ABOVE)
		{
			xsc += xd * (1.0 - ysc) / yd;
			ysc = 1.0;
		}
	 	code = CLIPCODE (xsc, ysc);
	}
/*
 * Place the cursor
 */
	G_put_target (Cursor_ov, xsc, ysc);
}




void
edit_threshold (cmds)
struct ui_command	*cmds;
/*
 * Threshold a field based on the value of another field
 */
{
	char	*id_name;
	char	*snd_default ();
	fldtype	targetfld, threshfld;
	int	criterion, ndx, i;
	float	compare_val;
	struct snd_datum	*target, *thresh, *nexttarget;
	struct snd_datum	*snd_data_ptr ();
/*
 * Get the target and threshold fields, and the threshold criterion
 */
	targetfld = fd_num (UPTR (cmds[0]));
	threshfld = fd_num (UPTR (cmds[1]));
	criterion = UKEY (cmds[2]);
/*
 * Get the comparison value (if any)
 */
	if (cmds[3].uc_ctype == UTT_VALUE && cmds[3].uc_vptype == SYMT_FLOAT)
	{
		compare_val = UFLOAT (cmds[3]);
		ndx = 4;
	}
	else
		ndx = 3;
/*
 * Get the sounding if specified, otherwise use the default
 */
	if (cmds[ndx].uc_ctype != UTT_END)
		id_name = UPTR (cmds[ndx]);
	else
		id_name = snd_default ();
/*
 * Get pointers to the data linked lists for the threshold and target fields
 */
	thresh = (struct snd_datum *) snd_data_ptr (id_name, threshfld);
	target = (struct snd_datum *) snd_data_ptr (id_name, targetfld);
/*
 * Apply the thresholding
 */
	while (thresh && target)
	{
		if (thresh->index == target->index)
		{
			nexttarget = target->next;
		/*
		 * Switch based on the thresholding being done
		 */
		    switch (criterion)
		    {
			case THR_EQ:
				if (thresh->value == compare_val)
					edit_unpoint (id_name, target, 
						targetfld);
				break;
			case THR_LT:
				if (thresh->value < compare_val)
					edit_unpoint (id_name, target, 
						targetfld);
				break;
			case THR_GT:
				if (thresh->value > compare_val)
					edit_unpoint (id_name, target, 
						targetfld);
				break;
			case THR_LE:
				if (thresh->value <= compare_val)
					edit_unpoint (id_name, target, 
						targetfld);
				break;
			case THR_GE:
				if (thresh->value >= compare_val)
					edit_unpoint (id_name, target, 
						targetfld);
				break;
		    }
		/*
		 * Bump both points
		 */
		    target = nexttarget;
		    thresh = thresh->next;
		}
	/*
	 * For non-matching indices, bump the lower index.
	 */
		else
		{
			if (target->index < thresh->index)
			{
				nexttarget = target->next;
			/*
			 * If we're doing bad value thresholding, remove the
			 * target point now, since there was no threshold point
			 * with a matching index.
			 */
				if (criterion == THR_BAD)
					edit_unpoint (id_name, target, 
						targetfld);

				target = nexttarget;
			}
			else
				thresh = thresh->next;
		}
	}
/*
 * For bad value thresholding, remove the remaining target points, if any
 */
	if (criterion == THR_BAD)
		for (; target; target = nexttarget)
		{
			nexttarget = target->next;
			edit_unpoint (id_name, target, targetfld);
		}
/*
 * If the affected sounding is in the current plot, replot it
 */
	for (i = 0; i < Ntrace; i++)
	{
		char	*snd = Trace[i].id;
		fldtype	xfld = Trace[i].xfld, yfld = Trace[i].yfld;

		if (! strcmp (snd, id_name) && 
			(targetfld == xfld || targetfld == yfld))
		{
			Replot = TRUE;
			(*Lastplot.plot) (Lastplot.cmds);
			Replot = FALSE;
			break;
		}
	}
}




void
edit_unpoint (id, pt, fld)
char	*id;
struct snd_datum	*pt;
fldtype	fld;
/*
 * Remove the offending point and return a pointer to the next datum
 * or the last point in the sounding, if there is no next datum.
 */
{
	int	moveptr = (pt == E_datum);

	if (! pt->prev && ! pt->next)
		ui_error ("Cannot delete the last point from a field");
/*
 * Change the link for the previous point.  If there is no previous point
 * then we are changing the head of the data list and must tell the sounding
 * package.
 */
	if (pt->prev)
		pt->prev->next = pt->next;
	else
		snd_head (id, fld, pt->next);
/*
 * Change the link for the next point
 */
	if (pt->next)
		pt->next->prev = pt->prev;
/*
 * Move the edit pointer if we're deleting the datum it now points to 
 */
	if (moveptr)
	{
		if (pt->next)
			E_datum = pt->next;
		else
			E_datum = pt->prev;
	}
/*
 * Free the point we whacked
 */
	cfree (pt);
}




void
edit_forget (id_name)
char	*id_name;
/*
 * The given sounding is the target of a "forget" command
 * If the sounding is in the current plot, all bets are off.  Forget
 * all about the current editing information.
 */
{
	int	i;

	for (i = 0; i < Ntrace; i++)
	{
	/*
	 * See if any of the traces are from the sounding being forgotten
	 */
		if (! strcmp (Trace[i].id, id_name))
		{
			ui_printf ("Sounding '%s' is in the current plot.\n",
				id_name);
			ui_printf ("A new plot must be made before further editing.\n");
		/*
		 * Forget the traces
		 */
			Ntrace = 0;
			Etrace = -1;
		/*
		 * Forget the edit region
		 */
			E_region.mark = 0;
			if (E_region.ov && G_ov_to_ws (E_region.ov) == Wkstn)
				G_clear (E_region.ov);
		}
	}
/*
 * Done
 */
	return;
}




void
edit_insert (cmds)
struct ui_command	*cmds;
/*
 * Handle the INSERT command
 */
{
	int	newindex, i, nfld = 0;
	struct snd_datum	*datum, *prev, *new;
	float	*vals;
	fldtype *flds;
/*
 * Get the index for the new point, depending on whether we're inserting 
 * above or below
 */
	newindex = E_datum->index;
	if (UKEY (cmds[0]) == KW_ABOVE)
		newindex++;
/*
 * Fields and values to insert
 */
	for (i = 1; cmds[i].uc_ctype != UTT_END; i += 2)
		nfld++;

	flds = (fldtype *) malloc (nfld * sizeof (fldtype));
	vals = (float *) malloc (nfld * sizeof (float));

	for (i = 0; i < nfld; i++)
	{
		flds[i] = fd_num (UPTR (cmds[2*i + 1]));
		vals[i] = UFLOAT (cmds[2*i + 2]);
	}
/*
 * Open up a space for the new index in the data lists
 */
	snd_bump_indices (Eid, newindex);
/*
 * Loop through the fields being inserted
 */
	for (i = 0; i < nfld; i++)
	{
	/*
	 * Get the data pointer for this field
	 */
		datum = (struct snd_datum *) snd_data_ptr (Eid, flds[i]);
	/*
	 * Find the points to insert between
	 */
		prev = (struct snd_datum *) 0;

		while (datum && datum->index < newindex)
		{
			prev = datum;
			datum = datum->next;
		}
	/*
	 * Create the new datum
	 */
		new = (struct snd_datum *) malloc (sizeof (struct snd_datum));
		new->index = newindex;
		new->value = vals[i];
		new->prev = prev;
		new->next = datum;
	/*
	 * Insert it into the linked list
	 */
		if (prev)
			prev->next = new;
		else
			snd_head (Eid, flds[i], new);

		if (datum)
			datum->prev = new;
	}
/*
 * Redraw the last plot if necessary
 */
	if (Etrace >= 0)
	{
		Replot = TRUE;
		(*Lastplot.plot) (Lastplot.cmds);
		Replot = FALSE;
	}
}
