/*
 * Routines for handling boolean flags
 * (stolen from ROBOT's flag.c)
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

static char *rcsid = "$Id: flags.c,v 1.9 2004-06-02 21:57:21 burghart Exp $";

# include "globals.h"
# include "flags.h"

# define ___	0

/*
 * Our data structure for dealing with flags.
 */
struct flgst
{
	char	*flg_name;	/* Name of this flag	*/
	bool	*flg_value;	/* Pointer to the flag	*/
	char	flg_default;	/* Default value for this flag	*/
	char	*flg_desc;	/* Description of this flag	*/
} Flg_tbl[] =
{
	/* name		value		default	*/
	/*	description  */
	{ "winds",	&Flg_winds,	TRUE,
		"Put winds on skew-t plots"},
	{ "mli",	&Flg_mli,	TRUE,
		"Use modified lifted index"},
	{ "lift",	&Flg_lift,	TRUE,
		"Show lifted parcel on skew-t"},
	{ "theta_w",	&Flg_theta_w,	TRUE,
		"Use equivalent wet-bulb temperature on skew-t"},
	{ "vt",		&Flg_vt,	FALSE,
		"Use virtual temperature for analysis and skew-t"},
	{ "hodo_msl",	&Flg_hodo_msl,	TRUE,
		"Use MSL altitudes for hodographs"},
	{ "wbarb",	&Flg_barb,	TRUE,
		"Plot wind barbs on skew-t plots"},
	{ "logp",	&Flg_logp,	FALSE,
		"Plot Log(P) for xyplot plots"},
	{ "datalim",	&Flg_datalim,	TRUE,
		"Use data to define interpolation limits"},
	{ "lsqfit",	&Flg_lsqfit,	FALSE,
		"Use least squares fit for interpolation "},
	{ "xs_annotate",&Flg_annotate,	TRUE,
		"Annotate on xsect plot "},
	{ "oldCLASS",	&Flg_oldCLASS,	FALSE,
		"Write files in old CLASS format "},
	{ "mflux_u",	&Flg_uwind,	TRUE,
		"Compute moisture flux in u-wind direction "},
	{ "wmoQuiet",	&Flg_wmoQuiet,	TRUE,
		"Suppress verbose messages about problems in WMO files"},
	{ ___,		___,		___,		___ }
};




flg_def ()
/*
 * Set all the flags to their default values.
 */
{
	struct flgst *f;
	stbl table = usy_g_stbl ("ui$variable_table");

	for (f = Flg_tbl; f->flg_name; f++)
	{
		*f->flg_value = f->flg_default;
		usy_c_indirect (table, f->flg_name, f->flg_value,
			SYMT_BOOL, 0);
	}
}




flg_list ()
/*
 * List off the value of all the flags.
 */
{
	struct flgst *f;

	ui_nf_printf ("\nFlag settings: \n");
	ui_nf_printf ("          Name      Value        Description\n");
	ui_printf ("        --------    ------       -----------\n");
	for (f = Flg_tbl; f->flg_name; f++)
		ui_printf ("        %8.8s     %5.5s       %s\n", f->flg_name,
				*f->flg_value ? "True" : "False", f->flg_desc);
}
