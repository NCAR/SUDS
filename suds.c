/*
 * SUDS main driver
 *
 * $Revision: 1.4 $ $Date: 1989-08-02 14:41:27 $ $Author: burghart $
 */
# ifdef VMS
#	include <ssdef.h>
# endif
# include <ui_date.h>
# include <ui_error.h>
# include "globals.h"
# include "keywords.h"

/*
 * Pointers to routines to handle each command (stored by keyword number)
 */
static void	(*Cmd_routine[MAXKW+1])();

# ifdef VMS
	/*
	 * Exit handler control block description.
	 */
	static struct exhblk
	{
		struct exhblk *eb_flink;	/* Forward link */
		int (*eb_handler) ();		/* Address of exit handler */
		int eb_argc;			/* Arg count */
		int *eb_cond;			/* Where to put condition */
	} Exh;
	static int Condition;
# endif



main (argc, argv)
int	argc;
char	**argv;
{
	void	main_finish (), main_interrupt (), main_cmd_init();
	int	main_dispatch ();
	int	status, read_init = TRUE;
	stbl	tag;
	union usy_value	helpdir;

# ifdef VMS
/*
 * Set up the exit handler.
 */
	Exh.eb_handler = main_finish;
	Exh.eb_argc = 0;
	Exh.eb_cond = &Condition;
	status = sys$dclexh (&Exh);
	if (status != SS$_NORMAL)
		c_panic ("$Dclexh status: %d", status);
# endif

/*
 * Lose the first command line argument (the name of the program) which
 * is there whether we want it or not
 */
	argv++;
	argc--;
/*
 * Use an error catch until we get to ui_get_command (which has its own)
 */
	ERRORCATCH
	/*
	 * Initialize
	 */
# ifdef VMS
		ui_init ("ds:[burghart.suds]suds.lf", ! argc, TRUE);
# else
		ui_init ("/localbin/suds.lf", ! argc, TRUE);
# endif
		init_globals ();
		main_cmd_init ();
	/*
	 * The keyboard interrupt handler
	 */
		uii_set_handler (main_interrupt, FALSE);
	/*
	 * Establish the help directory
	 */
# ifdef VMS
		helpdir.us_v_ptr = "ds:[burghart.suds.help]";
# else
		helpdir.us_v_ptr = "/home/burghart/suds/help/";
# endif
		usy_s_symbol (usy_g_stbl ("ui$variable_table"),
			"ui$helpdir", SYMT_STRING, &helpdir);
	/*
	 * Do we want the init file?
	 */
		if (argc && ! strcmp (argv[0], "-n"))
		{
			read_init = FALSE;
			argc--;
			argv++;
		}
	/*
	 * Set up the input files specified on the command line
	 */
		while (argc)
		{
			ut_open_file (argv[0], TRUE);
			argv++;
			argc--;
		}
	/*
 	 * Read the init file unless we were told not to
	 */
		if (read_init)
		{
			char	initfile[80], *getenv ();

			strcpy (initfile, getenv ("HOME"));
# ifndef VMS
			strcat (initfile, "/");
# endif
			strcat (initfile, "suds.ini");
			ut_open_file (initfile, FALSE);
		}
	ON_ERROR
		main_finish ();
	ENDCATCH
/*
 * GO!  Read commands until there aren't any more
 */
	ui_get_command ("suds-initial", "->", main_dispatch, 0);
/*
 * We're done, the exit handler declared above will execute 
 * main_finish now
 */
# ifdef VMS
	/* exit handler calls main_finish for us */
# else
	main_finish ();
# endif
}



void
main_cmd_init ()
/*
 * Initialize the array of command handling routines
 */
{
	void	snd_read_file (), out_output (), skt_handler ();
	void	main_copyright (), snd_create (), edit_mark ();
	void	edit_select (), edit_up_pointer (), edit_down_pointer ();
	void	edit_cut (), analyze (), main_show (), snd_write_file ();
	void	edit_erase (), edit_newvalue (), edit_examine ();
	void	edit_threshold (), snd_forget (), skt_wscale ();
	void	ft_plot (), hd_plot (), color_change ();

	Cmd_routine[KW_FILE]		= snd_read_file;
	Cmd_routine[KW_OUTPUT]		= out_output;
	Cmd_routine[KW_SKEWT]		= skt_handler;
	Cmd_routine[KW_COPYRIGHT]	= main_copyright;
	Cmd_routine[KW_CREATE]		= snd_create;
	Cmd_routine[KW_SELECT]		= edit_select;
	Cmd_routine[KW_UP]		= edit_up_pointer;
	Cmd_routine[KW_DOWN]		= edit_down_pointer;
	Cmd_routine[KW_MARK]		= edit_mark;
	Cmd_routine[KW_CUT]		= edit_cut;
	Cmd_routine[KW_ANALYZE]		= analyze;
	Cmd_routine[KW_SHOW]		= main_show;
	Cmd_routine[KW_WRITE]		= snd_write_file;
	Cmd_routine[KW_ERASE]		= edit_erase;
	Cmd_routine[KW_NEWVALUE]	= edit_newvalue;
	Cmd_routine[KW_EXAMINE]		= edit_examine;
	Cmd_routine[KW_THRESHOLD]	= edit_threshold;
	Cmd_routine[KW_FORGET]		= snd_forget;
	Cmd_routine[KW_WSCALE]		= skt_wscale;
	Cmd_routine[KW_FOOTE]		= ft_plot;
	Cmd_routine[KW_HODOGRAPH]	= hd_plot;
	Cmd_routine[KW_COLOR]		= color_change;
}



