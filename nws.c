/*
 * NWS format sounding access
 *
 * $Revision: 1.7 $ $Date: 1991-03-21 21:30:35 $ $Author: burghart $
 * 
 */
# include <stdio.h>
# include <ui_param.h>
# include "sounding.h"

/*
 * 7 fields in NWS data
 */
# define NFLD	7

# define STRSIZE	100

static char *Mnth[13] = 
{
	"", "JAN", "FEB", "MAR", "APR", "MAY", "JUN", "JUL",
	"AUG", "SEP", "OCT", "NOV", "DEC"
};

/*
 * True NWS format?
 */
static int	True_nws;




void
nws_read_file (fname, sounding)
char	*fname;
struct snd	*sounding;
/*
 * Get the NWS sounding from the given file
 */
{
	FILE	*sfile;
	int	i, year, month, day, rtype, status = 1;
	int	ndx, time, did_sfc = FALSE;
	struct snd_datum	*dptr[NFLD];
	float	sfc_pres, sfc[NFLD], val[NFLD], pres;
	char	string[STRSIZE];
	void	nws_insert_data ();
/*
 * Open the file
 */
	if ((sfile = fopen (fname, "r")) == 0)
		ui_error ("Cannot open NWS file '%s'", fname);
/*
 * Sounding release date and time
 */
	fscanf (sfile, "%d %d %d %s %d", &rtype, &time, &day, string, &year);
/*
 * Get the month name without leading blanks and convert it to a number
 */
	for (month=1; month < 13 && strncmp (string, Mnth[month], 3); month++)
		/* nothing */;

	if (month == 13)
		ui_error ("Unknown month name '%s'", string);

	year -= 1900;
	sounding->rls_time.ds_yymmdd = 10000 * year + 100 * month + day;
	sounding->rls_time.ds_hhmmss = 10000 * time;
/*
 * Site info
 */
	fscanf (sfile, "%f %f %f", &sounding->sitelat, &sounding->sitelon, 
		&sounding->sitealt);
	sounding->sitelon *= -1.0;	/* Make west longitudes negative */
/*
 * A record of stuff we don't use
 */
	fgets (string, STRSIZE - 1, sfile);	/* Finish off the last line */
	fgets (string, STRSIZE - 1, sfile);
/*
 * Read the site name and put it into the sounding structure
 * Kluge:  They leave the '3' record type in the "true" NWS format soundings 
 * but not in the modified ones, so we use this to set True_nws
 */
	fscanf (sfile, "%s", string);
	if (string[0] == '3')
	{
		fscanf (sfile, "%s", string);
		True_nws = TRUE;
	}
	else
		True_nws = FALSE;
/*
 * Put the site into the sounding structure
 */
	sounding->site = (char *) 
		malloc ((1 + strlen (string)) * sizeof (char));
	strcpy (sounding->site, string);
/*
 * Initialize the data pointers
 */
	for (i = 0; i < NFLD; i++)
		dptr[i] = 0;
/*
 * Assign the fields (hard-wired)
 */
	sounding->fields[0] = f_rtype;	sounding->fields[1] = f_pres;
	sounding->fields[2] = f_alt;	sounding->fields[3] = f_temp;
	sounding->fields[4] = f_dp;	sounding->fields[5] = f_wdir;
	sounding->fields[6] = f_wspd;	sounding->fields[7] = f_null;
/*
 * Read the surface data and save it for insertion at the proper location
 */
	for (i = 0; i < NFLD; i++)
		fscanf (sfile, " %f ", &sfc[i]);

	sfc_pres = sfc[1];
/*
 * Get the data
 */
	for (ndx = 0; ; ndx++)
	{
	/*
	 * Read the data values from the next line
	 */
		for (i = 0; i < NFLD && status != EOF; i++)
			status = fscanf (sfile, " %f ", &val[i]);

		if (status == EOF)
			break;
	/*
	 * Get the pressure and record type
	 */
		pres = val[1];
		rtype = (int) val[0];
	/*
	 * Quit if we are starting another sounding
	 */
		if (rtype == 254)
		{
			ui_printf ("More than one sounding in this file, ");
			ui_printf ("only the first sounding is being read\n");
			break;
		}
	/*
	 * If we passed the surface pressure, insert the surface data now
	 */
		if (pres < sfc_pres && ! did_sfc)
		{
			nws_insert_data (sounding, ndx, sfc, dptr);
			ndx++;
			did_sfc = TRUE;
		}
	/*
	 * Insert the data
	 * Put the seven data points into their respective data lists
	 */
		nws_insert_data (sounding, ndx, val, dptr);
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
nws_insert_data (sounding, ndx, val, dptr)
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
		if (val[i] > 9999.)
			continue;
	/*
	 * True NWS format soundings have a scaling factor for temp, 
	 * dp, and wspd
	 */
		if (True_nws && (i == 3 || i == 4 || i == 6))
			val[i] = 0.1 * val[i];
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
