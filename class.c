/*
 * CLASS format sounding access
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

static char *rcsid = "$Id: class.c,v 1.21 1993-02-18 17:58:40 burghart Exp $";

# include <stdio.h>
# include <errno.h>
# include <ui_param.h>
# include <time.h>
# include "sounding.h"

# define STRSIZE	400	/* Big to allow for oversized comments */

struct fldmatch
{
	fldtype	fld;	/* SUDS field identifier	*/
	char	*name;	/* CLASS name for this field	*/
	char	*units;	/* CLASS Units string		*/
};

struct fldmatch	F_match_tbl[] =
{
	{f_time, "Tim", ":s"}, 
	{f_lat, "Lat", ":d"}, 
	{f_lon, "Lon", ":d"},
	{f_alt, "Alt", ":m"}, 
	{f_pres, "Prs", ":mb"}, 
	{f_temp, "Tmp", ":C"},
	{f_dp, "Dpt", ":C"}, 
	{f_rh, "RH", ":%"}, 
	{f_wspd, "WS", ":m/s"},
	{f_wdir, "WD", ":d"}, 
	{f_u_wind, "U", ":m/s"}, 
	{f_v_wind, "V", ":m/s"},
	{f_qpres, "Qp", ""}, 
	{f_qtemp, "Qt", ""}, 
	{f_qrh, "Qh", ""}, 
	{f_qwind, "Quv", ""}, 
	{f_qu, "Qu", ""}, 
	{f_qv, "Qv", ""}, 
	{f_rtype, "Rtype", ""}, 
	{f_ascent, "dZ", ":m/s"}, 
	{f_mr, "MR", ":g/kg"},
	{f_azimuth, "Ang", ":deg"}, 
	{f_range, "Rng", ":km"}, 
	{f_null, "", ""}
};

/*
 * Forward declarations
 */
void	cls_newclass (), cls_lowell (), cls_ncbody ();

/*
 * The sounding file
 */
static FILE	*Sfile;

/*
 * Lowell sounding?
 */
static bool	Lowell;




