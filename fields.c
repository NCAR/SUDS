/*
 * Fields handling
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

static char *rcsid = "$Id: fields.c,v 1.15 1993-03-12 18:41:36 burghart Exp $";

# include <ui.h>
# include "fields.h"

static struct
{
	fldtype	fld;
	char	*desc, *units;
	char	*aliases[5];
	float	bot, top;
	float	center, step;
} Fnamelist[] =
{
	{f_alt, "altitude MSL", "m",
		{"alt", "altitude", ""},
		0.0,	16000.0,	0.0,	500.0	},
	{f_ascent, "ascent rate", "m/s",
		{"ascent", "dz/dt", ""},
		0.0,	10.0,	0.0,	1.0	},
	{f_azimuth, "balloon azimuth", "deg",
		{"azimuth", "az", ""},
		0.0,	360.0,	0.0,	10.0	},
	{f_dp, "dewpoint", "C",
		{"dp", "dewpoint", ""},
		-70.0,	30.0,	-10.0,	5.0	},
	{f_lat, "latitude", "deg",
		{"lat", "latitude", ""},
		-90.0,	90.0,	0.0,	1.0	},
	{f_lon,	"longitude", "deg",
		{"lon", "longitude", ""},
		-180.0,	180.0,	0.0,	1.0	},
	{f_mr, "mixing ratio", "g/kg",
		{"mr", ""},
		0.0,	20.0,	5.0,	1.0	},
	{f_pres, "pressure", "mb",
		{"pres", "press", "pressure", ""},
		1050.0,	100.0,	1000.0,	100.0	},
	{f_qpres, "pressure quality", "",
		{"qpres", "qp", ""},
		0.0,	10.0,	0.0,	0.1	},
	{f_qrh, "RH quality", "",
		{"qrh", ""},
		0.0,	10.0,	0.0,	0.1	},
	{f_qtemp, "temperature quality", "",
		{"qtemp", "qt", ""},
		0.0,	10.0,	0.0,	0.1	},
	{f_qu, "u_wind quality", "",
		{"qu", ""},
		0.0,	10.0,	0.0,	0.1	},
	{f_qv, "v_wind quality", "",
		{"qv", ""},
		0.0,	10.0,	0.0,	0.1	},
	{f_qwind, "wind quality", "",
		{"qwind", "qdz", ""},
		0.0, 	10.0,	0.0,	0.1	},
	{f_range, "balloon range", "km",
		{"range", ""},
		0.0,	10000.0,	0.0,	1.0	},
	{f_rh, "relative humidity", "percent",
		{"rh", ""},
		0.0,	100.0,	50.0,	10.0	},
	{f_rtype, "record type", "",
		{"rtype", ""},
		0.0,	10.0,	0.0,	1.0	},
	{f_temp, "temperature", "C",
		{"temp", "temperature", "tdry", ""},
		-60.0,	40.0,	0.0,	5.0	},
	{f_theta, "potential temperature", "K",
		{"theta", "pt", ""},
		300.0,	350.0,	300.0,	5.0	},
	{f_theta_e, "equivalent potential temperature", "K",
		{"theta_e", "ept", ""},
		300.0,	350.0,	300.0,	5.0	},
	{f_theta_v, "virtual potential temperature", "K", 
		{"theta_v", "vpt", ""},
		300.0, 350.0, 300.0, 5.0	},
	{f_time, "time from start", "s", 	
		{"time", "seconds", ""},
		0.0,	5000.0,	0.0,	60.0	},
	{f_u_prime, "u' wind component", "m/s",
		{"u_prime", ""},
		-30.0,	30.0,	0.0,	5.0	},
	{f_u_wind, "u wind component", "m/s",
		{"u_wind", ""},
		-30.0,	30.0,	0.0,	5.0	},
	{f_v_prime, "v' wind component", "m/s",
		{"v_prime", ""},
		-30.0,	30.0,	0.0,	5.0	},
	{f_v_wind, "v wind component", "m/s",
		{"v_wind", ""},
		-30.0,	30.0,	0.0,	5.0	},
	{f_vt, "virtual temperature", "K",
		{"vt", "t_v", ""},
		220.0,	320.0,	270.0,	5.0	},
	{f_wdir, "wind direction", "deg",
		{"wdir", ""},
		0.0,	360.0,	180.0,	10.0	},
	{f_wspd, "wind speed", "m/s",
		{"wspd", ""},
		0.0,	50.0,	0.0,	5.0	},
	{f_qrh, "wind speed", "m/s",
		{"wspd", ""},
		0.0,	50.0,	0.0,	5.0	},
	{f_null, "null field", "",
		{""},
		0.0,	0.0,	0.0,	0.0	}
};




fldtype
fd_num (fname)
char	*fname;
/*
 * Return the field number corresponding to the given name
 */
{
	int	i, j;

	for (i = 0; Fnamelist[i].fld != f_null; i++)
		for (j = 0; *Fnamelist[i].aliases[j]; j++)
			if (! strcmp (fname, Fnamelist[i].aliases[j]))
				return (Fnamelist[i].fld);

	ui_error ("Unknown field '%s'", fname);
}




