/*
 * Fields information
 *
 * $Revision: 1.2 $ $Date: 1989-11-08 10:52:17 $ $Author: burghart $
 */
typedef enum FLDTYPE		fldtype;

enum FLDTYPE
{
/* 0 */		f_null,	f_time,	f_lat,	f_lon,	f_alt,
/* 5 */		f_pres,	f_temp,	f_dp,	f_rh,	f_wspd,
/* 10 */	f_wdir,	f_u_wind,	f_v_wind,	f_pt,	f_ept,
/* 15 */	f_mr,	f_qpres,	f_qtemp,	f_qrh,	f_qwind,
/* 20 */	f_rtype
};
# define TOTAL_FLDS 21

/*
 * forward declarations
 */
# ifdef VMS
	fldtype	fd_num (char *fname);
	char	*fd_name (fldtype fld);
	float	fd_center (fldtype fld), fd_step (fldtype fld);
# else
	fldtype fd_num ();
	char	*fd_name ();
	float	fd_center (), fd_step ();
# endif
