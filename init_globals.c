/*
 * Initialize the global data
 *
 * $Revision: 1.3 $ $Date: 1989-08-02 14:39:57 $ $Author: burghart $
 */
# ifdef VMS
#	define var	globaldef
# else
#	define var	/* nothing */
# endif

# include "globals.h"
# include "color.h"
# include "flags.h"
# include "sounding.h"

init_globals ()
{
	stbl	symtbl = usy_g_stbl ("ui$variable_table");

	Wkstn = 0;
	Interrupt = FALSE;
	W_scale = 25.0;
/*
 * Set the flags and initialize the skew-t package
 */
	flg_def ();
	skt_init ();
/*
 * Set the default sounding and make it a UI variable
 */
	Def_snd[0] = (char) 0;
	usy_c_indirect (symtbl, "def_snd", Def_snd, SYMT_STRING, 40);
/*
 * Make the output device and type UI variables
 */
	usy_c_indirect (symtbl, "out_dev", Out_dev, SYMT_STRING, 40);
	usy_c_indirect (symtbl, "dev_type", Dev_type, SYMT_STRING, 40);
/*
 * Initialize the color arrays
 */
	color_init ();
}
