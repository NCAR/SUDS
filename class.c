/*
 * CLASS format sounding access
 *
 * $Revision: 1.9 $ $Date: 1990-04-13 15:32:57 $ $Author: burghart $
 * 
 */
# include <stdio.h>
# include <ui_param.h>
# include <time.h>
# include "sounding.h"

# define STRSIZE	200

struct fldmatch
{
	fldtype	fld;	/* SUDS field identifier	*/
	char	*name;	/* CLASS name for this field	*/
	char	*units;	/* CLASS Units string		*/
};

struct fldmatch	F_match_tbl[] =
{
	{f_time, "Tim", ":s"}, {f_lat, "Lat", ":d"}, {f_lon, "Lon", ":d"},
	{f_alt, "Alt", ":m"}, {f_pres, "Prs", ":mb"}, {f_temp, "Tmp", ":C"},
	{f_dp, "Dpt", ":C"}, {f_rh, "RH", ":%"}, {f_wspd, "WS", ":m/s"},
	{f_wdir, "WD", ":d"}, {f_u_wind, "U", ":m/s"}, {f_v_wind, "V", ":m/s"},
	{f_qpres, "Qp", ""}, {f_qtemp, "Qt", ""}, {f_qrh, "Qh", ""}, 
	{f_qwind, "Quv", ""}, {f_qu, "Qu", ""}, {f_qv, "Qv", ""}, 
	{f_null, "", ""}
};




