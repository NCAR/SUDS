/*
 * netCDF sounding access
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

static char *rcsid = "$Id: netcdf.c,v 1.21 2002-08-23 23:00:34 burghart Exp $";

# if HAVE_LIBNETCDF

# include <stdlib.h>
# include <time.h>

# include <netcdf.h>
# include <ui.h>
# include "sounding.h"
# include "globals.h"

# define NC_BADVAL	-32768.0


struct fldmatch
{
	fldtype	fld;		/* SUDS field identifier	*/
	char	*nc_name;	/* netCDF name for this field	*/
} Netcdf_tbl[] =
{
	{f_time,	"time_offset"	}, 
	{f_lat,		"lat"		}, 
	{f_lon,		"lon"		},
	{f_alt,		"alt"		},
	{f_pres,	"pres"		},
	{f_temp,	"tdry"		},
	{f_temp,	"temp"		},
	{f_dp,		"dp"		},
	{f_rh,		"rh"		},
	{f_wspd,	"wspd"		},
	{f_wdir,	"wdir"		},
	{f_u_wind,	"u_wind"	},
	{f_v_wind,	"v_wind"	},
	{f_qpres,	"qpres"		},
	{f_qtemp,	"qtemp"		},
	{f_qrh,		"qrh"		},
	{f_qwind,	"qwind"		},
	{f_qu,		"qu"		},
	{f_qv,		"qv"		},
	{f_rtype,	"rtype"		},
	{f_ascent,	"ascent_rate"	},
	{f_mr,		"mr"		},
	{f_azimuth,	"azim"		},
	{f_range,	"range"		},
        {f_vt,		"vt"		},
	{f_null, 	""		}
};

/*
 * The netCDF sounding file
 */
static int	Sfile;




