/*
 * Sounding structure definitions
 *
 * $Log: not supported by cvs2svn $
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
