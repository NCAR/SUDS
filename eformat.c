/*
 * E-format sounding access module
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

static char *rcsid = "$Id: eformat.c,v 1.5 1991-10-21 21:37:06 burghart Exp $";

# include <stdio.h>
# include "sounding.h"

# define BADVAL	 -999.

# define MAXRECLEN	512



void
ef_read_file (fname, sounding)
char	*fname;
struct snd	*sounding;
/*
 * Get the e-format sounding from the given file
 */
{
	int	file_id, rlen, nflds, i, ndx, fld, bufpos = 0;
	float	badval, datum;
	char	buf[MAXRECLEN];
	struct snd_datum	*dptr[MAXFLDS];
	float	ui_float_prompt ();
/*
 * Open the file
 */
	if ((file_id = bfview (fname)) == 0)
		ui_error ("Unable to open file '%s' for reading", fname);
/*
 * Read the first record and extract the release time, number of fields,
 * number of data records, bad value flag, and site
 */
	bfget (file_id, buf, &rlen);

	memcpy (&(sounding->rls_time), buf + bufpos, sizeof (date));
	bufpos += sizeof (date);

	memcpy (&nflds, buf + bufpos, sizeof (int));
	bufpos += sizeof (int);

	memcpy (&(sounding->maxndx), buf + bufpos, sizeof (int));
	bufpos += sizeof (int);

	memcpy (&badval, buf + bufpos, sizeof (float));
	bufpos += sizeof (float);

	sounding->site = (char *) 
		malloc ((1 + strlen (buf + bufpos)) * sizeof (char));
	strcpy (sounding->site, buf + bufpos);
	bufpos += (1 + strlen (sounding->site)) * sizeof (char);
/*
 * KLUGE: We prompt for the site altitude if we are reading an old e-format
 * file w/o the alt
 */
	if (bufpos - rlen >= sizeof (float))
		memcpy (&(sounding->sitealt), buf + bufpos, sizeof (float));
	else
	{
		ui_printf ("No site altitude in file\n");
		sounding->sitealt = ui_float_prompt (
			"Enter the site altitude in meters", 
			(char *) NULL, -50.0, 5000.0, 0.0);
	}
/*
 * Read the second record with the field identifiers
 */
	bufpos = 0;
	bfget (file_id, buf, &rlen);

	for (i = 0; i < nflds; i++)
	{
	/*
	 * Grab the field number
	 */
		memcpy (&(sounding->fields[i]), buf + bufpos, 
			sizeof (fldtype));
	/*
	 * Move the buffer position
	 */
		bufpos += sizeof (fldtype);
	/*
	 * Initialize the data pointer to null
	 */
		dptr[i] = 0;
	}

	sounding->fields[nflds] = f_null;
/*
 * Read the data records
 */
	for (ndx = 0; ndx <= sounding->maxndx; ndx++)
	{
		bufpos = 0;
		bfget (file_id, buf, &rlen);

		for (fld = 0; fld < nflds; fld++)
		{
		/*
		 * Read the datum for this field
		 */
			memcpy (&datum, buf + bufpos, sizeof (float));
			bufpos += sizeof (float);
		/*
		 * If this is a good point, add it to the data linked list
		 * for this field
		 */
			if (datum != badval)
			{
				struct snd_datum	*prevpt = dptr[fld];
			/*
			 * Allocate the data structure
			 */
				dptr[fld] = (struct snd_datum *)
					calloc (1, sizeof (struct snd_datum));
			/*
			 * Add this datum to the list or make it the head
			 */
				if (prevpt)
					prevpt->next = dptr[fld];
				else
					sounding->dlists[fld] = dptr[fld];
			/*
			 * Fill in the structure
			 */
				dptr[fld]->index = ndx;
				dptr[fld]->value = datum;
				dptr[fld]->prev = prevpt;
			}
		}
	}
/*
 * Close the file and return
 */
	bfclose (file_id);
	return;
}
