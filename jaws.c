/*
 * JAWS format sounding access
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  89/03/16  15:14:47  burghart
 * Initial revision
 * 
 */
# include <stdio.h>
# include <ui_param.h>
# include "sounding.h"
# include "derive.h"

# define T_K	273.15

# define STRSIZE	200

/*
 * 11 fields in JAWS data
 */
# define NFLD	11

void
jaw_read_file (fname, sounding)
char	*fname;
struct snd	*sounding;
/*
 * Get the JAWS sounding from the given file
 */
{
	FILE	*sfile;
	int	i, year, month, day, hour, minute;
	int	ndx, status = 0;
	struct snd_datum	*dptr[NFLD], *prevpt;
	float	val[NFLD];
	char	string[STRSIZE];
/*
 * Open the file
 */
	if ((sfile = fopen (fname, "r")) == 0)
		ui_error ("Cannot open JAWS file '%s'", fname);
/*
 * Sounding release date and time
 */
	fscanf (sfile, "%d %d %2d%2d%2d", &month, &day, &year, &hour, &minute);

	sounding->rls_time.ds_yymmdd = 10000 * year + 100 * month + day;
	sounding->rls_time.ds_hhmmss = 10000 * hour + 100 * minute;
/*
 * Site
 */
	sounding->site = (char *) malloc (7 * sizeof (char));
	strcpy (sounding->site, "Denver");
	sounding->sitealt = 1611.;	/* Alt of Denver NWS site */
/*
 * Initialize the data pointers
 */
	for (i = 0; i < NFLD; i++)
		dptr[i] = 0;
/*
 * Assign the fields (hard-wired)
 */
	sounding->fields[0] = f_time;	sounding->fields[1] = f_alt;
	sounding->fields[2] = f_pres;	sounding->fields[3] = f_temp;
	sounding->fields[4] = f_dp;	sounding->fields[5] = f_wdir;
	sounding->fields[6] = f_wspd;	sounding->fields[7] = f_theta;
	sounding->fields[8] = f_theta_e;sounding->fields[9] = f_mr;
	sounding->fields[10] = f_rh;	sounding->fields[11] = f_null;
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
	 * Put the data points into their respective data lists
	 */
		for (i = 0; i < NFLD; i++)
		{
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
		 * Convert altitude to ground relative
		 */
			if (i == 1)
				val[i] -= sounding->sitealt;
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
