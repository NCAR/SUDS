/*
 * Sounding module.  Load, copy, and keep track of soundings.
 *
 * $Revision: 1.3 $ $Date: 1989-08-11 11:04:33 $ $Author: burghart $
 * 
 */
# include <ui_date.h>		/* for date formatting stuff */
# include "globals.h"
# include "sounding.h"
# include "keywords.h"

# define ___ 0

# define MAXRECLEN	512

/*
 * The list of soundings currently open
 */
static struct snd	*Snd_list = 0;

/*
 * Access routines for each format
 */
static void	(*Read_file[MAXFMT + 1])();
static char	*Fmt_name[MAXFMT + 1];

/*
 * Have we initialized?
 */
static int	Init = FALSE;

/*
 * Forward declarations
 */
struct snd	snd_find_sounding ();
void	snd_set_default ();




void
snd_init ()
/*
 * Initialize the array of format-dependent read routines
 */
{
	void	cls_read_file (), ef_read_file (), jaw_read_file ();
	void	noa_read_file (), nws_read_file (), fgge_read_file ();
	void	rsn_read_file ();

	Read_file[SFMT_CLASS]	= cls_read_file;
	Fmt_name[SFMT_CLASS]	= "CLASS";

	Read_file[SFMT_EFMT]	= ef_read_file;
	Fmt_name[SFMT_EFMT]	= "E-format";

	Read_file[SFMT_JAWS]	= jaw_read_file;
	Fmt_name[SFMT_JAWS]	= "JAWS";

	Read_file[SFMT_NOAA]	= noa_read_file;
	Fmt_name[SFMT_NOAA]	= "NOAA";

	Read_file[SFMT_NWS]	= nws_read_file;
	Fmt_name[SFMT_NWS]	= "NWS";

	Read_file[SFMT_FGGE]	= fgge_read_file;
	Fmt_name[SFMT_FGGE]	= "FGGE";

	Read_file[SFMT_RSANAL]	= rsn_read_file;
	Fmt_name[SFMT_RSANAL]	= "RSANAL";

	Init = TRUE;
}




void
snd_read_file (cmds)
struct ui_command	*cmds;
/*
 * Handle the FILE command to get a new sounding
 */
{
	char	*id_name, *fname;
	int	type;
	struct snd	*sounding = Snd_list, *prev = 0;

	fname = UPTR (cmds[0]);
	type = UKEY (cmds[1]);
	id_name = UPTR (cmds[2]);
/*
 * Initialize if necessary
 */
	if (! Init)
		snd_init ();
/*
 * Find the end of the list, looking for name collisions on the way
 */
	while (sounding)
	{
		if (! strcmp (sounding->name, id_name))
			ui_error ("Sounding name '%s' is already used", 
				id_name);
		prev = sounding;
		sounding = sounding->next;
	}
/*
 * Get a new sounding structure
 */
	sounding = (struct snd *) calloc (1, sizeof (struct snd));
/*
 * Read the file using the appropriate routine and put the data 
 * into our structure
 */
	(*Read_file[type]) (fname, sounding);
/*
 * Link it to the end of the list or make it the head of the list
 */
	if (prev)
		prev->next = sounding;
	else
		Snd_list = sounding;
/*
 * Put in the sounding id, filename, and format
 */
	sounding->name = (char *) 
		malloc ((1 + strlen (id_name)) * sizeof (char));
	strcpy (sounding->name, id_name);

	sounding->filename = (char *) 
		malloc ((1 + strlen (fname)) * sizeof (char));
	strcpy (sounding->filename, fname);

	sounding->format = type;
/*
 * Set the default sounding
 */
	snd_set_default (id_name);
/*
 * Done
 */
	return;
}




