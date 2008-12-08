/*
 * WMO format sounding access
 */
/*
 *      Copyright (C) 2008 by UCAR
 *  University Corporation for Atmospheric Research
 *         All rights reserved
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

# include <stdio.h>
# include "sounding.h"

/*
 * A struct to hold everything unpacked from the last file we read
 */
static struct _unpackedFile {
	char filename[256];
	int nSoundings;
	struct snd** soundings;
} unpackedFile = { "", 0, 0 };

/*
 * Forwards
 */
void wmo_unpackFile(const char* filename);


int
wmo_snd_count(const char* fname)
/*
 * Return the count of separate soundings in the given WMO file.
 */
{
	if (strcmp(fname, unpackedFile.filename))
		wmo_unpackFile(fname);
	return unpackedFile.nSoundings;
}

void
wmo_get_sounding(const char* fname, int ndx, struct snd* sounding)
/*
 * Get the selected WMO sounding from the given file
 */
{
}

void
wmo_unpackFile(const char* filename)
/*
 * Unpack the WMO file, saving the results into the static struct "unpackedFile"
 */
{
	int s;
	
	for (s = 0; s < unpackedFile.nSoundings; s++)
	{
		freeSoundingContents(unpackedFile.soundings[s]);
		free(unpackedFile.soundings[s]);
		unpackedFile.soundings[s] = 0;
	}
	unpackedFile.nSoundings = 0;
}