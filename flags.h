/*
 * $Log: not supported by cvs2svn $
 * Revision 1.1  89/03/16  15:14:03  burghart
 * Initial revision
 * 
 */
# ifndef var
#	ifdef VMS
#		define var globalref
#	else
#		define var extern
#	endif
# endif

var char	Flg_mli;	/* Modified lifted index	*/
var char	Flg_winds;	/* Winds in skew-t plot?	*/
var char	Flg_lift;	/* Show the lifted parcel?	*/
var char	Flg_theta_w;	/* Theta_w on skew-t plots?	*/
