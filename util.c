/*
 * Utility routines
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

static char *rcsid = "$Id: util.c,v 1.2 1991-10-21 21:58:26 burghart Exp $";

# ifdef VMS
#	include <stdlib.h>
# endif
# include <ctype.h>

void
strcpyUC (dest, src)
char	*dest, *src;
/*
 * Copy src to dest, converting to upper case
 */
{
	while (*src)
		*dest++ = islower (*src) ? *src++ - 'a' + 'A' : *src++;

	*dest = 0;
}
