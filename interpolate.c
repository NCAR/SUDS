/*
 * Interpolation module.  Interpolate a sounding to a uniform grid of alt or pres
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

static char *rcsid = "$Id: interpolate.c,v 1.1 1993-04-28 16:21:23 carson Exp $";

# include <math.h>
# include <stdlib.h>

# include <ui_param.h>
# include "globals.h"
# include "sounding.h"
# include "keywords.h"
# include "flags.h"

/*
 * Forward declarations
 */
struct snd	snd_find_sounding ();
struct snd *	snd_get_new ();
void	snd_load_file (), snd_set_default ();
double  fd_ibot();
double  fd_itop();
double  fd_istep();
double  fd_igap();
int     interp();
void    minmax();

fldtype Xfld, Yfld; 

FILE *fd;
void
interp_snd (cmds)
struct ui_command	*cmds;
/*
 * Create a new sounding by interpolating another one
 */
{
	int	i,temp;
	char	*p_name, *id_name, *snd_default();
	struct snd	*sounding , parent ;
	struct snd_datum	*pdatum, *sdatum, *prevpt;
	fldtype	flds[5];
	struct ui_command *cmds0;

	double interp_bot, interp_top, interp_step, interp_gap;
	float xmin, xmax;
	int nout, npts, ierror, j;
	float 	xpts[BUFLEN], ypts[BUFLEN], xout[BUFLEN], yout[BUFLEN];


/*
 * Get the dependent variable field for this interpolation
 * Check that it is a valid choice for the dependent variable
 */
	Xfld = fd_num( UPTR (*cmds++));
	if ( !( Xfld == f_time || Xfld == f_pres || Xfld == f_alt ) )
		ui_error(" Invalid field selection for interpolation ");

/*
 * Get the name of the new sounding and the parent sounding
 */
	id_name = UPTR (*cmds++);
        if (cmds->uc_ctype != UTT_END)
                p_name = UPTR (*cmds++);
        else
                p_name = snd_default ();
  
/*
 * get the parent sounding
 */
	parent = snd_find_sounding(p_name);
/*
 * Get a new sounding 
 */
	sounding = snd_get_new (id_name);
/*
 * Copy the sounding id, filename, and site
 */
	sounding->name = (char *) 
		malloc ((1 + strlen (id_name)) * sizeof (char));
	strcpy (sounding->name, id_name);

	sounding->filename = (char *) 
		malloc ((1 + strlen (parent.filename)) * sizeof (char));
	strcpy (sounding->filename, parent.filename);

	sounding->site = (char *) 
		malloc ((1 + strlen (parent.site)) * sizeof (char));
	strcpy (sounding->site, parent.site);
/*
 * Copy the site altitude, format, and size
 */
	sounding->sitealt = parent.sitealt;
	sounding->format = parent.format;
	sounding->sitelat = parent.sitelat;
	sounding->sitelon = parent.sitelon;
	sounding->rls_time = parent.rls_time;
/*
 * Copy the field list
 */
	for (i = 0; i < MAXFLDS; i++)
		sounding->fields[i] = parent.fields[i];
/*
 * Interpolate the data
 */
/* 
 * Use the selected field for the dependent variable
 */
	interp_bot = fd_ibot( Xfld );
	interp_top = fd_itop( Xfld );
	interp_step= fd_istep( Xfld );
	interp_gap = fd_igap( Xfld );
	
	npts = snd_get_data(p_name, xpts, BUFLEN, Xfld, BADVAL);
	if ( npts < 2 ) 
		ui_error("No data for dependent field! %d", npts);

	/*
	 * If limits are to be selected from the data
	 * - use input step size
	 * - select top and bottom limits from data
	 */
	if ( Flg_datalim != 0 )
	{
		minmax( xpts, npts, &xmin, &xmax);
		interp_step = fabs( interp_step);
		interp_bot = ( (int)(xmin/interp_step) ) * interp_step;
		interp_top = ( (int)(xmax/interp_step) ) * interp_step;
		if ( interp_bot > xmin ) interp_bot -= interp_step;
		if ( interp_top < xmax ) interp_top += interp_step;
		if ( Xfld == f_pres )
		{
			temp = interp_bot;
			interp_bot = interp_top;
			interp_top = temp;
			interp_step = -interp_step;
		}
		/*
		 * Set the new top and bottom values for this field     
		 */
		cmds0 = (struct ui_command *) 
			malloc( 4*sizeof(struct ui_command));
		cmds0[0].uc_ctype = UTT_VALUE;
		cmds0[0].uc_vptype = SYMT_FLOAT ;
		cmds0[1].uc_ctype = UTT_VALUE;
		cmds0[1].uc_vptype = SYMT_FLOAT ;
		cmds0[2].uc_ctype = UTT_VALUE;
		cmds0[2].uc_vptype = SYMT_FLOAT ;
		cmds0[3].uc_ctype = UTT_VALUE;
		cmds0[3].uc_vptype = SYMT_FLOAT ;
		cmds0[4].uc_ctype = UTT_VALUE;
		cmds0[4].uc_vptype = SYMT_FLOAT ;
		cmds0[0].uc_v.us_v_ptr = fd_name( Xfld );
		cmds0[1].uc_v.us_v_float = interp_bot;
		cmds0[2].uc_v.us_v_float = interp_top;
		cmds0[3].uc_v.us_v_float = interp_step;
		cmds0[4].uc_v.us_v_float = interp_gap;
		fd_set_ilimits ( cmds0 );
		free( (void *) cmds0);
		ui_nf_printf("\nInterpolation limits selected: \n");
		ui_nf_printf("  Bottom Value = %f \n",interp_bot);
		ui_nf_printf("  Top    Value = %f \n",interp_top);
		ui_nf_printf("  Step   Value = %f \n",interp_step);
		ui_nf_printf("  Gap    Value = %f \n",interp_gap);
	}

	nout = (int) ( (interp_top-interp_bot)/interp_step) +1;
	nout = ( nout>0 ? nout : -nout );
	sounding->maxndx = nout - 1;

/* 
 * Create a list of the dependent variable for output
 */
	for ( i=0; i<nout; i++ )
	 	xout[i] = interp_bot + i*interp_step;

/*
 * Loop through each field and interpolate for the new sounding
 */

	for (i = 0; sounding->fields[i] != f_null; i++)
	{

 		Yfld = sounding->fields[i];

		if ( Xfld != Yfld ) 
		{

		/* 
		 * Get the data for this field
		 */
			temp =snd_get_data(p_name, ypts, BUFLEN, Yfld, BADVAL);

		/* 
		 * Interpolate to the output sounding data fields
		 */
		
			ierror = interp( xpts, ypts, xout, yout, npts, nout,
					 interp_gap );

			if ( ierror ) 
				ui_error(" Error during interpolation ",ierror);
		}
		else
		{
		/*
		 * Copy the dependent variable to its output field
		 */
			for ( j=0; j<= nout; j++)
				yout[j] = xout[j];
		}
		/* 
		 * Copy the interpolated results into the new sounding
		 */
		prevpt = (struct snd_datum *) 0;

		for ( j=0; j <= nout; j++)
		{

			sdatum = (struct snd_datum *) 
				  calloc(1, sizeof (struct snd_datum));
			sdatum->index = j;
			sdatum->value = yout[j];
			if (prevpt)
			{
				sdatum->prev = prevpt;
				prevpt->next = sdatum;
			}
			else
				sounding->dlists[i] = sdatum;

			prevpt = sdatum;
		}
	}
	snd_set_default(id_name);

/*
 * Done
 */
 fclose(fd);
	return;
}

