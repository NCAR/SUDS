/*
 * Initialize the global data
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

static char *rcsid = "$Id: init_globals.c,v 1.6 1993-04-28 16:20:15 carson Exp $";

# ifdef VMS
#	define var	globaldef
# else
#	define var	/* nothing */
# endif

# include "globals.h"
# include "color.h"
# include "flags.h"
# include "sounding.h"

init_globals ()
{
	stbl	symtbl = usy_g_stbl ("ui$variable_table");

	Wkstn = 0;
	Interrupt = FALSE;
	W_scale = 25.0;
	Mark_inc = 1;
	Wb_res = 10.0;
	Forecast_pres = 700.0;
/*
 * Set the flags and initialize the skew-t package
 */
	flg_def ();
	skt_init ();
/*
 * Set the default sounding and make it a UI variable
 */
	Def_snd[0] = (char) 0;
	usy_c_indirect (symtbl, "def_snd", Def_snd, SYMT_STRING, 40);
/*
 * Make the output device and type UI variables
 */
	usy_c_indirect (symtbl, "out_dev", Out_dev, SYMT_STRING, 40);
	usy_c_indirect (symtbl, "dev_type", Dev_type, SYMT_STRING, 40);
/*
 * Initialize the color arrays
 */
	color_init ();
}