void
cls_read_file (fname, sounding)
char	*fname;
struct snd	*sounding;
/*
 * Get the CLASS sounding from the given file
 */
{
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
	Sfile = NULL;

	if (! strcmp (fname, "-") && (Sfile = fdopen (0, "r")) == 0)
		ui_error ("Error %d reading CLASS sounding from stdin", errno);

	if (! Sfile && (Sfile = fopen (fname, "r")) == 0)
		ui_error ("Error %d opening CLASS file '%s'", errno, fname);
/*
 * Read the location name
 *
 * In a kind of klugy way, we attempt to identify the file by one of 
 * two means:
 *
 *	1) If there is a comma delimiter, assume it is an old CLASS format
 *	   file (before 4/90).  So far, old CLASS is the only format with
 *	   commas in the first line.
 *
 *	2) If the first ten characters are "Data Type:", assume it is
 *	   a new CLASS format file.
 */
	Lowell = FALSE;

	pos = 0;
	while ((character = fgetc (Sfile)) != ',' && character != '\n')
	{
		string[pos++] = character;
	/*
	 * If we have the string "Data Type:", this is a new CLASS
	 * format file, so deal with it elsewhere
	 */
		if (pos == 10 && ! strncmp (string, "Data Type:", 10))
		{
			cls_newclass (sounding);
			return;
		}
	/*
	 * Extra kluge for U. Lowell soundings, which have some
	 * extra garbage in the headers
	 */
		if (pos == 2 && ! strncmp (string, " #", 2))
		{
			Lowell = TRUE;
			cls_lowell (sounding);
			return;
		}
	}
/*
 * If the last character was a comma, we have an old CLASS format
 * file.  Otherwise, this probably isn't a CLASS file at all.
 */	
	if (character == ',')
		string[pos] = (char) 0;
	else
		ui_error ("Bad location string -- Is this a CLASS file?");
/*
 * Read the lon, lat, and alt
 */
	fscanf (Sfile, "%f,%f,%f\n", &(sounding->sitelon), 
		&(sounding->sitelat), &(sounding->sitealt));

	sounding->site = (char *) 
		malloc ((1 + strlen (string)) * sizeof (char));
	strcpy (sounding->site, string);
/*
 * Make sure we got non-zero lat, lon, and alt.  This is here because
 * some old files written by SUDS have these values set to zero.
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

	if (sounding->sitealt == 0.0)
	{
		ui_printf ("Site altitude in file is zero\n");
		sounding->sitealt = ui_float_prompt (
			"Enter site altitude in meters", (char *) 0, -50.0,
			5000.0, 0.0);
	}
/*
 * Sounding release date and time
 */
	fscanf (Sfile, "%d,%d,%d,%d:%d:%d\n", &year, &month, &day, &hour,
		&minute, &second);

	year -= 1900;	/* ui date stuff puts on 1900 for us */
	sounding->rls_time.ds_yymmdd = 10000 * year + 100 * month + day;
	sounding->rls_time.ds_hhmmss = 10000 * hour + 100 * minute + second;
/*
 * Comment and number of points
 */
	fscanf (Sfile, "%[^,],%d\n", string, &npts);
	npts++;		/* Add one for the surface point */
/*
 * Surface fields
 */
	fscanf (Sfile, "%[^,],", string);

	character = ' ';
	for (i = 0; character != '\n'; i++)
	{
		if (i >= MAXFLDS)
			ui_error ("Too many surface fields!");
	/*
	 * Get the field string
	 */
		fscanf (Sfile, "%[^:,\n]", string);
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
		fscanf (Sfile, "%[^,\n]", string);
		character = fgetc (Sfile);
	}
	n_sfc_fld = i;
/*
 * Sounding fields
 */
	fscanf (Sfile, "%[^,],", string);
	character = ' ';
	for (i = 0; character != '\n'; i++)
	{
		if (i >= MAXFLDS)
			ui_error ("Too many sounding fields!");
	/*
	 * Get the field string
	 */
		fscanf (Sfile, "%[^:,\n]", string);
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
		fscanf (Sfile, "%[^,\n]", string);
		character = fgetc (Sfile);
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
		fgets (string, STRSIZE, Sfile);
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
				fscanf (Sfile, ",");
		/*
		 * Read the value
		 */
			status = fscanf (Sfile, "%f", &val);
			end_of_file = (status == EOF);
			if (end_of_file && ndx < 1)
			   ui_error ("Bad data read -- Is this a CLASS file?");
		/*
		 * Test the value and assign it if it's good
		 */
			if (val < badthresh && ! end_of_file)
			{
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
		fscanf (Sfile, "\n");
	}
/*
 * Set the max index for these data
 */
	sounding->maxndx = ndx - 1;
/*
 * Close the file and return
 */
	fclose (Sfile);
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
	if ((Sfile = fopen (fname, "w")) == 0)
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
	fprintf (Sfile, "%s,%.5f,%.5f,%d\n", string, sounding->sitelon,
		sounding->sitelat, (int) sounding->sitealt);

	year = 1900 + sounding->rls_time.ds_yymmdd / 10000;
	month = (sounding->rls_time.ds_yymmdd / 100) % 100;
	day = sounding->rls_time.ds_yymmdd % 100;
	hour = sounding->rls_time.ds_hhmmss / 10000;
	minute = (sounding->rls_time.ds_hhmmss / 100) % 100;
	second = sounding->rls_time.ds_hhmmss % 100;
	fprintf (Sfile, "%d,%02d,%02d,%02d:%02d:%02d\n", year, month, day, 
		hour, minute, second);

	fprintf (Sfile, "SUDS edited data,%d\n", sounding->maxndx);
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
	fprintf (Sfile, "SURFACE:%s\n", fldstring);
	fprintf (Sfile, "FLIGHT: %s\n", fldstring);
/*
 * Four lines for comments
 */
	current_time = time (0);
	fprintf (Sfile, "SUDS edited sounding file, created by %s %s",
		getenv ("USER"), ctime (&current_time));
	fprintf (Sfile, "/\n");
	fprintf (Sfile, "/\n");
	fprintf (Sfile, "/\n");
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
			}
			else
				val = 99999.;
		/*
		 * Write the datum (5 places after the decimal 
		 * for lat and lon only)
		 */
			if (fld == f_lat || fld == f_lon)
				fprintf (Sfile, "%.5f", val);
			else
				fprintf (Sfile, "%.2f", val);
		/*
		 * Put in a comma or end the line
		 */
			if (nflds - i > 1)
				fprintf (Sfile, ",");
			else
				fprintf (Sfile, "\n");
		}
	}
