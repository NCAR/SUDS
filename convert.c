/*
 * lat,lon <-> x,y conversion utilities
 *
 * $Revision: 1.1 $ $Date: 1989-11-08 11:19:25 $ $Author: burghart $
 */
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
	if (Origin_lat < -90.0 || Origin_lon < -90.0)
	{
		ui_printf ("Enter the origin to use for conversions\n");
		Origin_lat = ui_float_prompt ("   Latitude (decimal degrees)",
			0, -90.0, 90.0, 0.0);
		Origin_lon = ui_float_prompt ("   Longitude (decimal degrees)",
			0, -180.0, 180.0, 0.0);
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
	if (Origin_lat < -90.0 || Origin_lon < -90.0)
	{
		ui_printf ("Enter the origin to use for conversions\n");
		Origin_lat = ui_float_prompt ("   Latitude (decimal degrees)",
			0, -90.0, 90.0, 0.0);
		Origin_lon = ui_float_prompt ("   Longitude (decimal degrees)",
			0, -180.0, 180.0, 0.0);
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




cvt_origin (cmds)
struct ui_command	*cmds;
/*
 * Use lat,lon (deg) as the reference location for 
 * latitude,longitude <-> x,y conversions
 */
{
	float	lat, lon;

	Origin_lat = UFLOAT (cmds[0]);
	Origin_lon = UFLOAT (cmds[1]);
/*
 * Store the values in radians
 */
	Origin_lat *= PI / 180.0;
	Origin_lon *= PI / 180.0;
}
