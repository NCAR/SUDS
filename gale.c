/*
 * GALE format sounding access (just the CLASS sites)
 *
 * $Revision: 1.1 $ $Date: 1991-10-21 21:46:36 $ $Author: burghart $
 */
# include <stdio.h>
# include <ui_param.h>
# include "globals.h"
# include "sounding.h"

/*
 * 14 fields in GALE data
 */
# define NFLD	14

# define STRSIZE	150

# define BAD	999.9




void
gale_read_file (fname, sounding)
char	*fname;
struct snd	*sounding;
/*
 * Get the GALE sounding from the given file
 */
{
	FILE	*sfile;
	int	ndx, start[NFLD], flen[NFLD], i, status;
	int	month, day, year, siteno;
	struct snd_datum	*dptr[NFLD];
	float	sitelat, sitelon, val[NFLD];
	char	string[STRSIZE], site[4], datestring[7], timestring[5];
	void	gale_insert_data ();
/*
 * Open the file
 */
	if ((sfile = fopen (fname, "r")) == 0)
		ui_error ("Cannot open sounding file '%s'", fname);
/*
 * Read the header line
 */
# ifdef VMS	
/* 
 * Bogus extra fgets to get around Fortran carriage control
 * on the VAX
 */
	fgets (string, STRSIZE - 1, sfile);
	string[0] = ' ';
	fgets (string + 1, STRSIZE - 1, sfile);
# else
	fgets (string, STRSIZE - 1, sfile);
# endif
/*
 * Get the site number and set sounding info based on the site
 */
	strncpy (site, string + 2, 3);
	site[3] = '\0';

	if (strcmp (site, "DUK") == 0)		/* Duck, NC */
	{
		sounding->sitealt = 2;
		sounding->sitelat = 36.14;
		sounding->sitelon = -75.73;
	}
	else if (strcmp (site, "FAY") == 0)	/* Fayetteville, NC */
	{
		sounding->sitealt = 43;
		sounding->sitelat = 35.03;
		sounding->sitelon = -78.72;
	}
	else if (strcmp (site, "ILM") == 0)	/* Wilmington, NC */
	{
		sounding->sitealt = 9;
		sounding->sitelat = 34.27;
		sounding->sitelon = -77.91;
	}
	else if (strcmp (site, "MRH") == 0)	/* Beaufort, NC */
	{
		sounding->sitealt = 2;
		sounding->sitelat = 34.71;
		sounding->sitelon = -76.67;
	}
	else if (strcmp (site, "MYR") == 0)	/* Myrtle Beach, SC */
	{
		sounding->sitealt = 7;
		sounding->sitelat = 33.72;
		sounding->sitelon = -78.89;
	}
	else if (strcmp (site, "PGV") == 0)	/* Greenville, NC */
	{
		sounding->sitealt = 9;
		sounding->sitelat = 35.63;
		sounding->sitelon = -77.37;
	}
	else if (strcmp (site, "SSC") == 0)	/* Sumter, SC */
	{
		sounding->sitealt = 41;
		sounding->sitelat = 33.85;
		sounding->sitelon = -80.32;
	}
	else
		ui_error ("Unknown GALE sounding site: %s", site);
/*
 * Put the site name into the sounding structure
 */
	sounding->site = (char *) malloc (4 * sizeof (char));
	strcpy (sounding->site, site);
/*
 * Get the date and time
 */
	strncpy (datestring, string + 5, 6);
	datestring[6] = '\0';
	sscanf (datestring, "%d", &(sounding->rls_time.ds_yymmdd));

	strncpy (timestring, string + 11, 4);
	timestring[4] = '\0';
	sscanf (timestring, "%d", &(sounding->rls_time.ds_hhmmss));
	sounding->rls_time.ds_hhmmss *= 100;
/*
 * Initialize the data pointers
 */
	for (i = 0; i < NFLD; i++)
		dptr[i] = 0;
/*
 * Assign the fields (hard-wired)
 */
	sounding->fields[0] = f_pres;
	sounding->fields[1] = f_qpres;
	sounding->fields[2] = f_temp;
	sounding->fields[3] = f_qtemp;
	sounding->fields[4] = f_rh;
	sounding->fields[5] = f_qrh;
	sounding->fields[6] = f_u_wind;
	sounding->fields[7] = f_qu;
	sounding->fields[8] = f_v_wind;
	sounding->fields[9] = f_qv;
	sounding->fields[10] = f_alt;
	sounding->fields[11] = f_lat;
	sounding->fields[12] = f_lon;
	sounding->fields[13] = f_dp;
/*
 * Get the data
 */
	for (ndx = 0; ; ndx++)
	{
	/*
	 * Read the next line
	 */
		for (i = 0; i < NFLD; i++)
		{
			status = fscanf (sfile, "%f", &(val[i]));
			if (status != 1)
				break;
		}

		if (status != 1)
			break;
	/*
	 * Insert the data points into their respective data lists
	 */
		gale_insert_data (sounding, ndx, val, dptr);
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
gale_insert_data (sounding, ndx, val, dptr)
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
	 * Don't put bad wind values in the list (u_wind and v_wind
	 * are fields 6 and 8, respectively)
	 */
		if ((i == 6 || i == 8) && ((int)(val[i] - BAD) == 0))
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
