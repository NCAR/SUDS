/*
 * NMC ADP (sort of) format sounding access
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

static char *rcsid = "$Id: nmc.c,v 1.3 1994-01-10 21:56:56 case Exp $";

# include <stdio.h>
# include <stdlib.h>

# include <ui_param.h>
# include "sounding.h"

/*
 * 6 fields in NMC data
 */
# define NFLD	6

# define STRSIZE	100

/*
 * Bad value flags for each field
 */
static float	Bad[NFLD];

/*
 * Forward declarations
 */
float	nmc_extract ();




void
nmc_read_file (fname, sounding)
char	*fname;
struct snd	*sounding;
/*
 * Get the NMC sounding from the given file
 */
{
	FILE	*sfile;
	char	*status;
	int	i, sid, ndx, npts;
	struct snd_datum	*dptr[NFLD];
	float	val[NFLD];
	char	string[STRSIZE];
	void	nmc_insert_data ();
/*
 * Open the file
 */
	if ((sfile = fopen (fname, "r")) == 0)
		ui_error ("Cannot open NMC file '%s'", fname);
/*
 * Sounding header line
 */
	fgets (string, STRSIZE - 1, sfile);
/*
 * Decode the date from the first six characters of the header
 */
	sounding->rls_time.ds_yymmdd = 
		(int) nmc_extract (string, 0, 2, 0.0) * 100 +
		(int) nmc_extract (string, 2, 2, 0.0) +
		(int) nmc_extract (string, 4, 2, 0.0) * 10000;
/*
 * Launch hour is in the 9th and 10th characters
 */
	sounding->rls_time.ds_hhmmss = 
		(int) nmc_extract (string, 8, 2, 0.0) * 10000;
/*
 * We can get the rest of the header information using sscanf.
 * Get the site ID, site altitude, number of points, site lat., and site lon.
 */
	sscanf (string + 10, "%d %f %d %f %f", &sid, &sounding->sitealt, 
		&npts, &sounding->sitelat, &sounding->sitelon);
	sounding->sitelon *= -1.0;	/* Make west longitudes negative */
/*
 * Put the site into the sounding structure
 */
	sounding->site = (char *) malloc (6 * sizeof (char));
	sprintf (sounding->site, "%d", sid);
/*
 * Initialize the data pointers
 */
	for (i = 0; i < NFLD; i++)
		dptr[i] = 0;
/*
 * Assign the fields (hard-wired)
 */
	sounding->fields[0] = f_pres;	Bad[0] = 9999.0;
	sounding->fields[1] = f_temp;	Bad[1] = 999.0;
	sounding->fields[2] = f_dp;	Bad[2] = 999.0;
	sounding->fields[3] = f_alt;	Bad[3] = 99999.0;
	sounding->fields[4] = f_wdir;	Bad[4] = 999.0;
	sounding->fields[5] = f_wspd;	Bad[5] = 999.0;
	sounding->fields[6] = f_null;
/*
 * Get the data
 */
	for (ndx = 0; ndx < npts; ndx++)
	{
	/*
	 * Read the next line
	 */
		status = fgets (string, STRSIZE - 1, sfile);

		if (status == NULL)
		{
			ndx--;
			break;
		}
	/*
	 * Read the data values and scale if necessary
	 */
		val[0] = nmc_extract (string, 0, 6, Bad[0]);
		val[1] = nmc_extract (string, 6, 5, Bad[1]);
		val[2] = nmc_extract (string, 11, 6, Bad[2]);
		val[3] = nmc_extract (string, 17, 8, Bad[3]);
		val[4] = nmc_extract (string, 25, 6, Bad[4]);
		val[5] = nmc_extract (string, 31, 6, Bad[5]);
	/*
	 * Insert the data
	 * Put the six data points into their respective data lists
	 */
		nmc_insert_data (sounding, ndx, val, dptr);
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
nmc_insert_data (sounding, ndx, val, dptr)
struct snd	*sounding;
int	ndx;
float	*val;
struct snd_datum	*dptr[MAXFLDS];
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
		if (val[i] >= Bad[i])
			continue;
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




float
nmc_extract (string, start, len, badval)
char	*string;
int	start, len;
float	badval;
/*
 * Extract an integer or float of length 'len' from the given string starting 
 * at position 'start'
 */
{
	int	neg = FALSE, ndx, ndigits = 0, ddigits = 0;
	int	decimal = FALSE;
	float	val = 0.0, mul = 1.0;
/*
 * Ignore leading spaces
 */
	ndx = start;
	while (string[ndx] == ' ')
		ndx++;
/*
 * Check for a sign
 */
	if (string[ndx] == '-')
	{
		neg = TRUE;
		ndx++;
	}
	else if (string[ndx] == '+')
		ndx++;
/*
 * Read the number
 */
	for (; ndx < start + len; ndx++)
	{
		char	ch = string[ndx];

		if (ch == ' ')
			continue;
		else if (ch == '.' && ! decimal)
		{
			decimal = TRUE;
			continue;
		}					
		else if (ch < '0' || ch > '9')
		{
			char	*temp = (char *) 
					malloc ((len + 1) * sizeof (char));

			strncpy (temp, string + start, len);
			temp[len] = 0;

			ui_warning (
				"Weird value string '%s', inserting bad flag",
				temp);
			free (temp);
			return (badval);
		}
		else
		{
			ndigits++;
			if (decimal)
			{
				mul *= 0.1;
				val += (float)(ch - '0') * mul;
			}
			else
			{
				val *= 10.0;
				val += (float)(ch - '0');
			}
		}
	}
/*
 * Return the appropriate value
 */
	if (ndigits == 0)
		return (badval);
	else if (neg)
		return (-val);
	else
		return (val);
}
