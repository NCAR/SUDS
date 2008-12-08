/*
 * Sounding module.  Load, copy, and keep track of soundings.
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

static char *rcsid = "$Id: sounding.c,v 1.20 1993-04-28 16:21:23 carson Exp $";

# include <stdlib.h>

# include <ui_param.h>
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
/* Return a count of soundings in the file */
static int	(*SoundingCountFunc[MAXFMT + 1])();
/* Read a sounding from the file */
static void	(*GetSoundingFunc[MAXFMT + 1])();
static char	*Fmt_name[MAXFMT + 1];

/*
 * Have we initialized?
 */
static int	Init = FALSE;

/*
 * Forward declarations
 */
struct snd	snd_find_sounding();
void	snd_load_file(), snd_set_default();
int	interp();
double	fd_ibot(), fd_itop(), fd_istep(), fd_igap();
int snd_count_one();




void
snd_init()
/*
 * Initialize the array of format-dependent read routines
 */
{
	void	cls_read_file(), jaw_read_file();
	void	noa_read_file(), nws_read_file(), fgge_read_file();
	void	rsn_read_file(), ef_read_file(), ncar_read_file();
	void	mist_read_file(), gale_read_file(), nmc_read_file();
	void	nc_read_file(), cape_read_file(), nssl_read_file();
	void	drexel_read_file(), wmo_get_sounding();
	int	wmo_snd_count();

	GetSoundingFunc[SFMT_CLASS]	= cls_read_file;
	SoundingCountFunc[SFMT_CLASS]	= 0;
	Fmt_name[SFMT_CLASS] = "CLASS";

	GetSoundingFunc[SFMT_JAWS]	= jaw_read_file;
	SoundingCountFunc[SFMT_JAWS]	= 0;
	Fmt_name[SFMT_JAWS] = "JAWS";

	GetSoundingFunc[SFMT_NOAA]	= noa_read_file;
	SoundingCountFunc[SFMT_NOAA]	= 0;
	Fmt_name[SFMT_NOAA] = "NOAA mobile";

	GetSoundingFunc[SFMT_NWS]		= nws_read_file;
	SoundingCountFunc[SFMT_NWS]	= 0;
	Fmt_name[SFMT_NWS] = "NWS";

	GetSoundingFunc[SFMT_FGGE]	= fgge_read_file;
	SoundingCountFunc[SFMT_FGGE]	= 0;
	Fmt_name[SFMT_FGGE] = "FGGE";

	GetSoundingFunc[SFMT_RSANAL]	= rsn_read_file;
	SoundingCountFunc[SFMT_RSANAL]	= 0;
	Fmt_name[SFMT_RSANAL] = "RSANAL";

	GetSoundingFunc[SFMT_EFMT]	= ef_read_file;
	SoundingCountFunc[SFMT_EFMT]	= 0;
	Fmt_name[SFMT_EFMT] = "E-format";

	GetSoundingFunc[SFMT_NCAR]	= ncar_read_file;
	SoundingCountFunc[SFMT_NCAR]	= 0;
	Fmt_name[SFMT_NCAR] = "NCAR mobile";

	GetSoundingFunc[SFMT_MIST]	= mist_read_file;
	SoundingCountFunc[SFMT_MIST]	= 0;
	Fmt_name[SFMT_MIST] = "MIST";

	GetSoundingFunc[SFMT_GALE]	= gale_read_file;
	SoundingCountFunc[SFMT_GALE]	= 0;
	Fmt_name[SFMT_GALE] = "GALE";

	GetSoundingFunc[SFMT_NMC]		= nmc_read_file;
	SoundingCountFunc[SFMT_NMC]	= 0;
	Fmt_name[SFMT_NMC] = "NMC";

	GetSoundingFunc[SFMT_NETCDF]	= nc_read_file;
	SoundingCountFunc[SFMT_NETCDF]	= 0;
	Fmt_name[SFMT_NETCDF] = "netCDF";

	GetSoundingFunc[SFMT_CAPE]	= cape_read_file;
	SoundingCountFunc[SFMT_CAPE]	= 0;
	Fmt_name[SFMT_CAPE] = "CaPE";

	GetSoundingFunc[SFMT_NSSL]	= nssl_read_file;
	SoundingCountFunc[SFMT_NSSL]	= 0;
	Fmt_name[SFMT_NSSL] = "NSSL";

	GetSoundingFunc[SFMT_DREXEL]	= drexel_read_file;
	SoundingCountFunc[SFMT_DREXEL]	= 0;
	Fmt_name[SFMT_DREXEL] = "DREXEL";
	
	GetSoundingFunc[SFMT_WMO]		= wmo_get_sounding;
	SoundingCountFunc[SFMT_WMO]	= wmo_snd_count;
	Fmt_name[SFMT_WMO] = "WMO";

	Init = TRUE;
}




