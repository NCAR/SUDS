/*
 * Output device handling
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

static char *rcsid = "$Id: output.c,v 1.3 1991-10-21 21:54:36 burghart Exp $";

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
