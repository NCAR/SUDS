/*
 * Fields handling
 *
 * $Log: not supported by cvs2svn $
 */
# include "fields.h"

struct fnames
{
	fldtype	fld;
	char	*name;
} Fnamelist[] =
{
	{f_time,	"time"	},
	{f_lat,		"lat"	},
	{f_lon,		"lon"	},
	{f_alt,		"alt"	},
	{f_pres,	"pres"	},
	{f_temp,	"temp"	},
	{f_dp,		"dp"	},
	{f_rh,		"rh"	},
	{f_wspd,	"wspd"	},
	{f_wdir,	"wdir"	},
	{f_u_wind,	"u_wind"},
	{f_v_wind,	"v_wind"},
	{f_pt,		"pt"	},
	{f_ept,		"ept"	},
	{f_mr,		"mr"	},
	{f_qpres,	"qpres"	},
	{f_qtemp,	"qtemp"	},
	{f_qrh,		"qrh"	},
	{f_qwind,	"qwind"	},
	{f_rtype,	"rtype"	},
	{f_null,	"null"	}
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