void
snd_read_file(cmds)
struct ui_command	*cmds;
/*
 * Handle the FILE command to get a new sounding
 */
{
	snd_load_file(UPTR(cmds[0]), UKEY(cmds[1]), UKEY(cmds[2]));
}




void
snd_load_file(fname, type, id_name)
char	*fname, *id_name;
int	type;
/*
 * Load a sounding, given the filename, type, and id name
 */
{
	struct snd	*sounding = Snd_list, *prev = 0;
	int	sCount;
	int	s;
	char	sname[64];
	int	multiSoundingSupport = (SoundingCountFunc[type] != 0);
/*
 * Initialize if necessary
 */
	if (! Init)
		snd_init();
/*
 * Get a count of soundings in the file (if supported), otherwise
 * assume there's exactly one sounding.
 */
	sCount = (multiSoundingSupport) ? (*SoundingCountFunc[type])(fname) : 1;
/*
 * Loop through the soundings in this file
 */
	for (s = 0; s < sCount; s++)
	{
	/*
	 * Copy id_name to our sounding name
	 */
		strncpy(sname, id_name, sizeof(sname));
	/*
	 * Generate a suffix for this sounding name if there's more than one
	 * sounding in the file.
	 */
		char suffix[8];
		if (sCount > 1)
			sprintf(sname + strlen(sname), "_%d", s + 1);
	/*
	 * Find the end of the list, looking for name collisions on the way
	 */
		while (sounding)
		{
			if (! strcmp(sounding->name, sname))
				ui_error("Sounding name '%s' is already used", 
					sname);
			prev = sounding;
			sounding = sounding->next;
		}
	/*
	 * Get a new sounding structure
	 */
		sounding = (struct snd *) calloc(1, sizeof(struct snd));
	/*
	 * Read the file using the appropriate routine and put the data 
	 * into our structure
	 */
		if (multiSoundingSupport)
			(*GetSoundingFunc[type])(fname, s, sounding);
		else
			(*GetSoundingFunc[type])(fname, sounding);
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
			malloc((1 + strlen(sname)) * sizeof(char));
		strcpy(sounding->name, sname);
	
		sounding->filename = (char *) 
			malloc((1 + strlen(fname)) * sizeof(char));
		strcpy(sounding->filename, fname);
	
		sounding->format = type;
	}
/*
 * Set the default sounding
 */
	snd_set_default(sname);
/*
 * Done
 */
	return;
}




void
snd_create(cmds)
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
	id_name = UPTR(cmds[0]);
	p_name = UPTR(cmds[1]);
/*
 * Find the parent sounding
 */
	while (parent)
	{
		if (! strcmp(parent->name, p_name))
			break;
		parent = parent->next;
	}

	if (! parent)
		ui_error("Sounding '%s' does not exist", p_name);
/*
 * Find the end of the list, looking for name collisions on the way
 */
	while (sounding)
	{
		if (! strcmp(sounding->name, id_name))
			ui_error("Sounding name '%s' is already used", 
				id_name);
		prev = sounding;
		sounding = sounding->next;
	}
/*
 * Get a new sounding structure
 */
	sounding = (struct snd *) calloc(1, sizeof(struct snd));
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
		malloc((1 + strlen(id_name)) * sizeof(char));
	strcpy(sounding->name, id_name);

	sounding->filename = (char *) 
		malloc((1 + strlen(parent->filename)) * sizeof(char));
	strcpy(sounding->filename, parent->filename);

	sounding->site = (char *) 
		malloc((1 + strlen(parent->site)) * sizeof(char));
	strcpy(sounding->site, parent->site);
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
				calloc(1, sizeof(struct snd_datum));

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
snd_has_field(id_name, fld)
char	*id_name;
fldtype	fld;
/*
 * Return TRUE if the named sounding has data for the chosen field,
 * otherwise return FALSE
 */
{
	struct snd	sounding;
	int	fpos;

	sounding = snd_find_sounding(id_name);
/*
 * Try to find the field in the list
 */
	for (fpos = 0; sounding.fields[fpos] != f_null; fpos++)
		if (sounding.fields[fpos] == fld)
			return(TRUE);

	return(FALSE);
}




