/*
 * MIST format sounding access
 *
 * $Revision: 1.2 $ $Date: 1991-03-20 23:05:14 $ $Author: burghart $
 */
# include <stdio.h>
# include <ui_param.h>
# include "globals.h"
# include "sounding.h"

/*
 * 15 fields in MIST data
 */
# define NFLD	15

# define STRSIZE	100

# define BAD	-9999.0

float	mist_extract ();



void
mist_read_file (fname, sounding)
char	*fname;
struct snd	*sounding;
/*
 * Get the MIST sounding from the given file
 */
{
	FILE	*sfile;
	int	ndx, start[NFLD], flen[NFLD], i;
	int	month, day, year, siteno;
	struct snd_datum	*dptr[NFLD];
	float	sitelat, sitelon, val[NFLD];
	char	string[STRSIZE], site[10], *status;
	void	mist_insert_data ();
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
 * Get the site number and set sounding info based on the site
 */
	siteno = mist_extract (string, 0, 5);
	switch (siteno)
	{
	    case 10000:	/* Site 10012 sometimes is reported as site 10000 */
		sounding->sitelat = 34.6250;
		sounding->sitelon = -86.6458;
		sounding->sitealt = 182;
		break;
	    case 10001:
		sounding->sitelat = 34.4567;
		sounding->sitelon = -85.8608;
		sounding->sitealt = 375;
		break;
	    case 10002:
		sounding->sitelat = 34.1436;
		sounding->sitelon = -87.3378;
		sounding->sitealt = 229;
		break;
	    case 10003:
		sounding->sitelat = 34.5936;
		sounding->sitelon = -88.6478;
		sounding->sitealt = 117;
		break;
	    case 10004:
		sounding->sitelat = 34.8672;
		sounding->sitelon = -86.7075;
		sounding->sitealt = 248;
		break;
	    case 10005:
		sounding->sitelat = 35.0258;
		sounding->sitelon = -87.4803;
		sounding->sitealt = 247;
		break;
	    case 10006:
		sounding->sitelat = 35.7033;
		sounding->sitelon = -85.8392;
		sounding->sitealt = 317;
		break;
	    case 10007:
		sounding->sitelat = 35.7153;
		sounding->sitelon = -86.9633;
		sounding->sitealt = 220;
		break;
	    case 10008:
		sounding->sitelat = 35.6514;
		sounding->sitelon = -88.3828;
		sounding->sitealt = 154;
		break;
	    case 10009:
		sounding->sitelat = 36.5444;
		sounding->sitelon = -86.9183;
		sounding->sitealt = 216;
		break;
	    case 10010:
		sounding->sitelat = 34.7097;
		sounding->sitelon = -87.0894;
		sounding->sitealt = 200;
		break;
	    case 10011:
		sounding->sitelat = 34.6100;
		sounding->sitelon = -86.6317;
		sounding->sitealt = 174;
		break;
	    case 10012:
		sounding->sitelat = 34.6250;
		sounding->sitelon = -86.6458;
		sounding->sitealt = 182;
		break;
	    default:
		ui_error ("Unknown MIST sounding site number: %d", siteno);
	}
/*
 * Put the site name into the sounding structure
 */
	sounding->site = (char *) malloc (29 * sizeof (char));
	strncpy (sounding->site, string + 5, 28);
	sounding->site[28] = '\0';
/*
 * Strip trailing spaces from the sounding site
 */
	for (i = 27; sounding->site[i] == ' '; i--)
		sounding->site[i] = '\0';
/*
 * Get the date and time
 */
	day = (int) mist_extract (string, 33, 2);
	month = (int) mist_extract (string, 42, 2);
	year = (int) mist_extract (string, 44, 2);
	sounding->rls_time.ds_yymmdd = 10000 * year + 100 * month + day;
	sounding->rls_time.ds_hhmmss = (int)mist_extract (string, 46, 4) * 100;
/*
 * Initialize the data pointers
 */
	for (i = 0; i < NFLD; i++)
		dptr[i] = 0;
/*
 * Assign the fields (hard-wired)
 */
	sounding->fields[0] = f_time;	start[0] = 0;	flen[0] = 5;
	sounding->fields[1] = f_alt;	start[1] = 10;	flen[1] = 7;
	sounding->fields[2] = f_pres;	start[2] = 17;	flen[2] = 6;
	sounding->fields[3] = f_temp;	start[3] = 23;	flen[3] = 5;
	sounding->fields[4] = f_dp;	start[4] = 28;	flen[4] = 5;
	sounding->fields[5] = f_wdir;	start[5] = 33;	flen[5] = 5;
	sounding->fields[6] = f_wspd;	start[6] = 38;	flen[6] = 4;
	sounding->fields[7] = f_u_wind;	start[7] = 42;	flen[7] = 5;
	sounding->fields[8] = f_v_wind;	start[8] = 47;	flen[8] = 5;
	sounding->fields[9] = f_theta;	start[9] = 52;	flen[9] = 5;
	sounding->fields[10] = f_theta_e;start[10] = 57;flen[10] = 5;
	sounding->fields[11] = f_mr;	start[11] = 62;	flen[11] = 4;	
	sounding->fields[12] = f_rh;	start[12] = 66;	flen[12] = 5;
	sounding->fields[13] = f_range;	start[13] = 71;	flen[13] = 5;
	sounding->fields[14] = f_azimuth;start[14] = 76;flen[14] = 4;
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
			val[i] = mist_extract (string, start[i], flen[i]);
	/*
	 * Convert time to seconds
	 */
		if (val[0] != BAD)
			val[0] *= 60.0;
	/*
	 * Insert the data points into their respective data lists
	 */
		mist_insert_data (sounding, ndx, val, dptr);
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
mist_insert_data (sounding, ndx, val, dptr)
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



float
mist_extract (string, start, len)
char	*string;
int	start, len;
/*
 * Extract an integer or float of length 'len' from the given string starting 
 * at position 'start'
 */
{
	int	neg = FALSE, ndx, ndigits = 0, ddigits = 0;
	int	bad = TRUE, decimal = FALSE;
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
			return (BAD);
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
		/*
		 * We have a bad value if all digits are 9's
		 */
			bad = bad && ch == '9';
		}
	}
/*
 * Return the appropriate value
 */
	if (ndigits == 0  || bad)
		return (BAD);
	else if (neg)
		return (-val);
	else
		return (val);
}
