/*
 * Fields information
 *
 * $Revision: 1.6 $ $Date: 1990-06-14 10:28:02 $ $Author: burghart $
 */
typedef enum FLDTYPE		fldtype;

enum FLDTYPE
{
/* 0 */		f_null, f_time, f_lat, f_lon, f_alt,
/* 5 */		f_pres, f_temp, f_dp, f_rh, f_wspd,
/* 10 */	f_wdir, f_u_wind, f_v_wind, f_theta, f_theta_e,
/* 15 */	f_mr, f_qpres, f_qtemp, f_qrh, f_qu,
/* 20 */	f_qv, f_qwind, f_rtype, f_range, f_azimuth,
/* 25 */	f_u_prime, f_v_prime, f_dz
};
# define TOTAL_FLDS 28

/*
 * forward declarations
 */
# ifdef VMS
	fldtype	fd_num (char *fname);
	char	*fd_name (fldtype fld);
	char	*fd_desc (fldtype fld);
	char	*fd_units (fldtype fld);
	float	fd_center (fldtype fld), fd_step (fldtype fld);
# else
	fldtype fd_num ();
	char	*fd_name ();
	char	*fd_desc ();
	char	*fd_units ();
	float	fd_center (), fd_step ();
# endif