int
snd_get_data(id_name, buf, buflen, fld, badval)
char	*id_name;
float	*buf, badval;
int	buflen;
fldtype fld;
/*
 * Stuff data from the chosen sounding and field into 'buf'
 */
{
	int	npts;

	npts = snd_derive_data(id_name, buf, buflen, fld, badval, 0, 0);

	if (! npts)
		ui_error("Unable to obtain or derive '%s' in sounding '%s'", 
			fd_name(fld), id_name);
	else
		return(npts);
}




int
snd_derive_data(id_name, buf, buflen, fld, badval, chain, chainlen)
char	*id_name;
float	*buf, badval;
int	buflen, chainlen;
fldtype fld, *chain;
/*
 * Attempt to get (or derive) data for the chosen field.
 * ON ENTRY:
 *	id_name		the name of the sounding to be used
 *	buf		the buffer to hold the data values
 *	buflen		the length of buf
 *	fld		the field to get
 *	badval		value to insert for non-existent or bad data
 *	chain		list of fields currently being derived
 *	chainlen	the length of chain
 * ON EXIT:
 *	The return value is the number of data points written 
 *	into buf (success) or zero (couldn't derive the field)
 */
{
	int		i, d, fndx, ic, gotfld;
	struct snd	sounding;
	struct snd_datum	*datum;
	float		s_alt;
	fldtype		*dfld, *dchain;
	int		fpos, npts = 0, ndflds, dchainlen, pt;
	float		*dbufs[5];	/* Up to 5 fields in a derivation */
	void		(* dfunc)();

	sounding = snd_find_sounding(id_name);
/*
 * Find the position of this field
 */
	for (fpos = 0; sounding.fields[fpos] != f_null; fpos++)
		if (sounding.fields[fpos] == fld)
			break;

	if (sounding.fields[fpos] != f_null)
	{
	/*
	 * If we made it here, the field already exists in this sounding.
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
			else
				ui_error("*BUG* Buffer too small in snd_derive_data");

			datum = datum->next;
		}
	/*
	 * Return the number of points
	 */
		return(npts);
	}
	else
	{
	/*
	 * We don't have a raw field, see if we can derive it
	 */
		for (i = 0; fdd_derive(fld, i, &dfld, &ndflds, &dfunc); i++)
		{
		/*
		 * Allocate space to hold the fields for the derivation
		 */
			for (fndx = 0; fndx < ndflds; fndx++)
				dbufs[fndx] = (float *) 
					malloc(buflen * sizeof(float));
		/*
		 * Build a new derivation chain
		 */
			dchain = (fldtype *) 
				malloc((chainlen + 1) * sizeof(fldtype));

			memcpy(dchain, chain, chainlen * sizeof(fldtype));

			dchain[chainlen] = fld;
			dchainlen = chainlen + 1;
		/*
		 * Try to get all of the fields for this derivation
		 */
			gotfld = 0;

			for (fndx = 0; fndx < ndflds; fndx++)
			{
			/*	
			 * Make sure this field isn't in the chain already
			 */
				for (ic = 0; ic < chainlen; ic++)
					if (chain[ic] == dfld[fndx])
						break;
			/*
			 * Get the data for this field
			 */
				npts = snd_derive_data(id_name, 
					dbufs[fndx], buflen, dfld[fndx], 
					badval, dchain, dchainlen);
				if (npts > 0)
					gotfld++;
				else
					break;
			}
		/*
		 * If we got data for all of the fields, perform the 
		 * derivation, free allocated space, and return the data.  
		 */
			if (gotfld == ndflds)
			{
				(* dfunc)(buf, dbufs, npts, badval);

				for (fndx = 0; fndx < ndflds; fndx++)
					free(dbufs[fndx]);
				free(dchain);
			/*
			 * KLUGE: altitude derivation returns AGL data,
			 * and we need MSL, so convert altitudes now
			 */
				if (fld == f_alt)
				{
					s_alt = sounding.sitealt;

					for (pt = 0; pt < npts; pt++)
						if (buf[pt] != badval)
							buf[pt] += s_alt;
				}

				return(npts);
			}
		/*
		 * We didn't get all needed fields, free allocated space
		 * and go to the next possible derivation.
		 */
			else
			{
				for (fndx = 0; fndx < ndflds; fndx++)
					free(dbufs[fndx]);
				free(dchain);
			}
		}
	}
/*
 * If we get here, the field isn't in the sounding and can't be derived
 */
	return(0);
}




