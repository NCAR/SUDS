/*
 * netCDF sounding access
 *
 * $Revision: 1.1 $ $Date: 1990-12-11 11:28:30 $ $Author: burghart $
 * 
 */
# include <time.h>
# include <netcdf.h>
# include <ui.h>
# include "sounding.h"

# define NC_BADVAL	-32768.0


struct fldmatch
{
	fldtype	fld;		/* SUDS field identifier	*/
	char	*nc_name;	/* netCDF name for this field	*/
} Netcdf_tbl[] =
{
	{f_time,	"offset_time"	}, 
	{f_lat,		"lat"		}, 
	{f_lon,		"lon"		},
	{f_alt,		"alt"		},
	{f_pres,	"pres"		},
	{f_temp,	"tdry"		},
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
	{f_dz,		"ascent_rate"	},
	{f_mr,		"mr"		},
	{f_azimuth,	"azim"		},
	{f_range,	"range"		},
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
	int	len, status, ndx, npts, fld, nflds, id;
	int	v_id[MAXFLDS], zero = 0;
	long	basetime;
	float	val, flag[MAXFLDS], *data;
	char	string[80];
	struct tm		*t;
	struct snd_datum	*pt, *prevpt;
	struct fldmatch	*fm;
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
 * Get the site name
 */
	status = ncattget (Sfile, NC_GLOBAL, "site_name", (void *) string);
	if (status == -1)
		ui_error ("NetCDF error %d getting 'site_name'!", ncerr);

	sounding->site = (char *) 
		malloc ((1 + strlen (string)) * sizeof (char));
	strcpy (sounding->site, string);
/*
 * Site lat, lon, alt
 */
	status = ncvarget1 (Sfile, ncvarid (Sfile, "site_lat"), 0, &val);
	if (status == -1)
		ui_error ("NetCDF error %d getting 'site_lat'!", ncerr);
	sounding->sitelat = val;

	status = ncvarget1 (Sfile, ncvarid (Sfile, "site_lon"), 0, &val);
	if (status == -1)
		ui_error ("NetCDF error %d getting 'site_lon'!", ncerr);
	sounding->sitelon = val;

	status = ncvarget1 (Sfile, ncvarid (Sfile, "site_alt"), 0, &val);
	if (status == -1)
		ui_error ("NetCDF error %d getting 'site_alt'!", ncerr);
	sounding->sitealt = val;
/*
 * Sounding release date and time
 */
	status = ncvarget1 (Sfile, ncvarid (Sfile, "base_time"), 0, &basetime);
	if (status == -1)
		ui_error ("NetCDF error %d getting 'base_time'!", ncerr);

	t = gmtime (&basetime);

	sounding->rls_time.ds_yymmdd = 10000 * t->tm_year + 
		100 * (t->tm_mon + 1) + t->tm_mday;
	sounding->rls_time.ds_hhmmss = 10000 * t->tm_hour + 100 * t->tm_min + 
		t->tm_sec;
/*
 * Number of points
 */
	status = ncdiminq (Sfile, ncdimid (Sfile, "time"), (char *) 0, &npts);
	if (status == -1)
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
		status = ncattget (Sfile, id, "missing_value", 
			(void *)(&flag[nflds]));
		if (status == -1)
		{
			ui_warning ("No missing value flag for %s, using %.1f",
				fd_name (sounding->fields[nflds]), NC_BADVAL);
			flag[nflds] = NC_BADVAL;
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
	data = (float *) malloc (npts * sizeof (float));
/*
 * Deal with each field
 */

	for (fld = 0; fld < nflds; fld++)
	{
	/*
	 * Get the data for this field
	 */
		status = ncvarget (Sfile, v_id[fld], &zero, &npts, 
			(void *) data);
		if (status == -1)
			ui_error ("Error reading %s data from file", 
				fd_name (sounding->fields[fld]));
	/*
	 * Build the list of snd_datum structures
	 */
		prevpt = (struct snd_datum *) 0;

		for (ndx = 0; ndx < npts; ndx++)
		{
		/*
		 * Skip bad points
		 */
			if (data[ndx] == flag[fld])
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
		 * Change altitude to meters AGL
		 */
			if (sounding->fields[fld] == f_alt)
			{
				data[ndx] *= 1000;
				data[ndx] -= sounding->sitealt;
			}
		/*
		 * Assign the value and index
		 */
			pt->value = data[ndx];
			pt->index = ndx;
		/*
		 * Reassign the previous point
		 */
			prevpt = pt;
		}
	}
/*
 * Free the temporary data vector
 */
	free (data);
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
	int	i, j, offset, ndx, nflds, have_time = FALSE;
	int	v_sitelat, v_sitelon, v_sitealt, v_base, v_time;
	int	v_id[MAXFLDS], timedim;
	long	current_time, base_time;
	char	*fname, *id_name, *snd_default ();
	struct snd	sounding, snd_find_sounding ();
	struct tm	t;
	float	val, bv = NC_BADVAL, zero = 0.0;
	char	string[80];
	fldtype	fld, nc_fld;
	struct snd_datum	*data[MAXFLDS];
/*
 * Get the sounding name then the sounding
 */
	fname = UPTR (cmds[0]);

	if (cmds[1].uc_ctype != UTT_END)
		id_name = UPTR (cmds[1]);
	else
		id_name = snd_default ();

	sounding = snd_find_sounding (id_name);
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

	for (i = 0; sounding.fields[i] != f_null; i++)
	{
		fld = sounding.fields[i];
	/*
	 * Search the netCDF fields table for a match
	 */
		for (j = 0; Netcdf_tbl[j].fld != f_null; j++)
		{
			nc_fld = Netcdf_tbl[j].fld;
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
		if (fld != f_time)
			v_id[i] = ncvardef (Sfile, Netcdf_tbl[j].nc_name, 
				NC_FLOAT, 1, &timedim);
		else
		{
			v_id[i] = v_time;
			have_time = TRUE;
		}

		(void) ncattput (Sfile, v_id[i], "missing_value",
			NC_FLOAT, 1, &bv);
	}

	nflds = i;
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
	t.tm_mon = (sounding.rls_time.ds_yymmdd / 100) % 100;
	t.tm_mday = sounding.rls_time.ds_yymmdd % 100;
	t.tm_hour = sounding.rls_time.ds_hhmmss / 10000;
	t.tm_min = (sounding.rls_time.ds_hhmmss / 100) % 100;
	t.tm_sec = sounding.rls_time.ds_hhmmss % 100;
	t.tm_gmtoff = offset * 3600;	/* convert to seconds */
	t.tm_zone = (char *) 0;
	t.tm_wday = t.tm_isdst = t.tm_yday = 0;

	base_time = timegm (&t);
	ncvarput1 (Sfile, v_base, 0, &base_time);
/*
 * If we don't have an offset time field, put in zeros now
 */
	if (! have_time)
		for (ndx = 0; ndx <= sounding.maxndx; ndx++)
			ncvarput1 (Sfile, v_time, &ndx, &zero);
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
			 * Adjust altitude to km MSL
			 */
				if (fld == f_alt)
				{
					val += sounding.sitealt;
					val *= 0.001;
				}
			}
			else
				val = NC_BADVAL;
		/*
		 * Write the datum
		 */
			ncvarput1 (Sfile, v_id[i], &ndx, (void *)(&val));
		}
	}
/*
 * Close the file
 */
	ncclose (Sfile);
}



