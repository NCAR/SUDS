/*
 * Routines for handling boolean flags
 * (stolen from ROBOT's flag.c)
 *
 * $Log: not supported by cvs2svn $
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
	char	*flg_desc;	/* Description of this flag	*/
	char	flg_default;	/* Default value for this flag	*/
} Flg_tbl[] =
{
	/* name		value		description			def  */
	{ "winds",	&Flg_winds,	"Put winds on skew-t plots",	TRUE },
	{ "mli",	&Flg_mli,	"Use modified lifted index",	TRUE },
	{ "lift",	&Flg_lift,	"Show lifted parcel on skew-t",	TRUE },
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
