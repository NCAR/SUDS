/*
 * lat,lon <-> x,y conversion utilities
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

static char *rcsid = "$Id: convert.c,v 1.4 1991-10-21 21:35:39 burghart Exp $";

# include <math.h>
# include <ui.h>

# define PI 3.141592654

/*
 * Radius of the earth, in km
 */
# define R_EARTH	6372.

/*
 * Origin latitude and longitude (radians)
 */
static float	Origin_lat = -99.0, Origin_lon = -99.0;




void
cvt_to_xy (lat, lon, x, y)
float	lat, lon, *x, *y;
/* 
 * Convert lat and lon (deg) to x and y (km) using azimuthal 
 * orthographic projection
 */
{
	float	del_lat, del_lon;
/*
 * Make sure we have an origin
 */
	if (Origin_lat < -2*PI || Origin_lon < -2*PI)
	{
		ui_printf ("Enter the origin to use for conversions\n");
		Origin_lat = ui_float_prompt ("   Latitude (decimal degrees)",
			0, -90.0, 90.0, 0.0) * PI / 180.0;
		Origin_lon = ui_float_prompt ("   Longitude (decimal degrees)",
			0, -180.0, 180.0, 0.0) * PI / 180.0;
	}
/*
 * Convert the lat,lon to x,y
 */
	lat *= PI / 180.0;
	lon *= PI / 180.0;

	del_lat = lat - Origin_lat;
	del_lon = lon - Origin_lon;

	*x = R_EARTH * cos (lat) * sin (del_lon);
	*y = R_EARTH * sin (del_lat);
}




void
cvt_to_latlon (x, y, lat, lon)
float	x, y, *lat, *lon;
/*
 * Convert x and y (km) to lat and lon (deg)
 */
{
	float	del_lat, del_lon;
/*
 * Make sure we have an origin
 */
	if (Origin_lat < -2*PI || Origin_lon < -2*PI)
	{
		ui_printf ("Enter the origin to use for conversions\n");
		Origin_lat = ui_float_prompt ("   Latitude (decimal degrees)",
			0, -90.0, 90.0, 0.0) * PI / 180.0;
		Origin_lon = ui_float_prompt ("   Longitude (decimal degrees)",
			0, -180.0, 180.0, 0.0) * PI / 180.0;
	}
/*
 * Convert the x,y to lat,lon
 */
	del_lat = asin (y / R_EARTH);
	*lat = Origin_lat + del_lat;

	del_lon = asin (x / (R_EARTH * cos (*lat)));
	*lon = Origin_lon + del_lon;
/*
 * Convert to degrees
 */
	*lat *= 180.0 / PI;
	*lon *= 180.0 / PI;
}




void
cvt_get_origin (lat, lon)
float	*lat, *lon;
/*
 * Return the current origin values (in degrees)
 */
{
/*
 * Make sure we have an origin
 */
	if (Origin_lat < -2*PI || Origin_lon < -2*PI)
	{
		ui_printf ("Enter the origin to use for conversions\n");
		Origin_lat = ui_float_prompt ("   Latitude (decimal degrees)",
			0, -90.0, 90.0, 0.0) * PI / 180.0;
		Origin_lon = ui_float_prompt ("   Longitude (decimal degrees)",
			0, -180.0, 180.0, 0.0) * PI / 180.0;
	}
/*
 * Return the current origin in degrees
 */
	*lat = Origin_lat / PI * 180.0;
	*lon = Origin_lon / PI * 180.0;
}




void
cvt_origin (cmds)
struct ui_command	*cmds;
/*
 * Handle the "origin" command
 */
{
	float	lat, lon;
	void	cvt_set_origin ();

	lat = UFLOAT (cmds[0]);
	lon = UFLOAT (cmds[1]);

	cvt_set_origin (lat, lon);
}



void
cvt_set_origin (lat, lon)
float	lat, lon;
/*
 * Use lat,lon (deg) as the reference location for 
 * latitude,longitude <-> x,y conversions
 */
{
/*
 * Test that the values are in range
 */
	if (lat > 90.0 || lat < -90.0)
		ui_error ("Latitude out of range (-90.0 to 90.0)");
	if (lon > 180.0 || lon < -180.0)
		ui_error ("Longitude out of range (-180.0 to 180.0)");
/*
 * Store the values in radians
 */
	Origin_lat = lat * PI / 180.0;
	Origin_lon = lon * PI / 180.0;
}



void
cvt_show_origin ()
/*
 * Print out the current origin
 */
{
	if (Origin_lat < -90.0 || Origin_lon < -90.0)
	{
		ui_printf ("\n    No current origin\n\n");
		return;
	}

	ui_printf ("\n    Current origin latitude: %.4f, longitude: %.4f\n\n",
		Origin_lat * 180.0 / PI, Origin_lon * 180.0 / PI);
	return;
}
