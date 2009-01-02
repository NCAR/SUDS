/*
 * Sounding structure definitions
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
 *
 * $Id: sounding.h,v 1.3 1991-10-21 22:13:26 burghart Exp $
 */
# ifndef _SOUNDING_H_
# define _SOUNDING_H_

# include <stdlib.h>

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

# endif // _SOUNDING_H_