/*
 * RSANAL format sounding access
 *
 * $Revision: 1.4 $ $Date: 1989-12-20 14:08:02 $ $Author: burghart $
 * 
 */
# include <stdio.h>
# include <ui_param.h>
# include "globals.h"
# include "sounding.h"

# define TRUE	1
# define FALSE	0
# define BAD	-9999

/*
 * 6 fields in RSANAL data
 */
# define NFLD	6

# define STRSIZE	100


int	rsn_int_extract ();



void
rsn_read_file (fname, sounding)
char	*fname;
struct snd	*sounding;
/*
 * Get the RSANAL format sounding from the given file
 */
{
	int	sfile, ndx, i, status, start = 0;
	struct snd_datum	*dptr[NFLD];
	float	site_x, site_y, val[NFLD];
	char	string[STRSIZE], site[27];
	void	rsn_insert_data ();
/*
 * Open the file
 */
	if ((sfile = dview (fname)) == 0)
		ui_error ("Cannot open sounding file '%s'", fname);
/*
 * Read the header line
 */
	status = dget (sfile, string, STRSIZE);
	if (status < 0)
		ui_error ("Cannot read header line");
/*
 * Read the date and time
 */
	sounding->rls_time.ds_yymmdd = rsn_int_extract (string, 0, 6);
	sounding->rls_time.ds_hhmmss = rsn_int_extract (string, 6, 4) * 100;
/*
 * Put the site into the sounding structure
 */
	strncpy (site, string + 10, 26);
	site[26] = 0;
	for (i = 25; site[i] == ' '; i--)
		site[i] = (char) 0;

	sounding->site = (char *) 
		malloc ((1 + strlen (site)) * sizeof (char));
	strcpy (sounding->site, site);
/*
 * Get the station location
 */
	sounding->sitealt = (float) rsn_int_extract (string, 45, 4);

	status = sscanf (string + 62, "%f", &sounding->sitelon);
	if (status == EOF)
		ui_error ("Bad formatted read -- Is this a RSANAL file?");

	status = sscanf (string + 73, "%f", &sounding->sitelat);
	if (status == EOF)
		ui_error ("Bad formatted read -- Is this a RSANAL file?");
/*
 * Kluge:  See if the lat and lon are really x and y; assume they are
 * x,y if the "lon" is greater than -60.0 (i.e., not in the U.S.).  This
 * assumption may cause some problems somewhere along the way, but it's 
 * the most obvious automatic solution for now.
 */
	if (sounding->sitelon > -60.0)
	{
		site_x = sounding->sitelon * 1.609344; /* miles to km */
		site_y = sounding->sitelat * 1.609344;
		cvt_to_latlon (site_x, site_y, &sounding->sitelat, 
			&sounding->sitelon);
	}
/*
 * Initialize the data pointers
 */
	for (i = 0; i < NFLD; i++)
		dptr[i] = 0;
/*
 * Assign the fields (hard-wired)
 */
	sounding->fields[0] = f_pres;
	sounding->fields[1] = f_temp;
	sounding->fields[2] = f_dp;
	sounding->fields[3] = f_wspd;
	sounding->fields[4] = f_wdir;
	sounding->fields[5] = f_alt;
/*
 * Get the data
 */
	for (ndx = 0; ; ndx++)
	{
	/*
	 * Read the next line or do the second half of the previous line
	 */
		if (start == 4)
			start = 44;
		else
		{
		/*
		 * read the next record
		 */
			status = dget (sfile, string, STRSIZE);
		/*
		 * make sure we got something
		 */
			if (status < 0)
				break;
			start = 4;
		}
	/*
	 * Read the data values and scale if necessary
	 */
		for (i = 0; i < NFLD; i++)
		{
			val[i] = (float) 
				rsn_int_extract (string, start + 6 * i, 6);
		/*
		 * Scale the field
		 */
			if (val[i] != BAD)
				val[i] *= (i == 0 || i == 4) ? 0.1 : 0.01;

			if (val[i] == 999.)
				val[i] = BAD;
		/*
		 * Change the units from 1000's of feet to meters for the
		 * altitude field
		 */
			if (i == 5 && val[i] != BAD)
				val[i] *= 304.8;
		}
	/*
	 * Insert the data
	 * Put the data points into their respective data lists
	 */
		rsn_insert_data (sounding, ndx, val, dptr);
	}
/*
 * Set the max index number for the sounding
 */
	sounding->maxndx = ndx - 1;
/*
 * Close the file and return
 */
	dclose (sfile);
	return;
}




void
rsn_insert_data (sounding, ndx, val, dptr)
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
rsn_int_extract (string, start, len)
char	*string;
int	start, len;
/*
 * Extract an integer of length 'len' from the given string starting 
 * at position 'start'.  (This does floats too, it just ignores the decimal
 * point)
 */
{
	int	ival = 0, neg = FALSE, ndx, ndigits = 0;
/*
 * Read the number
 */
	for (ndx = start; ndx < start + len; ndx++)
	{
		char	ch = string[ndx];
	/*
	 * Check for signs, decimals, or spaces
	 */
		if (ch == '-')
		{
			neg = TRUE;
			continue;
		}
		else if (ch == ' ' || ch == '.' || ch == '+')
			continue;
	/*
	 * Error on other non-digits
	 */
		if (ch < '0' || ch > '9')
		{
			char	*temp = (char *) 
					malloc ((len + 1) * sizeof (char));

			strncpy (temp, string + start, len);
			temp[len] = 0;

			ui_error (
				"Non-integer '%s' -- Is this a RSANAL file?",
				temp);
		}
	/*
	 * We have a digit, update our value
	 */
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