date
snd_time(id_name)
char	*id_name;
/*
 * Return the start time of the sounding
 */
{
	struct snd	sounding;

	sounding = snd_find_sounding(id_name);
	return(sounding.rls_time);
}



char *
snd_site(id_name)
char	*id_name;
/*
 * Return the site name for this sounding
 */
{
	struct snd	sounding;

	sounding = snd_find_sounding(id_name);
	return(sounding.site);
}




float
snd_s_alt(id_name)
char	*id_name;
/*
 * Return the site altitude
 */
{
	struct snd	sounding;

	sounding = snd_find_sounding(id_name);
	return(sounding.sitealt);
}



float
snd_s_lat(id_name)
char	*id_name;
/*
 * Return the site latitude
 */
{
	struct snd	sounding;

	sounding = snd_find_sounding(id_name);
	return(sounding.sitelat);
}



float
snd_s_lon(id_name)
char	*id_name;
/*
 * Return the site longitude
 */
{
	struct snd	sounding;

	sounding = snd_find_sounding(id_name);
	return(sounding.sitelon);
}



void
snd_forget(cmds)
struct ui_command	*cmds;
/*
 * Forget about the given sounding
 */
{
	int	i;
	struct snd	*prev = 0, *sounding = Snd_list;
	char	*id_name;
	void	edit_forget();

	id_name = UPTR(cmds[0]);
/*
 * Find the sounding in the list
 */
	while (sounding)
	{
		if (! strcmp(id_name, sounding->name))
			break;
		prev = sounding;
		sounding = sounding->next;
	}

	if (! sounding)
		ui_error("There is no sounding with name '%s'", id_name);
/*
 * Release allocated memory belonging to the struct snd
 */
	freeSoundingContents(sounding);
/*
 * Tell edit to forget if necessary
 */
	edit_forget(id_name);
/*
 * Remove it from the sounding list
 */
	if (prev)
		prev->next = sounding->next;
	else
		Snd_list = sounding->next;
/*
 * Finally, free the struct snd itself
 */
	free(sounding);
/*
 * Done
 */
	return;
}




struct snd
snd_find_sounding(id_name)
char	*id_name;
/*
 * Return the sounding associated with id_name
 */
{
	struct snd	*sounding = Snd_list;

	while (sounding)
	{
		if (! strcmp(id_name, sounding->name))
			break;
		sounding = sounding->next;
	}

	if (! sounding)
		ui_error("There is no sounding with name '%s'", id_name);

	return(*sounding);
}




char *
snd_data_ptr(id_name, fld)
char	*id_name;
fldtype	fld;
/*
 * Return a pointer to the data list for the chosen field
 */
{
	struct snd	sounding;
	int	fpos;

	sounding = snd_find_sounding(id_name);
/*
 * Find the position of this field
 */
	for (fpos = 0; sounding.fields[fpos] != f_null; fpos++)
		if (sounding.fields[fpos] == fld)
			break;

	if (sounding.fields[fpos] == f_null)
		ui_error("No sounding data for field '%s'", fd_name(fld));

	return((char *) sounding.dlists[fpos]);
}