void
cls_read_file (fname, sounding)
char	*fname;
struct snd	*sounding;
/*
 * Get the CLASS sounding from the given file
 */
{
	FILE	*sfile;
	int	i, j, year, month, day, hour, minute, second;
	int	ndx, npts, n_fld, n_sfc_fld, status, pos, fndx;
	int	end_of_file = FALSE;
	fldtype	sfc_fld[MAXFLDS];
	int	sfc_fndx[MAXFLDS];
	struct snd_datum	*dptr[MAXFLDS], *prevpt;
	float	val;
	char	string[STRSIZE], character;
	float	ui_float_prompt ();
/*
 * Open the file
 */
	if ((sfile = fopen (fname, "r")) == 0)
		ui_error ("Cannot open CLASS file '%s'", fname);
/*
 * Read the location name
 * In reading the location string, we test to see if it was terminated with
 * a comma.  If it wasn't, we assume this is not really a CLASS file.
 * (So far, CLASS files are the only ones with commas in the first line)
 */
	pos = 0;
	while ((character = fgetc (sfile)) != ',' && character != '\n')
		string[pos++] = character;

	if (character == ',')
		string[pos] = (char) 0;
	else
		ui_error ("Bad location string -- Is this a CLASS file?");
/*
 * Read the lon, lat, and alt
 */
	fscanf (sfile, "%f,%f,%f\n", &(sounding->sitelon), 
		&(sounding->sitelat), &(sounding->sitealt));

	sounding->site = (char *) 
		malloc ((1 + strlen (string)) * sizeof (char));
	strcpy (sounding->site, string);
/*
 * Make sure we got non-zero lat, lon, and alt.  This is here because
 * some old files written by SUDS have these values as zero.
 */
	if (sounding->sitelon == 0.0 && sounding->sitelat == 0.0)
	{
		ui_printf ("Site latitude and longitude in file are bad\n");
		sounding->sitelat = ui_float_prompt (
			"   Enter site latitude", (char *) 0, -90.0, 
			90.0, 0.0);
		sounding->sitelon = ui_float_prompt (
			"   Enter site longitude, west is negative",
			(char *) 0, -180.0, 180.0, 0.0);
	}

	if (sounding->sitelat == 0.0)
	{
		ui_printf ("Site altitude in file is zero\n");
		sounding->sitealt = ui_float_prompt (
			"Enter site altitude in meters", (char *) 0, -50.0,
			5000.0, 0.0);
	}
/*
 * Sounding release date and time
 */
	fscanf (sfile, "%d,%d,%d,%d:%d:%d\n", &year, &month, &day, &hour,
		&minute, &second);

	year -= 1900;	/* ui date stuff puts on 1900 for us */
	sounding->rls_time.ds_yymmdd = 10000 * year + 100 * month + day;
	sounding->rls_time.ds_hhmmss = 10000 * hour + 100 * minute + second;
/*
 * Comment and number of points
 */
	fscanf (sfile, "%[^,],%d\n", string, &npts);
	npts++;		/* Add one for the surface point */
/*
 * Surface fields
 */
	fscanf (sfile, "%[^,],", string);

	character = ' ';
	for (i = 0; character != '\n'; i++)
	{
		if (i >= MAXFLDS)
			ui_error ("Too many surface fields!");
	/*
	 * Get the field string
	 */
		fscanf (sfile, "%[^:,\n]", string);
	/*
	 * Search the fields table for a match
	 */
		for (j = 0; *F_match_tbl[j].name; j++)
			if (! strcmp (F_match_tbl[j].name, string))
				break;
	/*
	 * Did we get a field?
	 */
		if (F_match_tbl[j].fld == f_null)
			ui_error ("Unknown CLASS field '%s'", string);
	/*
	 * Put it in the surface field list
	 */
		sfc_fld[i] = F_match_tbl[j].fld;
	/*
	 * Read past the units
	 */
		fscanf (sfile, "%[^,\n]", string);
		character = fgetc (sfile);
	}
	n_sfc_fld = i;
/*
 * Sounding fields
 */
	fscanf (sfile, "%[^,],", string);
	character = ' ';
	for (i = 0; character != '\n'; i++)
	{
		if (i >= MAXFLDS)
			ui_error ("Too many sounding fields!");
	/*
	 * Get the field string
	 */
		fscanf (sfile, "%[^:,\n]", string);
	/*
	 * Search the fields table for a match
	 */
		for (j = 0; *F_match_tbl[j].name; j++)
			if (! strcmp (F_match_tbl[j].name, string))
				break;
	/*
	 * Did we get a field?
	 */
		if (F_match_tbl[j].fld == f_null)
			ui_error ("Unknown CLASS field '%s'", string);
	/*
	 * Put the field into the list
	 */
		sounding->fields[i] = F_match_tbl[j].fld;
	/*
	 * Read past the units
	 */
		fscanf (sfile, "%[^,\n]", string);
		character = fgetc (sfile);
	}
/*
 * Null terminate the fields list
 */
	n_fld = i;
	sounding->fields[n_fld] = f_null;
/*
 * Find the sounding field corresponding to each surface field
 */
	for (i = 0; i < n_sfc_fld; i++)
	{
		for (fndx = 0; fndx < n_fld; fndx++)
			if (sounding->fields[fndx] == sfc_fld[i])
				break;

		if (fndx == n_fld)
			ui_error ("*BUG* Sounding fld/surface fld mismatch");
		else
			sfc_fndx[i] = fndx;
	}
/*
 * Skip ahead 4 lines
 */
	for (i = 0; i < 4; i++)
		fgets (string, STRSIZE, sfile);
/*
 * Initialize the data pointers to null
 */
	for (i = 0; i < n_fld; i++)
		dptr[i] = 0;
/*
 * Loop through the data lines
 */
	for (ndx = 0; ndx < npts; ndx++)
	{
		int	surface = (ndx == 0);
		int	maxfld = surface ? n_sfc_fld : n_fld;
	/*
	 * Loop through the values in this data line
	 */
		for (i = 0; i < maxfld; i++)
		{
			fldtype	fld;
			int	fndx;
			float	badthresh = 999.0;
		/*
		 * Get the current field and field index
		 */
			fld = surface ? sfc_fld[i] : sounding->fields[i];
			fndx = surface ? sfc_fndx[i] : i;
		/*
		 * Higher threshold for some fields
		 */
			if (fld == f_pres || fld == f_time)
				badthresh = 9990.0;
			else if (fld == f_alt)
				badthresh = 99900.0;
		/*
		 * Bypass the comma
		 */
			if (i > 0)
				fscanf (sfile, ",");
		/*
		 * Read the value
		 */
			status = fscanf (sfile, "%f", &val);
			end_of_file = (status == EOF);
			if (end_of_file && ndx < 1)
			   ui_error ("Bad data read -- Is this a CLASS file?");
		/*
		 * Test the value and assign it if it's good
		 */
			if (val < badthresh && ! end_of_file)
			{
			/*
			 * Change altitude to ground relative
			 */
				if (fld == f_alt)
					val -= sounding->sitealt;
			/*
			 * Get a new point
			 */
				prevpt = dptr[fndx];
				dptr[fndx] = (struct snd_datum *)
					calloc (1, sizeof (struct snd_datum));
			/*
			 * Link it into the list or make it the head
			 */
				if (prevpt)
				{
					prevpt->next = dptr[fndx];
					dptr[fndx]->prev = prevpt;
				}
				else
					sounding->dlists[fndx] = dptr[fndx];
			/*
			 * Assign the value and index
			 */
				dptr[fndx]->value = val;
				dptr[fndx]->index = ndx;
			}
		}
	/*
	 * See if we ran out of data early
	 */
		if (end_of_file)
			break;
	/*
	 * Read the line terminator
	 */
		fscanf (sfile, "\n");
	}
/*
 * Set the max index for these data
 */
	sounding->maxndx = ndx - 1;
/*
 * Close the file and return
 */
	fclose (sfile);
	return;
}