void
snd_create (cmds)
struct ui_command	*cmds;
/*
 * Create a new sounding from another one
 */
{
	int	i;
	char	*p_name, *id_name;
	struct snd	*sounding = Snd_list, *parent = Snd_list, *prev = 0;
	struct snd_datum	*pdatum, *sdatum, *prevpt;
	fldtype	flds[5];
/*
 * Get the name of the new sounding and the parent sounding
 */
	id_name = UPTR (cmds[0]);
	p_name = UPTR (cmds[1]);
/*
 * Find the parent sounding
 */
	while (parent)
	{
		if (! strcmp (parent->name, p_name))
			break;
		parent = parent->next;
	}

	if (! parent)
		ui_error ("Sounding '%s' does not exist", p_name);
/*
 * Find the end of the list, looking for name collisions on the way
 */
	while (sounding)
	{
		if (! strcmp (sounding->name, id_name))
			ui_error ("Sounding name '%s' is already used", 
				id_name);
		prev = sounding;
		sounding = sounding->next;
	}
/*
 * Get a new sounding structure
 */
	sounding = (struct snd *) calloc (1, sizeof (struct snd));
/*
 * Link it to the end of the list or make it the head of the list
 */
	if (prev)
		prev->next = sounding;
	else
		Snd_list = sounding;
/*
 * Copy the sounding id, filename, and site
 */
	sounding->name = (char *) 
		malloc ((1 + strlen (id_name)) * sizeof (char));
	strcpy (sounding->name, id_name);

	sounding->filename = (char *) 
		malloc ((1 + strlen (parent->filename)) * sizeof (char));
	strcpy (sounding->filename, parent->filename);

	sounding->site = (char *) 
		malloc ((1 + strlen (parent->site)) * sizeof (char));
	strcpy (sounding->site, parent->site);
/*
 * Copy the site altitude, format, and size
 */
	sounding->sitealt = parent->sitealt;
	sounding->format = parent->format;
	sounding->maxndx = parent->maxndx;
/*
 * Copy the field list
 */
	for (i = 0; i < MAXFLDS; i++)
		sounding->fields[i] = parent->fields[i];
/*
 * Copy the data
 */
	for (i = 0; i < MAXFLDS; i++)
	{
		pdatum = parent->dlists[i];
		prevpt = (struct snd_datum *) 0;

		while (pdatum)
		{
		/*
		 * Get a new datum and copy over the index and value
		 */
			sdatum = (struct snd_datum *)
				calloc (1, sizeof (struct snd_datum));

			sdatum->index = pdatum->index;
			sdatum->value = pdatum->value;
		/*
		 * Link it to the list or make it the head of the list
		 */
			if (prevpt)
			{
				sdatum->prev = prevpt;
				prevpt->next = sdatum;
			}
			else
				sounding->dlists[i] = sdatum;
		/*
		 * Move to the next point
		 */
			prevpt = sdatum;
			pdatum = pdatum->next;
		}
	}
/*
 * Done
 */
	return;
}




int
snd_get_data (id_name, buf, buflen, fld, badval)
char	*id_name;
float	*buf, badval;
int	buflen;
fldtype fld;
/*
 * Stuff data from the chosen sounding and field into 'buf'
 */
{
	int	i;
	struct snd	sounding;
	struct snd_datum	*datum;
	int	fpos, npts;

	sounding = snd_find_sounding (id_name);
/*
 * Find the position of this field
 */
	for (fpos = 0; sounding.fields[fpos] != f_null; fpos++)
		if (sounding.fields[fpos] == fld)
			break;

	if (sounding.fields[fpos] == f_null)
		ui_error ("No sounding data for field '%s'", fd_name (fld));
/*
 * Initialize the buffer to bad values
 */
	npts = sounding.maxndx + 1;

	for (i = 0; i < npts && i < buflen; i++)
		buf[i] = badval;
/*
 * Grab the data from the datalist and put it into the buffer
 */
	datum = sounding.dlists[fpos];
	while (datum)
	{
		if (datum->index < buflen)
			buf[datum->index] = datum->value;
		datum = datum->next;
	}
/*
 * Return the number of points
 */
	return (npts);
}




date
snd_time (id_name)
/*
 * Return the start time of the sounding
 */
{
	struct snd	sounding;

	sounding = snd_find_sounding (id_name);
	return (sounding.rls_time);
}



char *
snd_site (id_name)
char	*id_name;
/*
 * Return the site name for this sounding
 */
{
	struct snd	sounding;

	sounding = snd_find_sounding (id_name);
	return (sounding.site);
}




float
snd_s_alt (id_name)
char	*id_name;
/*
 * Return the site altitude
 */
{
	struct snd	sounding;

	sounding = snd_find_sounding (id_name);
	return (sounding.sitealt);
}



void
snd_forget (cmds)
struct ui_command	*cmds;
/*
 * Forget about the given sounding
 */
{
	int	i;
	struct snd	*prev = 0, *sounding = Snd_list;
	struct snd_datum	*datum, *next;
	char	*id_name;
	void	edit_forget ();

	id_name = UPTR (cmds[0]);
/*
 * Find the sounding in the list
 */
	while (sounding)
	{
		if (! strcmp (id_name, sounding->name))
			break;
		prev = sounding;
		sounding = sounding->next;
	}

	if (! sounding)
		ui_error ("There is no sounding with name '%s'", id_name);
/*
 * Release our strings
 */
	free (sounding->name);
	free (sounding->filename);
	free (sounding->site);
/*
 * Release the snd_datum structures
 */
	for (i = 0; i < MAXFLDS; i++)
	{
		datum = sounding->dlists[i];
		while (datum)
		{
			next = datum->next;
			cfree (datum);
			datum = next;
		}
	}
/*
 * Tell edit to forget if necessary
 */
	edit_forget (id_name);
/*
 * Remove it from the sounding list
 */
	if (prev)
		prev->next = sounding->next;
	else
		Snd_list = sounding->next;

	cfree (sounding);
/*
 * Done
 */
	return;
}




