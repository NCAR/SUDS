/*
 * Routines for handling boolean flags
 * (stolen from ROBOT's flag.c)
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.2  90/05/04  13:41:51  burghart
 * New flag theta_w
 * 
 * Revision 1.2  90/05/04  13:36:47  burghart
 * function theta_w() changed name to theta_e()
 * 
 * Revision 1.1  89/03/16  15:13:51  burghart
 * Initial revision
 * 
 */
# include "globals.h"
# include "flags.h"

# define ___	0

/*
 * Our data structure for dealing with flags.
 */
struct flgst
{
	char	*flg_name;	/* Name of this flag	*/
	char	*flg_value;	/* Pointer to the flag	*/
	char	flg_default;	/* Default value for this flag	*/
	char	*flg_desc;	/* Description of this flag	*/
} Flg_tbl[] =
{
	/* name		value		default	*/
	/*	description  */
	{ "winds",	&Flg_winds,	TRUE,
		"Put winds on skew-t plots"},
	{ "mli",	&Flg_mli,	TRUE,
		"Use modified lifted index"},
	{ "lift",	&Flg_lift,	TRUE,
		"Show lifted parcel on skew-t"},
	{ "theta_w",	&Flg_theta_w,	TRUE,
		"Use equivalent wet-bulb temperature on skew-t"},
	{ "vt",		&Flg_vt,	TRUE,
		"Use virtual temperature on skew-t"},
	{ ___,		___,		___,		___ }
};




flg_def ()
/*
 * Set all the flags to their default values.
 */
{
	struct flgst *f;
	stbl table = usy_g_stbl ("ui$variable_table");

	for (f = Flg_tbl; f->flg_name; f++)
	{
		*f->flg_value = f->flg_default;
		usy_c_indirect (table, f->flg_name, f->flg_value,
			SYMT_BOOL, 0);
	}
}




flg_list ()
/*
 * List off the value of all the flags.
 */
{
	struct flgst *f;

	ui_nf_printf ("\nFlag settings: \n");
	ui_nf_printf ("          Name      Value        Description\n");
	ui_printf ("        --------    ------       -----------\n");
	for (f = Flg_tbl; f->flg_name; f++)
		ui_printf ("        %8.8s     %5.5s       %s\n", f->flg_name,
				*f->flg_value ? "True" : "False", f->flg_desc);
}
