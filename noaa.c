/*
 * NOAA format sounding access
 *
 * $Revision: 1.4 $ $Date: 1989-09-19 16:08:30 $ $Author: burghart $
 * 
 */
# include <stdio.h>
# include <ui_param.h>
# include "sounding.h"
# include "derive.h"

# define R_D	287.
# define G_0	9.80665
# define T_K	273.15

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
	struct snd_datum	*dptr[MAXFLDS], *prevpt;
	float	sfc_pres, xloc, yloc, val[10], temp, rh, dp, e;
	float	vt, vt_prev, pres, pres_prev = 9999.9, alt_prev;
	char	string[STRSIZE], c = ' ';
/*
 * Open the file
 */
	if ((sfile = fopen (fname, "r")) == 0)
		ui_error ("Cannot open NOAA file '%s'", fname);
/*
 * Read the sounding number and location (?)
 */
	fscanf (sfile, "%s", string);
	fscanf (sfile, "%f%f", &xloc, &yloc);

	sounding->site = (char *) malloc (10 * sizeof (char));
	sounding->site[9] = 0;
	sprintf (sounding->site, "%.1f %.1f", xloc, yloc);
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
 * (Kluge: the dewpoint field is not in the data but we derive it and treat
 *	it like part of the original data)
 */
	sounding->fields[0] = f_time;	sounding->fields[1] = f_pres;
	sounding->fields[2] = f_temp;	sounding->fields[3] = f_rh;
	sounding->fields[4] = f_temp;	sounding->fields[5] = f_temp;
	sounding->fields[6] = f_wspd;	sounding->fields[7] = f_wdir;
	sounding->fields[8] = f_dp;	sounding->fields[9] = f_alt;
	sounding->fields[10] = f_null;
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
	 * Kluge: derive the dewpoint here, since it isn't supplied for
	 *  us and field derivation isn't available yet.
	 */
		temp = val[2];
		rh = val[3];
		if (temp < 8888. && rh < 8888.)
		{
			e = rh / 100.0 * e_w (temp + T_K);
			dp = dewpoint (e) - T_K;
			val[8] = dp;
		}
		else
			val[8] = 9999.9;
	/*
	 * More kluge: derive the altitude field also.  This derivation
	 * is adapted from equation 2.24 of Wallace and
	 * Hobbs' "Atmospheric Science" (1977) using a simple estimation
	 * of the integral.
	 */
		dp = val[8];
		pres = val[1];
		if (pres < 8888. && dp < 8888. && temp < 8888.)
		{
			e = e_from_dp (dp + T_K);
			vt = t_v (temp + T_K, pres, e);
		/*
		 * Assign the altitude (the first good point is set to
		 * 0 km altitude
		 */
			if (pres_prev > 8888.)
				val[9] = 0.0;
			else
				val[9] = alt_prev + R_D / G_0 * 0.5 * 
					(vt / pres + vt_prev / pres_prev) * 
					(pres_prev - pres);

			pres_prev = pres;
			vt_prev = vt;
			alt_prev = val[9];
		}
		else
			val[9] = 9999.9;
	/*
	 * Put the ten data points into their respective data lists
	 */
		for (i = 0; i < 10; i++)
		{
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
	fclose (sfile);
	return;
}