struct snd
snd_find_sounding (id_name)
char	*id_name;
/*
 * Return the sounding associated with id_name
 */
{
	struct snd	*sounding = Snd_list;

	while (sounding)
	{
		if (! strcmp (id_name, sounding->name))
			break;
		sounding = sounding->next;
	}

	if (! sounding)
		ui_error ("There is no sounding with name '%s'", id_name);

	return (*sounding);
}




char *
snd_data_ptr (id_name, fld)
char	*id_name;
fldtype	fld;
/*
 * Return a pointer to the data list for the chosen field
 */
{
	struct snd	sounding;
	int	fpos;

	sounding = snd_find_sounding (id_name);
/*
 * Find the position of this field
 */
	for (fpos = 0; sounding.fields[fpos] != f_null; fpos++)
		if (sounding.fields[fpos] == fld)
			break;

	if (sounding.fields[fpos] == f_null)
		ui_error ("No sounding data for field '%s'", fd_name (fld));

	return ((char *) sounding.dlists[fpos]);
}




void
snd_show (cmds)
struct ui_command	*cmds;
/*
 * List the soundings currently available
 */
{
	int	i, brief, nlist;
	struct snd	*sounding = Snd_list;
	char	string[30];

	brief = (cmds[0].uc_ctype == UTT_KW) && (UKEY (cmds[0]) == KW_BRIEF);
/*
 * Print a heading
 */
	if (! sounding)
	{
		ui_nf_printf ("\n    No soundings have been loaded\n");
		return;
	}
	else
		ui_nf_printf ("\n            Sounding List\n");
/*
 * Print a line for each sounding
 */
	nlist = 0;

	while (sounding)
	{
	/*
	 * Just ID's for a brief listing
	 */
		if (brief)
		{
		/*
		 * Commas between each and a new line every 8th sounding
		 */
			if (nlist)
				ui_nf_printf (", ");

			if ((nlist % 8) == 0)
				ui_nf_printf ("\n");
		/*
		 * ID
		 */
			ui_nf_printf ("%s", sounding->name);
			nlist++;
		}
	/*
	 * Tell all for a non-brief listing
	 */
		else
		{
		/*
		 * ID
		 */
			ui_nf_printf ("\nID: %s\n", sounding->name);
		/*
		 * Format
		 */
			ui_nf_printf ("Format: %s  ", 
				Fmt_name[sounding->format]);
		/*
		 * Site
		 */
			ui_nf_printf ("Site: %s  ", sounding->site);
			ui_nf_printf ("Alt: %d m  ", (int) sounding->sitealt);
		/*
		 * Time
		 */
			ud_format_date (string, &sounding->rls_time, UDF_FULL);
			ui_nf_printf ("Time: %s\n", string);
		/*
		 * Fields
		 */
			for (i = 0; sounding->fields[i] != f_null; i++)
			{
				if (i == 0)
					ui_nf_printf ("Fields: ");
				else
				{
					ui_nf_printf (", ");
					if ((i % 10) == 0)
						ui_nf_printf ("\n          ");
				}
				ui_nf_printf ("%s", 
					fd_name (sounding->fields[i]));
			}

			ui_nf_printf ("\n");
		}
	/*
	 * Next sounding
	 */
		sounding = sounding->next;
	}
	ui_printf ("\n");
	return;
}




snd_write_file (cmds)
struct ui_command	*cmds;
/*
 * Write a sounding into a file
 */
{
	char	*id_name, *snd_default ();
	char	*fname = UPTR (cmds[0]);
	struct snd	sounding;
	void	cls_write_file ();
/*
 * Get the sounding name then the sounding
 */
	if (cmds[1].uc_ctype != UTT_END)
		id_name = UPTR (cmds[1]);
	else
		id_name = snd_default ();

	sounding = snd_find_sounding (id_name);
/*
 * Write the CLASS format sounding
 */
	cls_write_file (fname, &sounding);
}
	



void
snd_set_default (id_name)
char	*id_name;
/*
 * Set the current default sounding
 */
{
	strcpy (Def_snd, id_name);
	return;
}



char *
snd_default ()
/*
 * Return the id of the current default sounding
 */
{
	return (Def_snd);
}



void
snd_head (id_name, fld, ptr)
char	*id_name;
fldtype	fld;
struct snd_datum	*ptr;
/*
 * Change the head of the data list for sounding 'id_name', field 'fld'
 * to 'ptr'
 */
{
	struct snd	*sounding = Snd_list;
	int	i;
/*
 * Find the sounding in the list
 */
	while (sounding)
	{
		if (! strcmp (id_name, sounding->name))
			break;
		sounding = sounding->next;
	}

	if (! sounding)
		ui_error ("There is no sounding with name '%s'", id_name);
/*
 * Find the index for the field
 */
	for (i = 0; sounding->fields[i]; i++)
		if (sounding->fields[i] == fld)
			break;
/*
 * Make sure we got it
 */
	if (! sounding->fields[i])
		ui_error ("*BUG* (snd_set_head) -- field '%s' not found",
			fd_name (fld));
/*
 * Change the head
 */
	sounding->dlists[i] = ptr;
}
