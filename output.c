/*
 * Output device handling
 *
 * $Log: not supported by cvs2svn $
 */
# include "globals.h"
# include "color.h"

# define MIN(x,y)	((x) < (y) ? (x) : (y))


void
out_output (cmds)
struct	ui_command	*cmds;
/*
 * Handle the output command.  Open a workstation; if we have one already,
 * close it before getting the new one.
 */
{
	int	err;
	void	out_color_init ();

	if (Wkstn)
		G_close (Wkstn);

	err = G_open (UPTR (cmds[0]), UPTR (cmds[1]), &Wkstn, 0);
	if (err)
	{
		Wkstn = 0;
		ui_error ("Cannot open device '%s' of type '%s'", 
			UPTR (cmds[0]), UPTR (cmds[1]));
	}

	strcpy (Out_dev, UPTR(cmds[0]));
	strcpy (Dev_type, UPTR(cmds[1]));

	out_color_init ();
}



void
out_ws_check ()
/*
 * Make sure we have opened a workstation
 */
{
	if (! Wkstn)
	{
		char	dev[40], type[40], *deftype;
		int	err;
		void	out_color_init ();

		ui_warning ("No output device has been specified");
		ui_string_prompt ("Enter output device name", 0, dev, "TT");
		if (! strncmp (dev, "rm", 2))
			deftype = "rmimg1";
		else if (! strcmp (dev, "null"))
			deftype = "null";
		else
			deftype = "4107";

		ui_string_prompt ("Device type", 0, type, deftype);

		err = G_open (dev, type, &Wkstn, 0);
		if (err)
			ui_error ("Cannot open device '%s' of type '%s'", 
				dev, type);

		out_color_init ();
	}
}



void
out_color_init ()
/*
 * Initialize the colors on our workstation
 */
{
	int	ncolor, base, i;
	void	out_dv_color (), out_dv_mono ();

	G_w_inquire (Wkstn, GIW_NCOLOR, &ncolor);
/*
 * Allocate color space and set the color map
 */
	G_get_color (Wkstn, MIN (ncolor, 12), &Colorbase);
	G_set_color_map (Wkstn, Colorbase, MIN (ncolor, 12), Red, 
		Green, Blue);
}
