/*
 * Fields handling
 *
 * $Revision: 1.3 $ $Date: 1990-01-23 09:16:12 $ $Author: burghart $
 * 
 */
# include <ui.h>
# include "fields.h"

static struct
{
	fldtype	fld;
	char	*desc, *units;
	char	*aliases[5];
	float	center, step;
} Fnamelist[] =
{
	{f_alt, "altitude AGL", "m",
		{"alt", "altitude", ""},
		0.0,	500.0	},
	{f_dp, "dewpoint", "deg C",
		{"dp", "dewpoint", ""},
		-10.0,	5.0	},
	{f_lat, "latitude", "deg",
		{"lat", "latitude", ""},
		0.0,	1.0	},
	{f_lon,	"longitude", "deg",
		{"lon", "longitude", ""},
		0.0,	1.0	},
	{f_mr, "mixing ratio", "g/kg",
		{"mr", ""},
		5.0,	1.0	},
	{f_pres, "pressure", "mb",
		{"pres", "press", "pressure", ""},
		1000.0,	100.0	},
	{f_qpres, "pressure quality", "",
		{"qpres", ""},
		0.0,	0.1	},
	{f_qrh, "RH quality", "",
		{"qrh", ""},
		0.0,	0.1	},
	{f_qtemp, "temperature quality", "",
		{"qtemp", ""},
		0.0,	0.1	},
	{f_qu, "u_wind quality", "",
		{"qu", ""},
		0.0,	0.1	},
	{f_qv, "v_wind quality", "",
		{"qv", ""},
		0.0,	0.1	},
	{f_qwind, "wind quality", "",
		{"qwind", ""},
		0.0,	0.1	},
	{f_rh, "relative humidity", "%",
		{"rh", ""},
		50.0,	10.0	},
	{f_rtype, "record type", "",
		{"rtype", ""},
		0.0,	1.0	},
	{f_temp, "temperature", "deg C",
		{"temp", "temperature", "tdry", ""},
		0.0,	5.0	},
	{f_theta, "potential temperature", "deg K",
		{"theta", "pt", ""},
		270.0,	2.0	},
	{f_theta_e, "equivalent potential temperature", "deg K",
		{"theta_e", "ept", ""},
		270.0,	5.0	},
	{f_time, "time from start", "s", 	
		{"time", "seconds", ""},
		0.0,	60.0	},
	{f_u_wind, "u wind component", "m/s",
		{"u_wind", ""},
		0.0,	2.0	},
	{f_v_wind, "v wind component", "m/s",
		{"v_wind", ""},
		0.0,	2.0	},
	{f_wdir, "wind direction", "deg",
		{"wdir", ""},
		180.0,	10.0	},
	{f_wspd, "wind speed", "m/s",
		{"wspd", ""},
		0.0,	1.0	},
	{f_null, "null field", "",
		{""},
		0.0,	0.0	}
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




float
fd_center (fld)
fldtype fld;
/*
 * Return the center value to use for the given field
 */
{
	int	i;

	for (i = 0; Fnamelist[i].fld != f_null; i++)
		if (Fnamelist[i].fld == fld)
			return (Fnamelist[i].center);

	ui_error ("Unknown field type %d", fld);
}




float
fd_step (fld)
fldtype fld;
/*
 * Return the step value to use for the given field
 */
{
	int	i;

	for (i = 0; Fnamelist[i].fld != f_null; i++)
		if (Fnamelist[i].fld == fld)
			return (Fnamelist[i].step);

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

	ui_nf_printf ("\n     FIELD  CENTER    STEP\n");
	ui_nf_printf ("--------------------------\n");
	for (i = 0; Fnamelist[i].fld != f_null; i++)
		ui_nf_printf ("%10s %7.2f %7.2f\n", Fnamelist[i].aliases[0], 
			Fnamelist[i].center, Fnamelist[i].step);

	ui_printf ("\n");
}
