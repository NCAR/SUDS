/*
 * $Log: not supported by cvs2svn $
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
# else
	fldtype fd_num ();
	char	*fd_name ();
# endif