void
cls_write_file (fname, sounding)
char	*fname;
struct snd	*sounding;
/*
 * Write the given sounding into file 'fname' using the CLASS data format
 */
{
	FILE	*sfile;
	int	i, j, year, month, day, hour, minute, second;
	int	ndx, nflds, altered = FALSE;
	long	current_time;
	float	val;
	char	string[40], fldstring[150];
	fldtype	fld, classfld;
	struct snd_datum	*data[MAXFLDS];
/*
 * Open the file
 */
	if ((sfile = fopen (fname, "w")) == 0)
		ui_error ("Cannot create file '%s'", fname);
/*
 * Copy the site name into a string, replacing commas with spaces
 * (since commas will cause us problems when we read it back in)
 */
	for (i = 0; sounding->site[i]; i++)
	{
		char	c = sounding->site[i];
		if (c == ',')
		{
			altered = TRUE;
			c = ' ';
		}
		string[i] = c;
	}
	string[i] = '\0';

	if (altered)
		ui_warning ("Replaced site name '%s' with '%s'", 
			sounding->site, string);
/*
 * Write:
 *	site name,lat,lon,altitude
 *	year,month,day,hh:mm:ss
 *	blurb,npts
 */
	fprintf (sfile, "%s,%.5f,%.5f,%d\n", string, sounding->sitelon,
		sounding->sitelat, (int) sounding->sitealt);

	year = 1900 + sounding->rls_time.ds_yymmdd / 10000;
	month = (sounding->rls_time.ds_yymmdd / 100) % 100;
	day = sounding->rls_time.ds_yymmdd % 100;
	hour = sounding->rls_time.ds_hhmmss / 10000;
	minute = (sounding->rls_time.ds_hhmmss / 100) % 100;
	second = sounding->rls_time.ds_hhmmss % 100;
	fprintf (sfile, "%d,%02d,%02d,%02d:%02d:%02d\n", year, month, day, 
		hour, minute, second);

	fprintf (sfile, "SUDS edited data,%d\n", sounding->maxndx);
/*
 * Write the fields and units into a string
 */
	strcpy (fldstring, "");

	for (i = 0; sounding->fields[i] != f_null; i++)
	{
		fld = sounding->fields[i];
	/*
	 * Search the CLASS fields table for a match
	 */
		for (j = 0; F_match_tbl[j].fld != f_null; j++)
		{
			classfld = F_match_tbl[j].fld;
			if (classfld == fld)
				break;
		}
	/*
	 * Did we get a field?
	 */
		if (classfld == f_null)
			ui_error ("No CLASS name for field '%s'", 
				fd_name (fld));
	/*
	 * Write the CLASS field name and the units
	 */
		sprintf (string, ",%s%s", F_match_tbl[j].name, 
			F_match_tbl[j].units);
		strcat (fldstring, string);
	}

	nflds = i;
/*
 * Now write two lines with the field names into the output file
 */
	fprintf (sfile, "SURFACE:%s\n", fldstring);
	fprintf (sfile, "FLIGHT: %s\n", fldstring);
/*
 * Four lines for comments
 */
	current_time = time (0);
	fprintf (sfile, "SUDS edited sounding file, created by %s %s",
		getenv ("USER"), ctime (&current_time));
	fprintf (sfile, "/\n");
	fprintf (sfile, "/\n");
	fprintf (sfile, "/\n");
/*
 * Establish our pointers into the data lists
 */
	for (i = 0; i < nflds; i++)
		data[i] = sounding->dlists[i];
/*
 * Write the data
 */
	for (ndx = 0; ndx <= sounding->maxndx; ndx++)
	{
	/*
	 * Look for a value for each field at this index
	 */
		for (i = 0; i < nflds; i++)
		{
			fld = sounding->fields[i];
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
			 * Adjust altitude to MSL
			 */
				if (fld == f_alt)
					val += sounding->sitealt;
			}
			else
				val = 99999.;
		/*
		 * Write the datum (5 places after the decimal 
		 * for lat and lon only)
		 */
			if (fld == f_lat || fld == f_lon)
				fprintf (sfile, "%.5f", val);
			else
				fprintf (sfile, "%.2f", val);
		/*
		 * Put in a comma or end the line
		 */
			if (nflds - i > 1)
				fprintf (sfile, ",");
			else
				fprintf (sfile, "\n");
		}
	}
/*
 * Close the file
 */
	fclose (sfile);
}
					
