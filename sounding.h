/*
 * Sounding structure definitions
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  89/03/16  15:16:51  burghart
 * Initial revision
 * 
 */
# include <ui_param.h>		/* for the date stuff */
# include "fields.h"

# define MAXFLDS	32

/*
 * The sounding structure
 */
struct snd
{
	char	*name;
	char	*filename;
	int	format;
	date	rls_time;
	char	*site;
	float	sitealt;
	float	sitelat;
	float	sitelon;
	int	maxndx;
	fldtype	fields[MAXFLDS];
	struct snd_datum	*dlists[MAXFLDS];
	struct snd	*next;
};

/*
 * Structure for a sounding datum
 */
struct snd_datum
{
	int	index;
	float	value;
	struct snd_datum	*prev, *next;
};

# ifndef var
#	ifdef VMS
#		define var	globalref
#	else
#		define var	extern
#	endif
# endif

/*
 * The current default sounding
 */
var char	Def_snd[40];
