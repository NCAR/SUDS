/*
 * NCAR format sounding access
 *
 * $Revision: 1.1 $ $Date: 1989-11-21 14:45:18 $ $Author: burghart $
 * 
 */
# include <stdio.h>
# include <math.h>
# include <ui_param.h>
# include "sounding.h"
# include "derive.h"

# define R_D	287.
# define G_0	9.80665
# define T_K	273.15

# define DEG_TO_RAD(x)		((x) * 0.017453292)
# define RAD_TO_DEG(x)		((x) * 57.29577951)

# define STRSIZE	200
# define GARBAGE	9999.0

/*
 * The thermo and winds files
 */
static FILE	*Tfile, *Wfile;


/*
 * Winds info
 */
static float	Sfc_wdir, Sfc_wspd, Steptime;
static int	Nloc;
static struct wdata
{
	float		x, y, z;
} S_loc[100];





void
ncar_read_file (fname, sounding)
char	*fname;
struct snd	*sounding;
/*
 * Get the NCAR mobile sounding from the given file
 */
{
	int	i, year, month, day, hour, minute, second;
	int	ndx, status = 0;
	struct snd_datum	*dptr[MAXFLDS], *prevpt;
	float	val[8], temp, rh, dp, e, windtime;
	float	vt, vt_prev, pres, pres_prev = 9999.9, alt_prev;
	char	string[STRSIZE], windflag[2];
	void	ncar_w_init (), ncar_wind ();
/*
 * Open the thermo file and the winds file
 */
	if ((Tfile = fopen (fname, "r")) == 0)
		ui_error ("Cannot open NCAR mobile file '%s'", fname);

	strcpy (string, fname);
	strcat (string, "_wind");
	if ((Wfile = fopen (string, "r")) == 0)
		ui_error ("Cannot open NCAR mobile winds file '%s'", string);
/*
 * Read the sounding release date and time
 */
	fscanf (Tfile, "%d/%d/%d", &month, &day, &year);
	fscanf (Tfile, "%d:%d:%d", &hour, &minute, &second);

	sounding->rls_time.ds_yymmdd = 10000 * year + 100 * month + day;
	sounding->rls_time.ds_hhmmss = 10000 * hour + 100 * minute + second;
/*
 * Read the surface pressure and site altitude
 */
	fscanf (Tfile, "%s", string);
	fscanf (Tfile, "%f", &sounding->sitealt);
/*
 * Read and save the winds data and get the sounding site
 */
	ncar_w_init (&sounding->site);
/*
 * Initialize the data pointers
 */
	for (i = 0; i < MAXFLDS; i++)
		dptr[i] = 0;
/*
 * Assign the fields (hard-wired)
 */
	sounding->fields[0] = f_time;	sounding->fields[1] = f_temp;
	sounding->fields[2] = f_rh;	sounding->fields[3] = f_pres;
	sounding->fields[4] = f_dp;	sounding->fields[5] = f_alt;
	sounding->fields[6] = f_wspd;	sounding->fields[7] = f_wdir;
	sounding->fields[8] = f_null;
/*
 * Get the data
 */
	for (ndx = 0; ; ndx++)
	{
	/*
	 * Read the data values from the next line
	 */
		for (i = 0; i < 4 && status != EOF; i++)
			status = fscanf (Tfile, "%f", &val[i]);

		if (status == EOF || status == 0)
			break;
	/*
	 * Convert the time from mmss to seconds
	 */
		val[0] = 60 * ((int)val[0] / 100) + (int)val[0] % 100;
	/*
	 * Kluge: derive the dewpoint here, since it isn't supplied for
	 * us and we need it to get altitude (which we need to put the winds
	 * in the right place).
	 */
		temp = val[1];
		rh = val[2];
		if (temp < 8888. && rh < 8888.)
		{
			e = rh / 100.0 * e_w (temp + T_K);
			dp = dewpoint (e) - T_K;
			val[4] = dp;
		}
		else
			val[4] = GARBAGE;
	/*
	 * More kluge: derive the altitude field, since we need it to 
	 * do the winds data correctly.  This derivation is adapted from 
	 * equation 2.24 of Wallace and Hobbs' "Atmospheric Science" (1977) 
	 * using a simple estimation of the integral.
	 */
		dp = val[4];
		pres = val[3];
		if (pres < GARBAGE && dp < GARBAGE && temp < GARBAGE)
		{
			e = e_from_dp (dp + T_K);
			vt = t_v (temp + T_K, pres, e);
		/*
		 * Assign the altitude (the first good point is set to
		 * 0 km altitude
		 */
			if (pres_prev >= GARBAGE)
				val[5] = 0.0;
			else
				val[5] = alt_prev + R_D / G_0 * 0.5 * 
					(vt / pres + vt_prev / pres_prev) * 
					(pres_prev - pres);

			pres_prev = pres;
			vt_prev = vt;
			alt_prev = val[5];
		}
		else
			val[5] = GARBAGE;
	/*
	 * If this line has an asterisk on it, get the wind, otherwise
	 * put in bad values
	 */
		fscanf (Tfile, "%2c", windflag);

		if (windflag[1] == '*')
			ncar_wind (val[5], &val[6], &val[7]);
		else
			val[6] = val[7] = GARBAGE;		
	/*
	 * Put the data points into their respective data lists
	 */
		for (i = 0; i < 8; i++)
		{
		/*
		 * Don't put bad values in the list
		 */
			if (val[i] >= GARBAGE)
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
	fclose (Tfile);
	fclose (Wfile);
	return;
}




void
ncar_w_init (site)
char	**site;
/*
 * Get everything from the winds file and save it in the winds array
 * Also create a "site" name, since that info is in the winds file
 */
{
	float	az, el, alt;
	float	steptime, dummy, sitealt;
	int	index, status;
	char	id1[20], id2[20];
/*
 * Read the header lines from the winds file
 */
	fscanf (Wfile, "%s %s", id1, id2);
	fscanf (Wfile, "%f %f", &sitealt, &Steptime);
	fscanf (Wfile, "%f %f", &Sfc_wdir, &Sfc_wspd);
	fscanf (Wfile, "%f %f", &dummy, &dummy);
/*
 * Use the sounding id as the "site"
 */
	*site = (char *) malloc (10 * sizeof (char));
	sprintf (*site, "%s %s", id1, id2);
/*
 * Initial sonde position
 */
	S_loc[0].x = S_loc[0].y = S_loc[0].z = 0.0;
	Nloc = 1;
/*
 * Loop through and get all of the points
 */
	status = fscanf (Wfile, "%d %f %f %f", &index, &az, &el, &alt);

	while (status != EOF)
	{
	/*
	 * Convert the altitude to ground relative and the angles
	 * to radians
	 */
		alt -= sitealt;
		az = DEG_TO_RAD (az);
		el = DEG_TO_RAD (el);
	/*
	 * Save x, y, and z location for this point
	 */
		S_loc[Nloc].x = sin (az) * alt / tan (el);
		S_loc[Nloc].y = cos (az) * alt / tan (el);
		S_loc[Nloc].z = alt;

		Nloc++;
	/*
	 * Read the next point
	 */
		status = fscanf (Wfile, "%d %f %f %f", &index, &az, &el, &alt);
	}
}




void
ncar_wind (alt, wspd, wdir)
float	alt, *wspd, *wdir;
/*
 * Return the wind for the given altitude
 */
{
	float	del_x, del_y, mid;
	int	i;
/*
 * Just return the surface wind below 20 m
 */
	if (alt < 20.0)
	{
		*wspd = Sfc_wspd;
		*wdir = Sfc_wdir;
		return;
	}
/*
 * Return bad values if the altitude is above the highest sonde position
 */
	if (alt > S_loc[Nloc-1].z)
	{
		*wspd = GARBAGE;
		*wdir = GARBAGE;
		return;
	}
/*
 * Find the first sonde location above the given altitude
 */
	for (i = 0; i < Nloc && S_loc[i].z < alt; i++)
		/* nothing */;
# ifdef notdef
/*
 * Complain if the chosen altitude isn't close to midway between two
 * sonde locations
 */
	mid = (S_loc[i].z + S_loc[i-1].z) / 2.0;
	if (fabs (alt - mid) > 10.0)
		ui_warning ("Altitude discrepancy! at: %.1f  mid:%.1f",
			alt, mid);
# endif
/*
 * Compute speed and direction from the points straddling the given altitude
 */
	del_x = S_loc[i].x - S_loc[i-1].x;
	del_y = S_loc[i].y - S_loc[i-1].y;

	*wspd = hypot (del_x, del_y) / Steptime;
	*wdir = RAD_TO_DEG (atan2 (del_x, del_y));
	*wdir += 180.0;
}