int
main_dispatch (dummy, cmds)
int	dummy;
struct ui_command	*cmds;
/*
 * The command dispatcher
 */
{
	int	kwnum = UKEY (cmds[0]);
/*
 * Execute the command
 */
	if (Cmd_routine[kwnum])
	{
	/*
	 * We have a routine for this command; execute it, passing remaining
	 * commands (if any) as a parameter
	 */
		(*Cmd_routine[kwnum]) (cmds + 1);
	/*
	 * Check for unhandled interrupts
	 */
		if (Interrupt)
		{
			ui_printf ("Interrupt ignored\n");
			Interrupt = FALSE;
		}
	/*
	 * Done
	 */
		return (TRUE);
	}
	else
	{
	/*
	 * No routine, so dump the command
	 */
		for (;; cmds++)
		{
			main_dump_cmd (cmds);
			if (cmds->uc_ctype == UTT_END)
				return (TRUE);
		}
	}
}



main_dump_cmd (cmd)
struct ui_command *cmd;
/*
 * Dump a command structure
 */
{
	char	date_string[40];

	switch (cmd->uc_ctype)
	{
	   case UTT_END:
	   	ui_printf ("End of token list\n");
		return;
	   case UTT_VALUE:
	     switch (cmd->uc_vptype)
	     {
		case SYMT_FLOAT:
			ui_printf ("Value (float): %.4f\n", UFLOAT (cmd[0]));
			return;
		case SYMT_INT:
			ui_printf ("Value (int): %d\n", UINT (cmd[0]));
			return;
		case SYMT_STRING:
		   	ui_printf ("Value (string): '%s'\n", UPTR (cmd[0]));
			return;
		case SYMT_DATE:
			ud_format_date (date_string, &(UDATE (cmd[0])), 
				UDF_FULL);
			ui_printf ("Value (date): %s\n", date_string);
			return;
		case SYMT_BOOL:
			ui_printf ("Value (boolean): %s\n", 
				UBOOL (cmd[0]) ? "TRUE" : "FALSE");
			return;
		case SYMT_SYMBOL:
			ui_printf ("Value (symbol table)\n");
			return;
		case SYMT_POINTER:
			ui_printf ("Value (general pointer)\n");
			return;
		case SYMT_UNDEFINED:
			ui_printf ("Value (undefined symbol)\n");
			return;
		default:
			return;
	     }
	   case UTT_OTHER:
	   	ui_printf ("Type UTT_OTHER...\n");
		return;
	   case UTT_KW:
	   	ui_printf ("Keyword number %d\n", UKEY (cmd[0]));
		return;
	   default:
	   	ui_printf ("Something REALLY weird -- type = %d\n", 
			cmd->uc_ctype);
	}
}



void
main_finish ()
/*
 * Clean up and exit
 */
{
	ui_finish ();

	if (Wkstn)
		G_close (Wkstn);
# ifndef VMS	/* Avoid "Message number 00000000" printed on the VAX */
	exit (0);
# endif
}



void
main_interrupt ()
/*
 * Set the interrupt flag
 */
{
	Interrupt = TRUE;
}



void
main_copyright ()
/*
 * The copyright message.  Just a ui_printf of a very long string.
 */
{
ui_printf (
"		Copyright (C) 1988 by UCAR\n\
	University Corporation for Atmospheric Research\n\
		   All rights reserved\n\
\n\
No part of this work covered by the copyrights herein may be reproduced\n\
or used in any form or by any means -- graphic, electronic, or mechanical,\n\
including photocopying, recording, taping, or information storage and\n\
retrieval systems -- without permission of the copyright owner.\n\
\n\
This software and any accompanying written materials are provided \"as is\"\n\
without warranty of any kind.  UCAR expressly disclaims all warranties of\n\
any kind, either express or implied, including but not limited to the\n\
implied warranties of merchantibility and fitness for a particular purpose.\n\
UCAR does not indemnify any infringement of copyright, patent, or trademark\n\
through use or modification of this software.  UCAR does not provide \n\
maintenance or updates for its software.\n"
);
}




void
main_show (cmds)
struct ui_command	*cmds;
/*
 * Dispatch the show command to the proper place
 */
{
/*
 * Command switch table
 */
    switch (UKEY (cmds[0]))
    {
	case SHOW_SOUNDINGS:
		snd_show (cmds + 1);
		return;
	case SHOW_FLAGS:
		flg_list ();
		return;
	default:
		for (;; cmds++)
		{
			main_dump_cmd (cmds);
			if (cmds->uc_ctype == UTT_END)
				return;
		}
    }
}
