/*
 * CaPE format sounding access (the RATS and MSS soundings from CaPE are
 * provided in this format)
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

static char *rcsid = "$Id: cape.c,v 1.2 1991-12-10 23:39:30 case Exp $";

# include <stdio.h>
# include <ui_param.h>
# include "sounding.h"

/*
 * 7 fields that we currently deal with in these soundings
 * Assume there are no more than 300 points
 */
# define NFLD	7
# define MAXPTS	300

# define STRSIZE	100

/*
 * Bad value flags for each field
 */
float	Bad[NFLD];

/*
 * Scale factors for each field
 */
float	Scale[NFLD];

/*
 * Month names
 */
char *Mnames[] = {"", "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL", "AUG",
	"SEP", "OCT", "NOV", "DEC", (char *) 0};

/*
 * Forward declarations
 */
void	cape_insert_data ();
void	cape_sort ();

/*
 * The raw data structure
 */
typedef struct
{
	float	alt, wdir, wspd, temp, dp, pres, rh;
} rawpt;




void
cape_read_file (fname, sounding)
char	*fname;
struct snd	*sounding;
/*
 * Get the CaPE sounding from the given file
 */
{
	FILE	*sfile;
	int	i, m, npts, ndx, line = 0, time, day, year;
	struct snd_datum	*dptr[NFLD], *prevpt;
	float	dummy, *dp, *rawdata;
	char	string[STRSIZE], month[4];
/*
 * Open the file
 */
	if ((sfile = fopen (fname, "r")) == 0)
		ui_error ("Cannot open CaPE file '%s'", fname);
/*
 * Read until we hit "CAPE CANAVERAL AFS, FLORIDA"
 */
	strcpy (string, "");

	while (strncmp (string, "CAPE CANAV", 10))
	{
		line++;
		if (! fgets (string, STRSIZE - 1, sfile))
			ui_error ("Can't find site name in sounding file!");
	}
/*
 * The site name is on the fourth line for Melbourne (MLB) and on the fifth 
 * line for Cape Canaveral Air Force Station (CCAFS).  Good thing, otherwise 
 * we couldn't tell them apart
 */
	if (line == 4)
	{
		sounding->site = (char *) malloc (4 * sizeof (char));
		strcpy (sounding->site, "MLB");

		sounding->sitelat = 28.107;
		sounding->sitelon = -80.650;
		sounding->sitealt = 7;
	}
	else
	{
		sounding->site = (char *) malloc (6 * sizeof (char));
		strcpy (sounding->site, "CCAFS");

		sounding->sitelat = 28.483;
		sounding->sitelon = -80.550;
		sounding->sitealt = 0;
	}
/*
 * Read the time and date from the next line
 */
	fscanf (sfile, "%dZ %d %s %d", &time, &day, month, &year);
	fgets (string, STRSIZE - 1, sfile);	/* Read past the newline */

	for (m = 0; Mnames[m] && strcmp (month, Mnames[m]); m++)

	sounding->rls_time.ds_hhmmss = time * 100;
	sounding->rls_time.ds_yymmdd = year * 10000 + m * 100 + day;
/*
 * Allocate the raw data array
 */
	dp = rawdata = (float *) malloc (MAXPTS * NFLD * sizeof (rawpt));
	npts = 0;
/*
 * Assign the fields (hard-wired) and their scale factors.  (altitude is 
 * recorded in feet and wspd is recorded in knots; the scales are used to
 * convert to metric units)
 */
	sounding->fields[0] = f_alt; Bad[0] = 99999.; Scale[0] = 0.3048;
	sounding->fields[1] = f_wdir; Bad[1] = 999.; Scale[1] = 1.0;
	sounding->fields[2] = f_wspd; Bad[2] = 999.9; Scale[2] = 0.514791;
	sounding->fields[3] = f_temp; Bad[3] = 99.9; Scale[3] = 1.0;
	sounding->fields[4] = f_dp; Bad[4] = 99.9; Scale[4] = 1.0;
	sounding->fields[5] = f_pres; Bad[5] = 9999.99; Scale[5] = 1.0;
	sounding->fields[6] = f_rh; Bad[6] = 999.; Scale[6] = 1.0;
	sounding->fields[7] = f_null;
/*
 * Skip 4 lines and get the dense data
 */
	for (i = 0; i < 4; i++)
		fgets (string, STRSIZE - 1, sfile);

	while (fscanf (sfile, "%f%f%f%f%f%f%f%f", dp, dp + 1,
		dp + 2, &dummy, dp + 3, dp + 4, dp + 5, dp  + 6) == 8)
	{
		fgets (string, STRSIZE - 1, sfile);	/* finish off the line */
		npts++;
		dp += NFLD;
	}
/*
 * Skip 6 lines and read the mandatory data
 */
	for (i = 0; i < 6; i++)
		fgets (string, STRSIZE - 1, sfile);

	while (fscanf (sfile, "%f%f%f%f%f%f%f", dp, dp + 1, dp + 2, dp + 3, 
		dp + 4, dp + 5, dp  + 6) == 7)
	{
		npts++;
		dp += NFLD;
	}
/*
 * Skip 3 lines and read the significant level data
 */
	for (i = 0; i < 3; i++)
		fgets (string, STRSIZE - 1, sfile);

	while (fscanf (sfile, "%f%f%f%f%f%f%f%f", dp, dp + 1, dp + 2, dp + 3,
		dp + 4, dp + 5, &dummy, dp + 6) == 8)
	{
		npts++;
		dp += NFLD;
	}
/*
 * Sort the raw data
 */
	cape_sort ((rawpt *) rawdata, npts);
/*
 * Initialize the data pointers
 */
	for (i = 0; i < NFLD; i++)
		dptr[i] = (struct snd_datum *) 0;
/*
 * Put the data into the data lists
 */
	dp = rawdata;

	for (ndx = 0; ndx < npts; ndx++)
	{
		for (i = 0; i < NFLD; i++)
		{
		/*
		 * Bypass bad data and scale good data
		 */
			if (dp[i] == Bad[i])
				continue;
			else
				dp[i] *= Scale[i];
		/*
		 * Get a new point
		 */
			prevpt = dptr[i];
			dptr[i] = (struct snd_datum *)
				calloc (1, sizeof (struct snd_datum));
		/*
		 * Set the value and the index
		 */
			dptr[i]->value = dp[i];
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

		dp += NFLD;
	}
/*
 * Set the max index number for the sounding
 */
	sounding->maxndx = npts - 1;
/*
 * Close the file and return
 */
	fclose (sfile);
	free (rawdata);
	return;
}




void
cape_sort (rawdata, npts)
rawpt	*rawdata;
int	npts;
/*
 * Bubble sort the raw data by pressure
 */
{
	int	ndx, i;
	rawpt	temp;

	for (ndx = 0; ndx < npts; ndx++)
	{
		i = ndx;

		while (i > 0 && rawdata[i].pres > rawdata[i-1].pres)
		{
			temp = rawdata[i-1];
			rawdata[i-1] = rawdata[i];
			rawdata[i] = temp;
			i--;
		}
	}
}
