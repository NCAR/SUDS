/*
 * UI keywords for SUDS
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
 * $Id: keywords.h,v 1.24 1993-04-28 16:21:23 carson Exp $
 */

# define KW_SUGAR	0
# define KW_FILE	1
# define KW_OUTPUT	2
# define KW_SKEWT	3
# define KW_PLIMITS	4
# define KW_TLIMITS	5
# define KW_COPYRIGHT	6
# define KW_TEST	7
# define KW_UP		8
# define KW_DOWN	9
# define KW_CREATE	10
# define KW_EXIT	11
# define KW_SELECT	12
# define KW_MARK	13
# define KW_CUT		14
# define KW_ANALYZE	15
# define KW_SHOW	16
# define KW_WRITE	17
# define KW_ERASE	18
# define KW_NEWVALUE	19
# define KW_EXAMINE	20
# define KW_THRESHOLD	21
# define KW_FORGET	22
# define KW_WSCALE	23
# define KW_FOOTE	24
# define KW_BRIEF	25
# define KW_HODOGRAPH	26
# define KW_COLOR	27
# define KW_INSERT	28
# define KW_ABOVE	29
# define KW_BELOW	30
# define KW_ORIGIN	31
# define KW_LIMITS	32
# define KW_XSECT	33
# define KW_USE		34
# define KW_FROM	35
# define KW_VSCALE	36
# define KW_TIMHGT	37
# define KW_STEP	38
# define KW_TOP		39
# define KW_EXTEND	40
# define KW_CENTER	41
# define KW_NETCDF	42
# define KW_ADD_ALT	43
# define KW_FORECAST	44
# define KW_MLVW	45
# define KW_XYPLOT	46
# define KW_CONLIMITS	47
# define KW_TO		48
# define KW_BARBRES     49
# define KW_MIXDEPTH	50
# define KW_INTERPOLATE 51
# define KW_ILIMITS     52
# define KW_WINDSTEP    53
 
/*
 * Change MAXKW each time a new keyword is added
 */
# define MAXKW	53

/*
 * Threshold criteria keywords
 */
# define THR_BAD	1
# define THR_EQ		2
# define THR_LT		3
# define THR_GT		4
# define THR_LE		5
# define THR_GE		6
# define THR_NE		7
# define THR_DECREASE   8
# define THR_INCREASE   9

/*
 * Sounding format keywords
 */
# define SFMT_CLASS	0
# define SFMT_JAWS	1
# define SFMT_NOAA	2
# define SFMT_NWS	3
# define SFMT_FGGE	4
# define SFMT_RSANAL	5
# define SFMT_EFMT	6
# define SFMT_NCAR	7
# define SFMT_MIST	8
# define SFMT_GALE	9
# define SFMT_NMC	10
# define SFMT_NETCDF	11
# define SFMT_CAPE	12
# define SFMT_NSSL	13
# define SFMT_DREXEL    14
# define SFMT_WMO   15
# define SFMT_EOL 16

# define MAXFMT		16  /* Increase MAXFMT each time a format is added */

/*
 * Things we can show
 */
# define SHOW_SOUNDINGS	0
# define SHOW_FLAGS	1
# define SHOW_COLORS	2
# define SHOW_ORIGIN	3
# define SHOW_LIMITS	4
# define SHOW_FORECAST	5