/*
 * Close the file
 */
	fclose (Sfile);
}




void
cls_newclass (sounding)
struct snd	*sounding;
/*
 * We have a new CLASS format file opened and the first ten
 * characters ("Data Type:") have been read already.  Read
 * the rest and stuff the info into 'sounding'.
 */
{				
	char	string[STRSIZE];
	int	i, year, month, day, hour, minute, second;
/*
 * Ignore the first and second lines
 */
	fgets (string, STRSIZE, Sfile);
	fgets (string, STRSIZE, Sfile);
/*
 * Read the third line and strip trailing '\r' and '\n' characters
 */
	fgets (string, STRSIZE, Sfile);
	for (i = strlen (string) - 1; string[i]=='\r' || string[i]=='\n'; i--)
		string[i] = '\0';
/*
 * Build a site name from the third line info
 */
	sounding->site = (char *) 
		malloc ((strlen (string) - 34) * sizeof (char));
	strcpy (sounding->site, string + 35);
/*
 * Get the lon, lat, and alt
 */
	fscanf (Sfile, "%[^:]:%[^,],%[^,],%f,%f,%f", string, string, string,
		&sounding->sitelon, &sounding->sitelat, &sounding->sitealt);
/*
 * Sounding release date and time
 */
	fscanf (Sfile, "%[^:]:%d,%d,%d,%d:%d:%d", string, &year, &month, 
		&day, &hour, &minute, &second);
	year -= 1900;

	sounding->rls_time.ds_yymmdd = 10000 * year + 100 * month + day;
	sounding->rls_time.ds_hhmmss = 10000 * hour + 100 * minute + second;
/*
 * Skip the rest of this line and the next ten lines
 */
	for (i = 0; i < 11; i++)
		fgets (string, STRSIZE, Sfile);
/*
 * Read the body of the file
 */
	cls_ncbody (sounding);
}




void
cls_lowell (sounding)
struct snd	*sounding;
/*
 * Read a U. Lowell format sounding file, which is like the new CLASS
 * format, but has a modified header.  The first line has been read
 * up to the \n when this gets called.
 */
{				
	char	string[STRSIZE];
	int	i, year, month, day, hour, minute, second;
/*
 * Skip the first line's \n and the next three lines
 */
	for (i = 0; i < 4; i++)
		fgets (string, STRSIZE, Sfile);
/*
 * Read the next line and strip trailing '\r' and '\n' characters
 */
	fgets (string, STRSIZE, Sfile);
	for (i = strlen (string) - 1; string[i]=='\r' || string[i]=='\n'; i--)
		string[i] = '\0';
/*
 * Build a site name from the third line info
 */
	sounding->site = (char *) 
		malloc ((strlen (string) - 37) * sizeof (char));
	strcpy (sounding->site, string + 38);
/*
 * Skip the lat, lon, alt line, since it's usually wrong anyway
 */
	fgets (string, STRSIZE, Sfile);
	sounding->sitelon = -81.1942;
	sounding->sitelat = 28.4250;
/*
 * Sounding release date and time
 */
	fscanf (Sfile, "%[^:]:%d/%d/%d %d:%d:%d", string, &month, &day, 
		&year, &hour, &minute, &second);
	year -= 1900;

	sounding->rls_time.ds_yymmdd = 10000 * year + 100 * month + day;
	sounding->rls_time.ds_hhmmss = 10000 * hour + 100 * minute + second;
/*
 * Read lines until we hit the last header line
 */
	while (strncmp (string, " # ___", 6))
		fgets (string, STRSIZE, Sfile);
/*
 * Read the body
 */
	cls_ncbody (sounding);
}




