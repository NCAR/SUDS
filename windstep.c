
/*
 * Handle selection of wind barb plot interval
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

static char *rcsid = "$Id: windstep.c,v 1.1 1993-04-28 16:23:32 carson Exp $";

# include <stdio.h>
# include "globals.h"
# include "keywords.h"


void
windstep (cmds)
struct ui_command	*cmds;
{

			Mark_inc = (int)(UFLOAT (cmds[0]));
			if ( Mark_inc <= 0 ) Mark_inc = 0;
                        return;

/*

        if (cmds->uc_ctype == UTT_KW)
        {
 
            switch (UKEY (cmds[0]))
            {
                case KW_BARBRES:
                        Wb_res = UFLOAT (cmds[1]);
                        return;

                case KW_STEP:
			Mark_inc = (int)(UFLOAT (cmds[0]));
			if ( Mark_inc <= 0 ) Mark_inc = 0;
                        return;

                case KW_WSCALE:
                        W_scale = UFLOAT (cmds[1]);
 
                        if (W_scale <= 0.0 )
                        ui_error ("The wind scale factor must be >0");
                        return;

            }
        }

*/

}
