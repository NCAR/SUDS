/*
 * FGGE format sounding access
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

static char *rcsid = "$Id: fgge.c,v 1.6 1991-10-21 21:37:39 burghart Exp $";

# include <stdio.h>
# include <stdlib.h>

# include <ui_param.h>
# include "globals.h"
# include "sounding.h"

# define BAD	-999

/*
 * 12 fields in FGGE data
 */
# define NFLD	12

# define STRSIZE	100


int	fgge_int_extract ();



void
fgge_read_file (fname, sounding)
char	*fname;
struct snd	*sounding;
/*
 * Get the FGGE sounding from the given file
 */
{
	FILE	*sfile;
	int	ndx, start[NFLD], flen[NFLD], i;
	struct snd_datum	*dptr[NFLD];
	float	val[NFLD];
	char	string[STRSIZE], site[10], *status;
	void	fgge_insert_data ();
/*
 * Open the file
 */
	if ((sfile = fopen (fname, "r")) == 0)
		ui_error ("Cannot open sounding file '%s'", fname);
/*
 * Read the header line
 */
	fgets (string, STRSIZE - 1, sfile);
/*
 * Put the site (station number) into the sounding structure
 */
	sounding->site = (char *) malloc (10 * sizeof (char));
	sounding->site[9] = 0;
	strcpy (sounding->site, "Stn ");
	strncat (sounding->site, string + 8, 5);
/*
 * Get the station location
 */
	sounding->sitealt = fgge_int_extract (string, 13, 5) / 10.0;

	sounding->sitelat = fgge_int_extract (string, 18, 4) / 100.0;
	if (string[22] == 'S')
		sounding->sitelat *= -1.0;

	sounding->sitelon = fgge_int_extract (string, 23, 5) / 100.0;
	if (string[28] == 'W')
		sounding->sitelon *= -1.0;
/*
 * Get the date and time
 */
	sounding->rls_time.ds_yymmdd = fgge_int_extract (string, 29, 6);
	sounding->rls_time.ds_hhmmss = fgge_int_extract (string, 35, 4) * 100;
/*
 * Initialize the data pointers
 */
	for (i = 0; i < NFLD; i++)
		dptr[i] = 0;
/*
 * Assign the fields (hard-wired)
 */
	sounding->fields[0] = f_pres;	start[0] = 1;	flen[0] = 5;
	sounding->fields[1] = f_qpres;	start[1] = 6;	flen[1] = 2;
	sounding->fields[2] = f_alt;	start[2] = 8;	flen[2] = 5;
	sounding->fields[3] = f_rtype;	start[3] = 13;	flen[3] = 2;
	sounding->fields[4] = f_temp;	start[4] = 15;	flen[4] = 4;
	sounding->fields[5] = f_qtemp;	start[5] = 19;	flen[5] = 2;
	sounding->fields[6] = f_rh;	start[6] = 21;	flen[6] = 3;
	sounding->fields[7] = f_dp;	start[7] = 24;	flen[7] = 4;
	sounding->fields[8] = f_qrh;	start[8] = 28;	flen[8] = 2;
	sounding->fields[9] = f_wdir;	start[9] = 30;	flen[9] = 3;
	sounding->fields[10] = f_wspd;	start[10] = 33;	flen[10] = 3;
	sounding->fields[11] = f_qwind;	start[11] = 36;	flen[11] = 2;	
/*
 * Get the data
 */
	for (ndx = 0; ; ndx++)
	{
	/*
	 * Read the next line
	 */
		status = fgets (string, STRSIZE - 1, sfile);

		if (status == NULL)
			break;
	/*
	 * Read the data values and scale if necessary
	 */
		for (i = 0; i < NFLD; i++)
		{
			val[i] = (float) 
				fgge_int_extract (string, start[i], flen[i]);
		/*
		 * Scale four of the fields and convert altitude to MSL
		 */
			if (val[i] != BAD)
			{
				if (i == 0 || i == 4 || i == 7 || i == 10)
					val[i] *= 0.1;
				else if (i == 2)
					val[i] += sounding->sitealt;
			}
		}
	/*
	 * Insert the data
	 * Put the data points into their respective data lists
	 */
		fgge_insert_data (sounding, ndx, val, dptr);
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
fgge_insert_data (sounding, ndx, val, dptr)
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
		if (val[i] == BAD)
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



int
fgge_int_extract (string, start, len)
char	*string;
int	start, len;
/*
 * Extract an integer of length 'len' from the given string starting 
 * at position 'start'
 */
{
	int	ival = 0, neg = FALSE, ndx, ndigits = 0;
/*
 * Check for a sign
 */
	if (string[start] == '-')
	{
		neg = TRUE;
		start++;
		len--;
	}
	else if (string[start] == '+')
	{
		start++;
		len--;
	}
/*
 * Read the number
 */
	for (ndx = start; ndx < start + len; ndx++)
	{
		char	ch = string[ndx];

		if (ch == ' ')
			continue;

		if (ch < '0' || ch > '9')
		{
			char	*temp = (char *) 
					malloc ((len + 1) * sizeof (char));

			strncpy (temp, string + start, len);
			temp[len] = 0;

			ui_warning (
				"Non-numeric string '%s', inserting bad flag",
				temp);
			free (temp);
			return (BAD);
		}
		else
		{
			ndigits++;
			ival *= 10;
			ival += (int)(ch - '0');
		}
	}
/*
 * Return the appropriate value
 */
	if (! ndigits)
		return (BAD);
	else if (neg)
		return (-ival);
	else
		return (ival);
}
