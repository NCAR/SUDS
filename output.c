/*
 * Output device handling
 *
 * $Revision: 1.2 $ $Date: 1989-08-02 14:42:20 $ $Author: burghart $
 * 
 */
# include "globals.h"


void
out_output (cmds)
struct	ui_command	*cmds;
/*
 * Handle the output command.  Open a workstation; if we have one already,
 * close it before getting the new one.
 */
{
	int	err;
	void	color_loadmap ();

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

	color_loadmap ();
}



void
out_ws_check ()
/*
 * Make sure we have opened a workstation
 */
{
	void	color_loadmap ();

	if (! Wkstn)
	{
		char	dev[40], type[40], *deftype;
		int	err;
		void	out_colormap ();

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

		color_loadmap ();
	}
}
