/*
 * Fields handling
 *
 * $Revision: 1.2 $ $Date: 1989-11-08 10:52:42 $ $Author: burghart $
 * 
 */
# include "fields.h"

struct fnames
{
	fldtype	fld;
	char	*name;
	float	center, step;
} Fnamelist[] =
{
	{f_time,	"time",		0.0,	60.0	},
	{f_lat,		"lat",		0.0,	1.0	},
	{f_lon,		"lon",		0.0,	1.0	},
	{f_alt,		"alt",		0.0,	10.0	},
	{f_pres,	"pres",		1000.0,	10.0	},
	{f_temp,	"temp",		25.0,	1.0	},
	{f_dp,		"dp",		25.0,	1.0	},
	{f_rh,		"rh",		50.0,	5.0	},
	{f_wspd,	"wspd",		0.0,	1.0	},
	{f_wdir,	"wdir",		180.0,	10.0	},
	{f_u_wind,	"u_wind",	0.0,	2.0	},
	{f_v_wind,	"v_wind",	0.0,	2.0	},
	{f_pt,		"pt",		270.0,	1.0	},
	{f_ept,		"ept",		270.0,	1.0	},
	{f_mr,		"mr",		5.0,	0.5	},
	{f_qpres,	"qpres",	0.0,	0.1	},
	{f_qtemp,	"qtemp",	0.0,	0.1	},
	{f_qrh,		"qrh",		0.0,	0.1	},
	{f_qwind,	"qwind",	0.0,	0.1	},
	{f_rtype,	"rtype",	0.0,	0.1	},
	{f_null,	"null",		0.0,	0.0	}
};




fldtype
fd_num (fname)
char	*fname;
/*
 * Return the field number corresponding to the given name
 */
{
	int	i;

	for (i = 0; Fnamelist[i].fld != f_null; i++)
		if (! strcmp (fname, Fnamelist[i].name))
			return (Fnamelist[i].fld);

	ui_error ("Unknown field '%s'", fname);
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
			return (Fnamelist[i].name);

	ui_error ("Unknown field type %d", fld);
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