int 
interp( xdata, ydata, xout, yout, npts, nout, gap )
float *xdata, *ydata, *xout, *yout;
double gap;
int npts, nout;
{

	int i, j, dir, ngood, k, Na, Nb, Nlsq, nsum;
	float xtemp[BUFLEN], ytemp[BUFLEN], fgap, yj, yjm1;
	double acoef, bcoef, sumx, sumxx, sumxy, sumy;

/* 
 * Copy good values to local array for interpolation
 */
	Nlsq = 5;
	j = 0;
	fgap = (float) gap;
	for ( i=0; i < npts; i++)
	{
		if ( xdata[i] != BADVAL && ydata[i] != BADVAL )
		{
			ytemp[j] = ydata[i];
			xtemp[j++] = xdata[i];
		}
	}
	ngood = j;
/*
 * Check to see if the x data is increasing or decreasing
 */

	if ( xtemp[1] > xtemp[0] )
		dir=0;
	else
		dir=1;

	for ( i=0; i < nout; i++)
	{
		j = 0;
		if ( dir )
			while ( xout[i] < xtemp[j] && j < ngood )
				j++;
		else
			while ( xout[i] > xtemp[j]  && j < ngood)
				j++;

		if ( j == 0 ) j++;
		if ( j >= ngood-1 ) j=ngood-1;

		/*
		 * If a least squares fit is selected
		 */
		if ( Flg_lsqfit )
		{

			/*
			 * Select points evenly on either side of nearest data
			 */
			sumx = 0.0;
			sumxx= 0.0;
			sumxy= 0.0;
			sumy = 0.0;
			nsum = 0;
			Nb = Nlsq / 2 + 1;
			Na = Nlsq / 2;
			for ( k=0; k<Nb; k++)
				if ( (j-k) >= 0 &&
				fabs((double)(xtemp[j-k]-xout[i])) <= fgap)
				{
					/* Use this point if it's not too
					 * far away from the desired point */
					sumx += xtemp[j-k];
					sumy += ytemp[j-k];
					sumxy += xtemp[j-k]*ytemp[j-k];
					sumxx += xtemp[j-k]*xtemp[j-k];
					nsum++;
				}
				else
					/* Look for data on the other side 
					 * of the desired point */
					Na++;

                        for ( k=0; k<Na; k++) 
                                if ( (j+k) >= 0 &&
                                fabs((double)(xtemp[j+k]-xout[i])) <= fgap)
                                { 
                                        /* Use this point if it's not too
                                         * far away from the desired point */
                                        sumx += xtemp[j+k]; 
                                        sumy += ytemp[j+k];
                                        sumxy += xtemp[j+k]*ytemp[j+k];
                                        sumxx += xtemp[j+k]*xtemp[j+k];
					nsum++;
                                } 
                                else
				{
                                        /* Look for data on the other side 
                                         * of the desired point */
					if ( j-Nb >= 0 &&
                                	   fabs((double)(xtemp[j+Nb]-xout[i])) 
					   <= fgap)
					{
                                              sumx += xtemp[j+Nb];  
                                       	      sumy += ytemp[j+Nb];
                                              sumxy += xtemp[j+Nb]*ytemp[j+Nb];
                                              sumxx += xtemp[j+Nb]*xtemp[j+Nb];
					      nsum++;
					      Nb++;
					}
				}

			acoef = (sumxx*sumy - sumx*sumxy)/(nsum*sumxx- sumx*sumx);
			bcoef = (nsum*sumxy - sumx*sumy)/(nsum*sumxx- sumx*sumx);
			yout[i] = acoef + bcoef * xout[i];

		}
		/*
		 * Else use a linear polynomial fit
		 */
		else 
		{

			if (  fabs((double)(xtemp[j]-xtemp[j-1])) >= fgap ) 
				yout[i] = BADVAL;
			else
				if ( j == ngood-1 &&
				   fabs((double)(xtemp[j]-xout[i])) >= fgap )
				 	yout[i] = BADVAL;
				else
				{
				   yj = ytemp[j];
				   yjm1 = ytemp[j-1];
				   if ( Yfld == f_wdir &&
				   fabs((double)(yj-yjm1)) > 180.0 )
				   {
				      if ( yj > yjm1 )
				 	yjm1 += 360.0;
				      else
					yj += 360.0;
				   }
				   yout[i] = 
				      ( yj-yjm1) * (xout[i]-xtemp[j-1])
               		            / ( xtemp[j]-xtemp[j-1] ) + yjm1;
				}
		}

		if ( Yfld == f_pres && yout[i] <=0.0 && yout[i] != BADVAL)
		     ui_warning("negative pressure calculated - Xfld= %f\r\n", 
				    xout[i]);
		if ( Yfld == f_wdir ) 
			yout[i] = fmod( (double)yout[i],(double)360.0); 
	}


	return(0);
}

void
minmax( xpts, npts, xmin, xmax)
float *xpts, *xmin, *xmax;
int npts;
{
int i;

*xmin = 1.0e10;
*xmax = -1.0e10;

for ( i=0; i<npts; i++)
{
	if ( xpts[i] > *xmax && xpts[i] != BADVAL ) *xmax = xpts[i];
	if ( xpts[i] < *xmin && xpts[i] != BADVAL ) *xmin = xpts[i];
}
return;
}
