/*
 * NOAA format sounding access
 *
 * $Revision: 1.6 $ $Date: 1989-12-19 15:52:25 $ $Author: burghart $
 * 
 */
# include <stdio.h>
# include <ui_param.h>
# include "sounding.h"

# define STRSIZE	200



void
noa_read_file (fname, sounding)
char	*fname;
struct snd	*sounding;
/*
 * Get the NOAA sounding from the given file
 */
{
	FILE	*sfile;
	int	i, year, month, day, hour, minute, second;
	int	ndx, status = 0;
	struct snd_datum	*dptr[MAXFLDS], *head[MAXFLDS], *prevpt;
	float	sfc_pres, xloc, yloc, val[8];
	char	string[STRSIZE], c = ' ';
/*
 * Open the file
 */
	if ((sfile = fopen (fname, "r")) == 0)
		ui_error ("Cannot open NOAA file '%s'", fname);
/*
 * Read the sounding number and x,y location in statute miles
 */
	fscanf (sfile, "%s", string);
	fscanf (sfile, "%f%f", &xloc, &yloc);

	sounding->site = (char *) malloc (10 * sizeof (char));
	sounding->site[9] = 0;
	sprintf (sounding->site, "%.1f %.1f", xloc, yloc);
/*
 * Convert the x,y to kilometers and then to lat,lon
 */
	xloc *= 1.609344;
	yloc *= 1.609344;
	cvt_to_latlon (xloc, yloc, &sounding->sitelat, &sounding->sitelon);
/*
 * Finish off the line
 */
	while (c != '\n' && c != 0)
		c = (char) fgetc (sfile);
/*
 * Sounding release date and time
 */
	fscanf (sfile, "%d/%d/%d", &month, &day, &year);
	fscanf (sfile, "%d:%d:%d", &hour, &minute, &second);

	sounding->rls_time.ds_yymmdd = 10000 * year + 100 * month + day;
	sounding->rls_time.ds_hhmmss = 10000 * hour + 100 * minute + second;
/*
 * Surface pressure and altitude 
 */
	fscanf (sfile, "%f\n", &sfc_pres);
	fscanf (sfile, "%f\n", &(sounding->sitealt));
/*
 * Initialize the data pointers
 */
	for (i = 0; i < MAXFLDS; i++)
		dptr[i] = 0;
/*
 * Assign the fields (hard-wired)
 */
	sounding->fields[0] = f_time;	sounding->fields[1] = f_pres;
	sounding->fields[2] = f_temp;	sounding->fields[3] = f_rh;
	sounding->fields[4] = f_wspd;	sounding->fields[5] = f_wdir;
	sounding->fields[6] = f_null;
/*
 * Get the data
 */
	for (ndx = 0; ; ndx++)
	{
	/*
	 * Read the data values from the next line
	 */
		for (i = 0; i < 8 && status != EOF; i++)
			status = fscanf (sfile, " %f ", &val[i]);

		if (status == EOF || status == 0)
			break;
	/*
	 * Convert the time (if good) from mmss to seconds
	 */
		minute = (int) val[0] / 100;
		second = (int) val[0] % 100;
		val[0] = 60 * minute + second;
	/*
	 * Put the eight data points into their respective data lists
	 */
		for (i = 0; i < 8; i++)
		{
		/*
		 * We don't use fields 4 and 5 from the file
		 */
			if (i == 4 || i == 5)
				continue;
		/*
		 * Don't put bad values in the list
		 */
			if (val[i] > 8888.)
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
				head[i] = dptr[i];
		}
	}
/*
 * Assign the sounding datalists.  We don't use fields 4 and 5
 * from the file (azimuth and elevation), since winds are already
 * derived for us.
 */
	sounding->dlists[0] = head[0];
	sounding->dlists[1] = head[1];
	sounding->dlists[2] = head[2];
	sounding->dlists[3] = head[3];
	sounding->dlists[4] = head[6];
	sounding->dlists[5] = head[7];
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
