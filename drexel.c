/*
 *  GALE Data Center (Drexel) format sounding access (where original data 
 *  has been
 *  split into separate sounding files)
 */
/*
 *              Copyright (C) 1988-91 by UCAR
 *      University Corporation for Atmospheric Research
 *                 All rights reserved
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

static char *rcsid = "$Id: drexel.c,v 1.1 1993-01-15 18:05:25 case Exp $";

#include <stdio.h>
#include <ui_param.h>
#include "sounding.h"

/* 
 * 7 fields
 */

# define NFLD    7

# define STRSIZE 46 

# define HDRSIZE 316  /* includes header and comments for now */


/* 
 * Bad value flags for each field
 */

float Bad[NFLD];

/*
 *  Forward declarations
 */



void drexel_read_file (fname, sounding)
char *fname;
struct snd   *sounding;
/*
 * Get the Drexel sounding from the given file
 */

{
     FILE     *sfile;
     char     *status;
     int      i, ndx, npts;
     struct   snd_datum     *dptr[NFLD];
     float    val[NFLD], lat, lon, alt;
     char     site[4], string[STRSIZE], header[HDRSIZE + 1], datestring[7],
              timestring[5], latit[7], longit[8], elev[7], e_time[8],
              press[7], height[8], ctemp[6], rhumid[6], windir[6], winspeed[6];
     void     drexel_insert_data ();
     void     drexel_extract_data ();

/*
 * Open the file
 */

     if ((sfile = fopen (fname, "r")) == 0)
          ui_error ("Cannot open Drexel file '%s'", fname);

/*
 * Sounding header
 */

     fgets (header, HDRSIZE + 1, sfile);

/*
 * Get the site number
 */

     strncpy (site, header+10, 3);
     site[3] = '\0';
     sounding->site = (char *) malloc (4 * sizeof (char));
     strcpy (sounding->site, site);

/*
 * Get site latitude, longitude and elevation
 */
     drexel_extract_data (header+13, latit, &lat, 6);
     sounding->sitelat = lat/100;
     drexel_extract_data (header+19, longit, &lon, 7);
     lon *= -1;
     sounding->sitelon = lon/100;
     drexel_extract_data (header+26, elev, &alt, 6);
     sounding->sitealt = alt;

/*
 *   Get sounding date and time
 */

     strncpy (datestring, header+32, 6);
     datestring[6] = '\0';
     sscanf (datestring, "%d", &(sounding->rls_time.ds_yymmdd));

     strncpy (timestring, header+38, 4);
     timestring[4] = '\0';
     sscanf (timestring, "%d", &(sounding->rls_time.ds_hhmmss));
     sounding->rls_time.ds_hhmmss *= 100;

/*
 *   Find out how many points we have
 */

     sscanf (header + 46, "%2d", &npts);
 

/*
 * Initialize the data pointers
 */
     for (i = 0; i < NFLD; i++)
           dptr[i] = 0;
/*
 * Assign the fields (hard-wired)
 */

     sounding->fields[0] = f_time;  Bad[0] = 99999.9;
     sounding->fields[1] = f_pres;  Bad[1] = 9999.9;
     sounding->fields[2] = f_alt;   Bad[2] = 99999.9;
     sounding->fields[3] = f_temp;  Bad[3] = 999.9;
     sounding->fields[4] = f_rh;    Bad[4] = 999.9;
     sounding->fields[5] = f_wdir;  Bad[5] = 999.9;
     sounding->fields[6] = f_wspd;  Bad[6] = 999.9;
     sounding->fields[7] = f_null;

/*
 *  Get the data
 */

     for (ndx = 0; ndx < npts; ndx++)
     {
     /*
      *  Read the next line
      */

         status = fgets (string, STRSIZE, sfile);
  
         if (status == NULL)
         {
               ndx--;
               break;
         }

     /*
      * Read data values
      */
         drexel_extract_data (string, e_time, &(val[0]), 7);

         drexel_extract_data (string+7, press, &(val[1]), 6);

         drexel_extract_data (string+13, height, &(val[2]), 7);

         drexel_extract_data (string+20, ctemp, &(val[3]), 5);

         drexel_extract_data (string+25, rhumid, &(val[4]), 5);

         drexel_extract_data (string+30, windir, &(val[5]), 5);

         drexel_extract_data (string+35, winspeed, &(val[6]), 5);

    /*
     * Insert the data values
     */

         drexel_insert_data (sounding, ndx, val, dptr);
     }

    /*
     * Set the max index number for the sounding
     */

         sounding->maxndx = ndx - 1;

    /*
     *  Close file and return
     */
 
         fclose (sfile);
         return;
}


void
drexel_extract_data (in_string, out_str, data_value, len_str)
char *in_string;
char out_str[];
float *data_value;
int len_str;

/*
 * store the len_str characters from in_string into out_string and
 * convert them to a float data_value
 */
{
     int num_chars;

     strncpy (out_str, in_string, len_str);
     out_str[len_str] = '\0';
     num_chars = sscanf (out_str, "%f", data_value);
     if (num_chars == 0)
        ui_error ("Error reading drexel_data values");
}

void
drexel_insert_data (sounding, ndx, val, dptr)
struct snd   *sounding;
int ndx;
float *val;
struct snd_datum      *dptr[MAXFLDS];

/*
 * insert the data from array val into the sounding
 */
{
     int i;
     struct snd_datum     *prevpt;
     for (i = 0; i < NFLD; i++)
     {
     /*
      * Don't put bad values in the list
      */
         if (val[i] >= Bad[i])
           continue;

     /*
      * Get a new point
      */
         prevpt = dptr[i];
         dptr[i] = (struct snd_datum *) 
                   calloc (1, sizeof (struct snd_datum));

     /*
      * Set the value and the index
      */
         dptr[i]->value = val[i];
         dptr[i]->index = ndx;

     /*
      * Link the point or make it the head of the list
      */
         if (prevpt)
         {
              prevpt->next = dptr[i];
              dptr[i]->prev = prevpt;
         }
         else
              sounding->dlists[i] = dptr[i];
         }
}
  




 

