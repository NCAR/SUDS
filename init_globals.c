/*
 * Initialize the global data
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.1  89/03/16  15:14:28  burghart
 * Initial revision
 * 
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
	Red[C_BLACK] = 0.0;
	Red[C_WHITE] = 1.0;
	Red[C_LGRAY] = 0.6;
	Red[C_DGRAY] = 0.45;
	Red[C_RED] = 1.0;
	Red[C_GREEN] = 0.0;
	Red[C_BLUE] = 0.0;
	Red[C_MAGENTA] = 1.0;
	Red[C_YELLOW] = 1.0;
	Red[C_CYAN] = 0.0;
	Red[C_WET] = 0.3;
	Red[C_DRY] = 0.6;

	Green[C_BLACK] = 0.0;
	Green[C_WHITE] = 1.0;
	Green[C_LGRAY] = 0.6;
	Green[C_DGRAY] = 0.45;
	Green[C_RED] = 0.0;
	Green[C_GREEN] = 1.0;
	Green[C_BLUE] = 0.0;
	Green[C_MAGENTA] = 0.0;
	Green[C_YELLOW] = 1.0;
	Green[C_CYAN] = 1.0;
	Green[C_WET] = 0.3;
	Green[C_DRY] = 0.4;

	Blue[C_BLACK] = 0.0;
	Blue[C_WHITE] = 1.0;
	Blue[C_LGRAY] = 0.6;
	Blue[C_DGRAY] = 0.45;
	Blue[C_RED] = 0.0;
	Blue[C_GREEN] = 0.0;
	Blue[C_BLUE] = 1.0;
	Blue[C_MAGENTA] = 1.0;
	Blue[C_YELLOW] = 0.0;
	Blue[C_CYAN] = 1.0;
	Blue[C_WET] = 0.6;
	Blue[C_DRY] = 0.2;
}