void
cls_ncbody (sounding)
struct snd	*sounding;
/*
 * Read the body of a new CLASS format file
 */
{
	char	string[STRSIZE], c;
	int	i, status = 0, ndx, nfld;
	struct snd_datum	*dptr[MAXFLDS], *prevpt;
	float	val[MAXFLDS], badval[MAXFLDS];
/*
 * Initialize the data pointers
 */
	for (i = 0; i < MAXFLDS; i++)
		dptr[i] = 0;
/*
 * Assign the fields (hard-wired)
 */
	sounding->fields[0] = f_time;	sounding->fields[1] = f_pres;
	sounding->fields[2] = f_temp;	sounding->fields[3] = f_dp;
	sounding->fields[4] = f_rh;	sounding->fields[5] = f_u_wind;
	sounding->fields[6] = f_v_wind;	sounding->fields[7] = f_wspd;
	sounding->fields[8] = f_wdir;	sounding->fields[9] = f_ascent;
	sounding->fields[10] = f_lon;	sounding->fields[11] = f_lat;
	sounding->fields[12] = f_range;	sounding->fields[13] = f_azimuth;
	sounding->fields[14] = f_alt;	sounding->fields[15] = f_qpres;
	sounding->fields[16] = f_qtemp;	sounding->fields[17] = f_qrh;
	sounding->fields[18] = f_qu;	sounding->fields[19] = f_qv;
	sounding->fields[20] = f_qwind;	sounding->fields[21] = f_null;
/*
 * Bad value flags
 * We don't use bad value flags for the quality fields (15-20)
 */
	badval[0] = 9999.0;	badval[1] = 9999.0;
	badval[2] = 999.0;	badval[3] = 999.0;
	badval[4] = 999.0;	badval[5] = 999.0;
	badval[6] = 999.0;	badval[7] = 999.0;
	badval[8] = 999.0;	badval[9] = 99.0;
	badval[10] = 999.0;	badval[11] = 999.0;
	badval[12] = 999.0;	badval[13] = 999.0;
	badval[14] = 99999.0;	badval[15] = 32767.0;
	badval[16] = 32767.0;	badval[17] = 32767.0;
	badval[18] = 32767.0;	badval[19] = 32767.0;
	badval[20] = 32767.0;
/*
 * Only 20 fields in U. Lowell soundings
 */
	if (Lowell)
	{
		nfld = 20;
		sounding->fields[20] = f_null;
	}
	else
		nfld = 21;
/*
 * Get the data
 */
	for (ndx = 0; ; ndx++)
	{
	/*
	 * Read the data values from the next line.  (we need the %c here 
	 * because the fields are sometimes delimited with commas, sometimes 
	 * with spaces.  Explicitly reading a character gets us past either 
	 * way.)
	 */
		for (i = 0; i < nfld && status != EOF; i++)
			status = fscanf (Sfile, "%f%c", &val[i], &c);

		if (status == EOF || status == 0)
			break;
	/*
	 * Put the data points into their respective data lists
	 */
		for (i = 0; i < nfld; i++)
		{
		/*
		 * Don't put bad values in the list
		 */
			if (val[i] == badval[i])
				continue;
		/*
		 * KLUGE: U. of Lowell soundings have wspd in knots, even
		 * though the header says m/s.  U and v are actually reported
		 * in m/s, though.  Go figure.
		 */
			if (Lowell && sounding->fields[i] == f_wspd)
				val[i] *= 0.514791;
		/*
		 * Get a new point
		 */
			prevpt = dptr[i];
			dptr[i] = (struct snd_datum *)
				calloc (1, sizeof (struct snd_datum));
		/*
		 * Set the value and the index
		 */
			dptr[i]->value = val[i];
			dptr[i]->index = ndx;
		/*
		 * Link the point into the list or make it the head
		 */
			if (prevpt)
			{
				prevpt->next = dptr[i];
				dptr[i]->prev = prevpt;
			}
			else
				sounding->dlists[i] = dptr[i];
		}
	}
/*
 * Set the max index number for the sounding
 */
	sounding->maxndx = ndx - 1;
/*
 * Close the file and return
 */
	fclose (Sfile);
	return;
}

