/*
 * NSSL format sounding access
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

static char *rcsid = "$Id: nssl.c,v 1.2 1994-01-10 21:56:24 case Exp $";

# include <stdio.h>
# include <ui_param.h>
# include "sounding.h"

/*
 * 13 fields that we currently deal with in these soundings
 */
# define NFLD	13

/*
 * Bad value flags for each field
 */
static float	Bad[NFLD];

/*
 * Forward declarations
 */
void	nssl_insert_data ();

/*
 * The raw data structure
 */
typedef struct
{
	float	alt, wdir, wspd, temp, dp, pres, rh;
} rawpt;




void
nssl_read_file (fname, sounding)
char	*fname;
struct snd	*sounding;
/*
 * Get the CaPE sounding from the given file
 */
{
	FILE	*sfile;
	int	i, dummy, ilat, ilon, ndx, fndx, status;
	float	v, val[NFLD];
	char	string[200];
	struct snd_datum	*dptr[NFLD], *prevpt;
/*
 * Open the file
 */
	if ((sfile = fopen (fname, "r")) == 0)
		ui_error ("Cannot open NSSL file '%s'", fname);
/*
 * Bag the first line
 */
	fgets (string, sizeof (string), sfile);
/*
 * Site, date, time, ?, alt, lat, lon
 */
	sounding->site = (char *) malloc (4 * sizeof (char));

	fscanf (sfile, "%s %d %d %d %f %d %d", sounding->site, 
		&(sounding->rls_time.ds_yymmdd), 
		&(sounding->rls_time.ds_hhmmss),
		&dummy, &(sounding->sitealt), &ilat, &ilon);
/*
 * Change time from hhmm to hhmmss
 */
	sounding->rls_time.ds_hhmmss *= 100;
/*
 * Lat & lon in the file are degrees * 100 + minutes (e.g., 40.5 degrees
 * shows up as 4030 in the file).
 */
	sounding->sitelat = ilat / 100 + (ilat % 100) / 60.0;
	sounding->sitelon = ilon / 100 + (ilon % 100) / 60.0;
/*
 * Assign the fields (hard-wired) and their bad values.
 */
	sounding->fields[0] = f_alt;	Bad[0] = -99.9;
	sounding->fields[1] = f_pres;	Bad[1] = -99.9;
	sounding->fields[2] = f_temp;	Bad[2] = -99.9;
	sounding->fields[3] = f_dp;	Bad[3] = -99.9;
	sounding->fields[4] = f_rh;	Bad[4] = -99.9;
	sounding->fields[5] = f_vt;	Bad[5] = -99.9;
	sounding->fields[6] = f_wspd;	Bad[6] = -99.9;
	sounding->fields[7] = f_wdir;	Bad[7] = 999.;
	sounding->fields[8] = f_u_wind;	Bad[8] = -99.9;
	sounding->fields[9] = f_v_wind;	Bad[9] = -99.9;
	sounding->fields[10] = f_azimuth;	Bad[10] = 999.;
	sounding->fields[11] = f_range;	Bad[11] = -99.9;
	sounding->fields[12] = f_time;	Bad[12] = -99.9;
	sounding->fields[13] = f_null;
/*
 * Finish off the second line and bag the next four lines
 */
	for (i = 0; i < 5; i++)
		fgets (string, sizeof (string), sfile);
/*
 * Initialize the data pointers
 */
	for (i = 0; i < NFLD; i++)
		dptr[i] = (struct snd_datum *) 0;
/*
 * Get the data
 */
	for (ndx = 0; ; ndx++)
	{
	/*
	 * Read the next line
	 */
		fndx = 0;
		for (i = 0; i < 20; i++)
		{
			status = fscanf (sfile, "%f", &v);
			if (status != 1)
				break;
		/*
		 * Go on if it's a field we're not saving
		 */
			if (i == 5 || i == 6 || i == 8 || i == 9 || i == 16 ||
				i == 17 || i == 19)
				continue;
		/*
		 * Otherwise deal with it
		 */
			val[fndx++] = v;
		}

		if (status != 1)
			break;
	/*
	 * Insert the data points into their respective data lists
	 */
		nssl_insert_data (sounding, ndx, val, dptr);
	}
/*
 * Set the max index number for the sounding
 */
	sounding->maxndx = ndx - 1;
/*
 * Close the file and return
 */
	fclose (sfile);
	return;
}




void
nssl_insert_data (sounding, ndx, val, dptr)
struct snd	*sounding;
int	ndx;
float	*val;
struct snd_datum	**dptr;
/*
 * Insert the data from array val into the sounding
 */
{
	int	i;
	struct snd_datum	*prevpt;
/*
 * Put the data into the data lists
 */
	for (i = 0; i < NFLD; i++)
	{
	/*
	 * Don't put bad values in the list
	 */
		if ((int)(val[i] - Bad[i]) == 0)
			continue;
	/*
	 * Convert times from decimal minutes to seconds
	 */
		if (i == 12)
			val[i] *= 60.0;
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