void
nc_read_file (fname, sounding)
char	*fname;
struct snd	*sounding;
/*
 * Get the sounding from the given netCDF file
 */
{
	int	len, i, status, ndx, fld, nflds, id;
	int	v_id[MAXFLDS], dims[MAX_VAR_DIMS], ndims, natts;
        long    zero = 0;
	long	basetime;
	long    npts;
	float	v, flag[MAXFLDS], *fdata;
	double	*ddata, dval;
	void	*dptr;
	nc_type	type;
	char	string[80];
	struct tm		*t;
	struct snd_datum	*pt, *prevpt;
	struct fldmatch	*fm;
/*
 * We accept any of a number of different attributes for the site name
 */
	char *site_attrs[] = 
	{
	    "zebra_platform",
	    "zeb_platform",
	    "platform",
	    "site_name"
	};
	int n_site_attrs = sizeof (site_attrs) / sizeof (char*);
/*
 * Set netCDF errors to non-verbose, non-fatal
 */
	ncopts = 0;
/*
 * Open the file
 */
	if ((Sfile = ncopen (fname, NC_NOWRITE)) == -1)
		ui_error ("Cannot open netCDF file '%s'", fname);
/*
 * Go through the attributes named in the site_attrs array, and accept the
 * first one we find as our site name.
 */
	for (i = 0; i < n_site_attrs; i++)
	{
	    if (ncattinq (Sfile, NC_GLOBAL, site_attrs[i], &type, &len) == -1)
		continue;

	    ncattget (Sfile, NC_GLOBAL, site_attrs[i], (void*) string);

	    sounding->site = (char*) malloc (1 + len);
	    strncpy (sounding->site, string, len);
	    sounding->site[len] = 0;
	    break;
	}

	if (i == n_site_attrs)
	{
	    char *noname = "(unknown)";
	    ui_warning ("Can't find a site name.  Using '%s'.", noname);
	    sounding->site = (char*) malloc (1 + strlen(noname));
	    strcpy (sounding->site, noname);
	}
/*
 * Site lat, lon, alt
 */
	if ((ncvarget1 (Sfile, ncvarid (Sfile, "site_lat"), &zero, &v) == -1)&&
		(ncvarget1 (Sfile, ncvarid (Sfile, "lat"), &zero, &v) == -1))
		ui_error ("Can't find 'site_lat' or 'lat'!", ncerr);
	sounding->sitelat = v;

	if ((ncvarget1 (Sfile, ncvarid (Sfile, "site_lon"), &zero, &v) == -1)&&
		(ncvarget1 (Sfile, ncvarid (Sfile, "lon"), &zero, &v) == -1))
		ui_error ("Can't find 'site_lon' or 'lon'!", ncerr);
	sounding->sitelon = v;

	if ((ncvarget1 (Sfile, ncvarid (Sfile, "site_alt"), &zero, &v) == -1)&&
		(ncvarget1 (Sfile, ncvarid (Sfile, "alt"), &zero, &v) == -1))
		ui_error ("Can't find 'site_alt' or 'alt'!", ncerr);
	sounding->sitealt = v;
/*
 * Sounding release date and time
 */
	if (ncvarget1 (Sfile, ncvarid (Sfile, "base_time"), &zero, 
		&basetime) == -1)
		ui_error ("NetCDF error %d getting 'base_time'!", ncerr);

	t = gmtime (&basetime);

	sounding->rls_time.ds_yymmdd = 10000 * t->tm_year + 
		100 * (t->tm_mon + 1) + t->tm_mday;
	sounding->rls_time.ds_hhmmss = 10000 * t->tm_hour + 100 * t->tm_min + 
		t->tm_sec;
/*
 * Number of points
 */
	if (ncdiminq (Sfile, ncdimid (Sfile, "time"), (char *) 0, 
                &npts) == -1)
		ui_error ("Unable to get number of points in file!");
/*
 * Sounding fields (We can't just ask what fields are available, we have 
 * to check each possible field)
 */
	nflds = 0;

	for (fm = Netcdf_tbl; fm->fld != f_null; fm++)
	{
	/*
	 * Continue if this field isn't in the sounding
	 */
		if ((id = ncvarid (Sfile, fm->nc_name)) == -1)
			continue;
	/*
	 * The field is here, add it to the lists
	 */
		v_id[nflds] = id;
		sounding->fields[nflds] = fm->fld;
	/*
	 * Get the missing value flag for this field
	 */
		if (ncattinq (Sfile, id, "missing_value", &type, &len) == -1)
		{
		    ui_warning ("No missing_value flag for %s, using %.1f",
				fd_name (sounding->fields[nflds]), NC_BADVAL);
		    flag[nflds] = NC_BADVAL;
		}
		else
		{
		    if (type == NC_DOUBLE)
		    {
			ncattget (Sfile, id, "missing_value", (void *)(&dval));
			flag[nflds] = (float) dval;
		    }
		    else
			ncattget (Sfile, id, "missing_value", 
				  (void *)(&flag[nflds]));
		}
		
		nflds++;
	}
/*
 * Null terminate the fields list
 */
	sounding->fields[nflds] = f_null;
/*
 * Get an array to hold the data temporarily
 */
	fdata = (float *) malloc (npts * sizeof (float));
	ddata = (double *) malloc (npts * sizeof (double));
/*
 * Deal with each field
 */
	for (fld = 0; fld < nflds; fld++)
	{
	/*
	 * We only deal with float or double data, dimensioned only by time...
	 */
		status = ncvarinq (Sfile, v_id[fld], NULL, &type, &ndims, 
				   dims, &natts);
		if (status == -1 || ndims != 1 ||
		    (type != NC_FLOAT && type != NC_DOUBLE))
			ui_error ("Dimension or data type problem with '%s'!",
				  fd_name (sounding->fields[fld]));
	/*
	 * Get the data for this field
	 */
		dptr = (type == NC_FLOAT) ? (void *)fdata : (void *)ddata;
		
		status = ncvarget (Sfile, v_id[fld], &zero, &npts, 
				   dptr);

		if (status == -1)
			ui_error ("Error reading %s data from file", 
				fd_name (sounding->fields[fld]));
	/*
	 * Go from double to float if necessary (Of course we're assuming here
	 * that none of the data *require* the extra range available in a
	 * double...)
	 */
		if (type == NC_DOUBLE)
			for (ndx = 0; ndx < npts; ndx++)
				fdata[ndx] = ddata[ndx];
	/*
	 * Build the list of snd_datum structures
	 */
		prevpt = (struct snd_datum *) 0;

		for (ndx = 0; ndx < npts; ndx++)
		{
		/*
		 * Skip bad points
		 */
			if (fdata[ndx] == flag[fld])
				continue;
		/*
		 * Allocate a new structure
		 */
			pt = (struct snd_datum *)
				calloc (1, sizeof (struct snd_datum));
		/*
		 * Link it into the list or make it the head
		 */
			if (prevpt)
			{
				prevpt->next = pt;
				pt->prev = prevpt;
			}
			else
				sounding->dlists[fld] = pt;
		/*
		 * Change altitude to meters
		 */
			if (sounding->fields[fld] == f_alt)
				fdata[ndx] *= 1000;
		/*
		 * Assign the value and index
		 */
			pt->value = fdata[ndx];
			pt->index = ndx;
		/*
		 * Reassign the previous point
		 */
			prevpt = pt;
		}
	}
/*
 * Free the temporary data vectors
 */
	free (fdata);
	free (ddata);
/*
 * Set the max index for these data
 */
	sounding->maxndx = npts - 1;
/*
 * Close the file and return
 */
	ncclose (Sfile);
	return;
}