void
snd_show(cmds)
struct ui_command	*cmds;
/*
 * List the soundings currently available
 */
{
	int	i, brief, nlist;
	struct snd	*sounding = Snd_list;
	char	string[30];

	brief = (cmds[0].uc_ctype == UTT_KW) && (UKEY(cmds[0]) == KW_BRIEF);
/*
 * Print a heading
 */
	if (! sounding)
	{
		ui_nf_printf("\n    No soundings have been loaded\n");
		return;
	}
	else
		ui_nf_printf("\n            Sounding List\n");
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
				ui_nf_printf(", ");

			if ((nlist % 8) == 0)
				ui_nf_printf("\n");
		/*
		 * ID
		 */
			ui_nf_printf("%s", sounding->name);
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
			ui_nf_printf("\nID: %s\n", sounding->name);
		/*
		 * Format
		 */
			ui_nf_printf("Format: %s  ", 
				Fmt_name[sounding->format]);
		/*
		 * Site name
		 */
			ui_nf_printf("Site: %s  ", sounding->site);
		/*
		 * Time
		 */
			ud_format_date(string, &sounding->rls_time, UDF_FULL);
			ui_nf_printf("Time: %s\n", string);
		/*
		 * Site location
		 */
			ui_nf_printf("Alt: %d m  ", (int)sounding->sitealt);
			ui_nf_printf("Lat: %.4f deg.  ", sounding->sitelat);
			ui_nf_printf("Lon: %.4f deg.\n", sounding->sitelon);
		/*
		 * Fields
		 */
			for (i = 0; sounding->fields[i] != f_null; i++)
			{
				if (i == 0)
					ui_nf_printf("Fields: ");
				else
				{
					ui_nf_printf(", ");
					if ((i % 10) == 0)
						ui_nf_printf("\n          ");
				}
				ui_nf_printf("%s", 
					fd_name(sounding->fields[i]));
			}

			ui_nf_printf("\n");
		}
	/*
	 * Next sounding
	 */
		sounding = sounding->next;
	}
	ui_printf("\n");
	return;
}




snd_write_file(cmds)
struct ui_command	*cmds;
/*
 * Write a sounding into a file
 */
{
	char	*id_name, *snd_default();
	char	*fname = UPTR(cmds[0]);
	struct snd	sounding;
	void	cls_write_file();
/*
 * Get the sounding name then the sounding
 */
	if (cmds[1].uc_ctype != UTT_END)
		id_name = UPTR(cmds[1]);
	else
		id_name = snd_default();

	sounding = snd_find_sounding(id_name);
/*
 * Write the CLASS format sounding
 */
	cls_write_file(fname, &sounding);
}
	



void
snd_set_default(id_name)
char	*id_name;
/*
 * Set the current default sounding
 */
{
	strcpy(Def_snd, id_name);
	return;
}



char *
snd_default()
/*
 * Return the id of the current default sounding
 */
{
	return(Def_snd);
}



void
snd_head(id_name, fld, ptr)
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
		if (! strcmp(id_name, sounding->name))
			break;
		sounding = sounding->next;
	}

	if (! sounding)
		ui_error("There is no sounding with name '%s'", id_name);
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
		ui_error("*BUG* (snd_set_head) -- field '%s' not found",
			fd_name(fld));
/*
 * Change the head
 */
	sounding->dlists[i] = ptr;
}




void
snd_bump_indices(id_name, newindex)
char	*id_name;
int	newindex;
/*
 * For all data associated with the given sounding, increment all indices
 * which are greater than or equal to 'newindex'.  This is to open up an
 * index for a point to be inserted.
 */
{
	struct snd		*prev, *sounding = Snd_list;
	struct snd_datum	*dat;
	int			fpos;

/*
 * Find the sounding in the list
 */
	while (sounding)
	{
		if (! strcmp(id_name, sounding->name))
			break;
		prev = sounding;
		sounding = sounding->next;
	}

	if (! sounding)
		ui_error("There is no sounding with name '%s'", id_name);
/*
 * Increment the indices and maxndx
 */
	for (fpos = 0; sounding->fields[fpos] != f_null; fpos++)
		for (dat = sounding->dlists[fpos]; dat; dat = dat->next)
			if (dat->index >= newindex)
				dat->index++;

	sounding->maxndx++;
}



struct snd *
snd_get_new(id_name)
char    *id_name;
/*
 * Create a new sounding and add it to the list
 */
{
        int     i;
        struct snd      *sounding = Snd_list, *prev = 0;
/*
 * Find the end of the list, looking for name collisions on the way
 */
        while (sounding)
        {
                if (! strcmp(sounding->name, id_name))
                        ui_error("Sounding name '%s' is already used",
                                id_name);
                prev = sounding;
                sounding = sounding->next;
        }
/*
 * Get a new sounding structure
 */
        sounding = (struct snd *) calloc(1, sizeof(struct snd));
/*
 * Link it to the end of the list or make it the head of the list
 */
        if (prev)
                prev->next = sounding;
        else     
                Snd_list = sounding;

	return(sounding);
}