char *
fd_desc (fld)
fldtype	fld;
/*
 * Return the field description corresponding to the given field
 */
{
	int	i;

	for (i = 0; Fnamelist[i].fld != f_null; i++)
		if (Fnamelist[i].fld == fld)
			return (Fnamelist[i].desc);

	ui_error ("Unknown field type %d in fd_desc", fld);
}




char *
fd_name (fld)
fldtype	fld;
/*
 * Return the field name corresponding to the given field
 */
{
	int	i;

	for (i = 0; Fnamelist[i].fld != f_null; i++)
		if (Fnamelist[i].fld == fld)
			return (Fnamelist[i].aliases[0]);

	ui_error ("Unknown field type %d in fd_name", fld);
}




char *
fd_units (fld)
fldtype	fld;
/*
 * Return the units corresponding to the given field
 */
{
	int	i;

	for (i = 0; Fnamelist[i].fld != f_null; i++)
		if (Fnamelist[i].fld == fld)
			return (Fnamelist[i].units);

	ui_error ("Unknown field type %d in fd_units", fld);
}




double
fd_bot (fld)
fldtype fld;
/*
 * Return the bottom value to use for the given field
 */
{
	int	i;

	for (i = 0; Fnamelist[i].fld != f_null; i++)
		if (Fnamelist[i].fld == fld)
			return ((double) Fnamelist[i].bot);

	ui_error ("Unknown field type %d", fld);
}




double
fd_top (fld)
fldtype fld;
/*
 * Return the top value to use for the given field
 */
{
	int	i;

	for (i = 0; Fnamelist[i].fld != f_null; i++)
		if (Fnamelist[i].fld == fld)
			return ( (double) Fnamelist[i].top);

	ui_error ("Unknown field type %d", fld);
}




double
fd_center (fld)
fldtype fld;
/*
 * Return the center value to use for the given field
 */
{
	int	i;

	for (i = 0; Fnamelist[i].fld != f_null; i++)
		if (Fnamelist[i].fld == fld)
			return ( (double) Fnamelist[i].center);

	ui_error ("Unknown field type %d", fld);
}




double
fd_step (fld)
fldtype fld;
/*
 * Return the step value to use for the given field
 */
{
	int	i;

	for (i = 0; Fnamelist[i].fld != f_null; i++)
		if (Fnamelist[i].fld == fld)
			return ( (double) Fnamelist[i].step);

	ui_error ("Unknown field type %d", fld);
}



void
fd_set_limits (cmds)
struct ui_command	*cmds;
/*
 * Change the plot limits for a field
 */
{
	int	i, j;
	char	*fname = UPTR (cmds[0]);

	for (i = 0; Fnamelist[i].fld != f_null; i++)
		for (j = 0; *Fnamelist[i].aliases[j]; j++)
			if (! strcmp (fname, Fnamelist[i].aliases[j]))
			{
				Fnamelist[i].bot = UFLOAT (cmds[1]);
				Fnamelist[i].top = UFLOAT (cmds[2]);
				return;
			}

	ui_error ("Unknown field '%s'", fname);
}




void
fd_set_conlimits (cmds)
struct ui_command	*cmds;
/*
 * Change the plot limits for a field
 */
{
	int	i, j;
	char	*fname = UPTR (cmds[0]);

	for (i = 0; Fnamelist[i].fld != f_null; i++)
		for (j = 0; *Fnamelist[i].aliases[j]; j++)
			if (! strcmp (fname, Fnamelist[i].aliases[j]))
			{
				Fnamelist[i].center = UFLOAT (cmds[1]);
				Fnamelist[i].step = UFLOAT (cmds[2]);
				return;
			}

	ui_error ("Unknown field '%s'", fname);
}




void
fd_show_limits ()
/*
 * Show the current limits for all fields
 */
{
	int	i;

	ui_nf_printf ("\n     FIELD   BOTTOM      TOP   CENTER     STEP\n");
	ui_nf_printf ("----------------------------------------------\n");
	for (i = 0; Fnamelist[i].fld != f_null; i++)
		ui_nf_printf ("%10s %8.2f %8.2f %8.2f %8.2f\n", 
			Fnamelist[i].aliases[0], Fnamelist[i].bot, 
			Fnamelist[i].top, Fnamelist[i].center, 
			Fnamelist[i].step);

	ui_printf ("\n");
}