void
nc_write_file (cmds)
struct ui_command	*cmds;
/*
 * Handle the NETCDF command for writing a netCDF sounding file
 */
{
	int	i, j, offset, nflds, c;
	int	have_time = FALSE, have_lat = FALSE, have_lon = FALSE;
	int	v_sitelat, v_sitelon, v_sitealt, v_base, v_time, v_lat, v_lon;
	int	v_id[MAXFLDS], timedim;
	int	add_alt = FALSE, v_alt, n_alts;
	long	current_time, base_time, zero = 0;
	long    ndx;
	char	*fname, *id_name, *snd_default ();
	struct snd	sounding, snd_find_sounding ();
	struct tm	t;
	float	val, bv = NC_BADVAL, altbuf[BUFLEN];
	char	string[80];
	fldtype	fld, nc_fld;
	struct snd_datum	*data[MAXFLDS];
/*
 * Get the filename then the sounding name
 */
	c = 0;

	fname = UPTR (cmds[c++]);

	if (cmds[c].uc_ctype == UTT_VALUE)
		id_name = UPTR (cmds[c++]);
	else
		id_name = snd_default ();

	sounding = snd_find_sounding (id_name);
/*
 * kluge to add altitude field
 */
	if (cmds[c].uc_ctype == UTT_KW)
		add_alt = TRUE;
/*
 * Set netCDF errors to non-verbose, non-fatal
 */
	ncopts = 0;
/*
 * Put on a ".cdf" extension and open the file
 */
	fixdir_t ("", ".", fname, string, ".cdf");
	if ((Sfile = nccreate (string, NC_NOCLOBBER)) == -1)
	{
		if (access (string, 0) == 0)
			ui_error ("File '%s' already exists", string);
		else
			ui_error ("Cannot create file '%s'", string);
	}
/*
 * Create the time dimension
 */
	timedim = ncdimdef (Sfile, "time", NC_UNLIMITED);
/*
 * Put on a title attribute
 */
	current_time = time (0);
	sprintf (string, "SUDS edited sounding file, created by %s %s",
		getenv ("USER"), ctime (&current_time));

	string[strlen (string) - 1] = '\0';	/* remove trailing \n */

	ncattput (Sfile, NC_GLOBAL, "title", NC_CHAR, strlen (string) + 1, 
		(void *) string);
/*
 * Save the site name as an attribute
 */
	ncattput (Sfile, NC_GLOBAL, "site_name", NC_CHAR, 
		strlen (sounding.site) + 1, (void *) sounding.site);
/*
 * Create our variables
 */
	v_base = ncvardef (Sfile, "base_time", NC_LONG, 0, 0);
	v_sitelat = ncvardef (Sfile, "site_lat", NC_FLOAT, 0, 0);
	v_sitelon = ncvardef (Sfile, "site_lon", NC_FLOAT, 0, 0);
	v_sitealt = ncvardef (Sfile, "site_alt", NC_FLOAT, 0, 0);

	v_time = ncvardef (Sfile, "time_offset", NC_FLOAT, 1, &timedim);
	(void) ncattput (Sfile, v_time, "missing_value", NC_FLOAT, 1, &bv);

	v_lat = ncvardef (Sfile, "lat", NC_FLOAT, 1, &timedim);
	(void) ncattput (Sfile, v_lat, "missing_value", NC_FLOAT, 1, &bv);

	v_lon = ncvardef (Sfile, "lon", NC_FLOAT, 1, &timedim);
	(void) ncattput (Sfile, v_lon, "missing_value", NC_FLOAT, 1, &bv);

	for (i = 0; sounding.fields[i] != f_null; i++)
	{
		fld = sounding.fields[i];
	/*
	 * Search the netCDF fields table for a match
	 */
		for (j = 0; (nc_fld = Netcdf_tbl[j].fld) != f_null; j++)
		{
			if (nc_fld == fld)
				break;
		}
	/*
	 * Did we get a field?
	 */
		if (nc_fld == f_null)
			ui_error ("No netCDF name for field '%s'", 
				fd_name (fld));
	/*
	 * Create the variable in the netCDF file
	 */
		if (fld == f_time)
		{
			v_id[i] = v_time;
			have_time = TRUE;
		}
		else if (fld == f_lat)
		{
			v_id[i] = v_lat;
			have_lat = TRUE;
		}
		else if (fld == f_lon)
		{
			v_id[i] = v_lon;
			have_lon = TRUE;
		}
		else
		{
			v_id[i] = ncvardef (Sfile, Netcdf_tbl[j].nc_name, 
				NC_FLOAT, 1, &timedim);
			(void) ncattput (Sfile, v_id[i], "missing_value",
				NC_FLOAT, 1, &bv);
		}
	}

	nflds = i;
/*
 * Get the data for the altitude kluge
 */
	if (add_alt)
	{
		v_alt = ncvardef (Sfile, "alt", NC_FLOAT, 1, &timedim);
		(void) ncattput (Sfile, v_alt, "missing_value",
			NC_FLOAT, 1, &bv);

		nflds++;

		n_alts = snd_get_data (id_name, altbuf, BUFLEN, f_alt, 
			NC_BADVAL);

		for (i = 0; i < n_alts; i++)
			if (altbuf[i] != NC_BADVAL)
				altbuf[i] *= 0.001;
	}
/*
 * Get out of definition mode
 */
	ncendef (Sfile);
/*
 * We need to know the time zone used for the sounding, so we can put
 * GMT in the netCDF file
 */
	ui_printf ("Enter the offset in hours for conversion to GMT\n");
	offset = ui_int_prompt ("(0=GMT, 7=MST, 6=MDT, etc.)", NULL, -12, 
		12, 0);
/*
 * Put in the base time
 */
	t.tm_year = sounding.rls_time.ds_yymmdd / 10000;
	t.tm_mon = ((sounding.rls_time.ds_yymmdd / 100) % 100) - 1;
	t.tm_mday = sounding.rls_time.ds_yymmdd % 100;
	t.tm_hour = sounding.rls_time.ds_hhmmss / 10000;
	t.tm_min = (sounding.rls_time.ds_hhmmss / 100) % 100;
	t.tm_sec = sounding.rls_time.ds_hhmmss % 100;

        putenv ("TZ=UTC");

        timezone = 0;
        daylight = 0;
        t.tm_wday = t.tm_yday = 0;
        t.tm_isdst = -1;
        base_time = (long) mktime (&t) + offset * 3600;

	ncvarput1 (Sfile, v_base, 0, &base_time);
/*
 * If we don't have an offset time field, put in zeros now
 */
	if (! have_time)
		for (ndx = 0; ndx <= sounding.maxndx; ndx++)
		{
			float	faketime = 0.1 * ndx;
			ncvarput1 (Sfile, v_time, &ndx, &faketime);
		}
/*
 * If we don't have latitude or longitude, fill in with the site lat and/or lon
 */
	if (! have_lat)
		for (ndx = 0; ndx <= sounding.maxndx; ndx++)
			ncvarput1 (Sfile, v_lat, &ndx, 
                                       &sounding.sitelat);

	if (! have_lon)
		for (ndx = 0; ndx <= sounding.maxndx; ndx++)
			ncvarput1 (Sfile, v_lon, &ndx, 
                                       &sounding.sitelon);
/*
 * Site lat, lon, altitude
 */
	ncvarput1 (Sfile, v_sitelat, 0, &sounding.sitelat);
	ncvarput1 (Sfile, v_sitelon, 0, &sounding.sitelon);
	ncvarput1 (Sfile, v_sitealt, 0, &sounding.sitealt);
/*
 * Establish our pointers into the data lists
 */
	for (i = 0; i < nflds; i++)
		data[i] = sounding.dlists[i];
/*
 * Write the data
 */
	for (ndx = 0; ndx <= sounding.maxndx; ndx++)
	{
	/*
	 * Look for a value for each field at this index
	 */
		for (i = 0; i < nflds; i++)
		{
			fld = sounding.fields[i];
		/*
		 * If the index matches, get the value and move to the next
		 * datum in the list for this field, otherwise put in a bad 
		 * value
		 */
			if (data[i] && data[i]->index == ndx)
			{
				val = data[i]->value;
				data[i] = data[i]->next;
			/*
			 * Adjust altitude to km
			 */
				if (fld == f_alt)
					val *= 0.001;
			}
			else
				val = NC_BADVAL;
		/*
		 * Write the datum
		 */
			ncvarput1 (Sfile, v_id[i], &ndx, (void *)(&val));
		}

		if (add_alt)
			ncvarput1 (Sfile, v_alt, ndx, (void *)(altbuf + ndx));
	}
/*
 * Close the file
 */
	ncclose (Sfile);
}


# else	/* NetCDF not enabled */

# include <ui.h>
# include "sounding.h"


void
nc_read_file (fname, sounding)
char	*fname;
struct snd	*sounding;
{
	ui_error ("NetCDF functions not enabled on your system");
}


void
nc_write_file (cmds)
struct ui_command	*cmds;
{
	ui_error ("NetCDF functions not enabled on your system");
}


# endif /* HAVE_LIBNETCDF */
