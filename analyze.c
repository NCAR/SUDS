/*
 * Sounding analysis module
 *
 * $Revision: 1.19 $ $Date: 1991-01-16 21:38:07 $ $Author: burghart $ 
 */
# include <math.h>
# include <stdio.h>
# include <ui_error.h>
# include <ui_param.h>
# include <ui_date.h>
# include "globals.h"
# include "fields.h"
# include "flags.h"
# include "met_formulas.h"

# define R_D	287.

# define DEG_TO_RAD	0.017453293

/*
 * Pressure to use for lifted index calculations
 */
# define LI_PRES	(Flg_mli ? 400 : 500)

/*
 * Forward declarations
 */
float	an_lfc_calc (), an_area (), an_li (), an_li_ref (), an_shear ();
void	an_surface (), an_printf (), an_do_analysis ();

/*
 * Sounding id
 */
char	*Id_name;
/*
 * Output file info
 */
static char	Outfile_name[80] = "";
static FILE	*Outfile = (FILE *) 0;
static int	Write_to_file = FALSE;


analyze (cmds)
struct ui_command	*cmds;
/*
 * Perform an analysis of a sounding
 */
{
	float	t[BUFLEN], vt[BUFLEN], p[BUFLEN], dp[BUFLEN];
	float	u[BUFLEN], v[BUFLEN];
	float	t_sfc, vt_sfc, p_sfc, dp_sfc, t700, vt700, dp700, ref;
	int	i, npts, name_loc = 0, success, ndx700;
	char	*snd_default (), *snd_site (), string[80];
	date	sdate, snd_time ();
/*
 * See if we're writing to a file
 */
	if (cmds[0].uc_ctype == UTT_KW)
	{
	/*
	 * Set the output flag and make sure we look for the sounding name
	 * in the right place
	 */
		name_loc = 2;
		Write_to_file = TRUE;
	/*
	 * Open a new file if necessary
	 */
		if (strcmp (Outfile_name, UPTR (cmds[1])))
		{
			if (Outfile)
				fclose (Outfile);
			strcpy (Outfile_name, UPTR (cmds[1]));
			Outfile = fopen (Outfile_name, "w");
			if (Outfile == NULL)
			{
				Write_to_file = FALSE;
				ui_error ("Unable to open file '%s'", 
					Outfile_name);
			}
		}
	/*
	 * Otherwise, put a form feed between soundings
	 */
		else
			fprintf (Outfile, "\f");
	}
	else
		Write_to_file = FALSE;
/*
 * Get the sounding name (or the default)
 */
	if (cmds[name_loc].uc_ctype != UTT_END)
		Id_name = UPTR (cmds[name_loc]);
	else
		Id_name = snd_default ();
/*
 * Get the necessary sounding data
 */
	npts = snd_get_data (Id_name, t, BUFLEN, f_temp, BADVAL);
	snd_get_data (Id_name, vt, BUFLEN, f_vt, BADVAL);
	snd_get_data (Id_name, p, BUFLEN, f_pres, BADVAL);
	snd_get_data (Id_name, dp, BUFLEN, f_dp, BADVAL);
/*
 * Convert the temperatures and dewpoints to K so they're easier to deal with
 */
	for (i = 0; i < npts; i++)
	{
		if (t[i] != BADVAL)
			t[i] += T_K;
		if (dp[i] != BADVAL)
			dp[i] += T_K;
	}
/*
 * Begin printing info
 */
	sdate = snd_time (Id_name);
	ud_format_date (string, &sdate, UDF_FULL);

	an_printf ("\nAnalysis for sounding '%s'\n", Id_name);
	an_printf ("Time: %s, Site: %s\n", string, snd_site (Id_name));
	an_printf ("-----------------------------------------------\n");
/*
 * Print preliminaries for the surface-based analysis
 */
	an_printf ("SURFACE-BASED ANALYSIS\n");

	an_surface (t, p, dp, npts, &t_sfc, &p_sfc, &dp_sfc);
	an_printf ("\t Surface potential temperature%s: %.1f K\n", 
		Flg_mli ? " (50 mb average)" : "",
		theta_dry (t_sfc, p_sfc));

	an_surface (vt, p, dp, npts, &vt_sfc, &p_sfc, &dp_sfc);
	an_printf ("\t Surface virtual potential temperature%s: %.1f K\n", 
		Flg_mli ? " (50 mb average)" : "",
		theta_dry (vt_sfc, p_sfc));

	an_printf ("\t Surface mixing ratio%s: %.1f g/kg \n", 
		Flg_mli ? " (50 mb average)" : "",
		w_sat (dp_sfc, p_sfc));

	ref = an_li_ref (p, t, npts);
	an_printf ("\t %d mb temperature: %.1f K ", LI_PRES, ref);
	an_printf ("(potential temp.: %.1f K)\n", theta_dry (ref, LI_PRES));

	ref = an_li_ref (p, vt, npts);
	an_printf ("\t %d mb virtual temperature: %.1f K ", LI_PRES, ref);
	an_printf ("(virtual potential temp.: %.1f K)\n", 
		theta_dry (ref, LI_PRES));
/*
 * Do the analysis, using virtual temperature
 */
	an_do_analysis (vt, p, dp, npts, vt_sfc, p_sfc, dp_sfc);
/*
 * Preliminaries for the forecast (700 mb based) analysis
 */
	an_printf ("FORECAST (700 mb-BASED) ANALYSIS\n");

	success = an_700 (t, p, dp, npts, p_sfc, dp_sfc, &t700, &dp700, 
		&ndx700);

	if (success)
	{
		an_printf ("\t 700 mb temperature: %.1f K ", t700);
		an_printf ("(potential temp.: %.1f K)\n", 
			theta_dry (t700, 700.0));
	}

	success = an_700 (vt, p, dp, npts, p_sfc, dp_sfc, &vt700, &dp700, 
		&ndx700);

	if (! success)
	{
		an_printf ("\t Unable to find 700 mb virtual temperature\n");
		return;
	}

	an_printf ("\t 700 mb virtual temperature: %.1f K ", vt700);
	an_printf ("(virtual potential temp.: %.1f K)\n", 
		theta_dry (vt700, 700.0));

	an_printf ("\t Surface mixing ratio%s: %.1f g/kg \n", 
		Flg_mli ? " (50 mb average)" : "",
		w_sat (dp_sfc, p_sfc));

	ref = an_li_ref (p, t, npts);
	an_printf ("\t %d mb temperature: %.1f K ", LI_PRES, ref);
	an_printf ("(potential temp.: %.1f K)\n", theta_dry (ref, LI_PRES));

	ref = an_li_ref (p, vt, npts);
	an_printf ("\t %d mb virtual temperature: %.1f K ", LI_PRES, ref);
	an_printf ("(virtual potential temp.: %.1f K)\n", 
		theta_dry (ref, LI_PRES));
/*
 * Do the analysis, using virtual temperature
 */
	an_do_analysis (vt + ndx700, p + ndx700, dp + ndx700, 
		npts - ndx700, vt700, 700.0, dp700);
}




void
an_do_analysis (t, p, dp, npts, t_sfc, p_sfc, dp_sfc)
float	*t, *p, *dp, t_sfc, p_sfc, dp_sfc;
int	npts;
/*
 * Perform the analysis of the sounding data in arrays t, p, and dp, using
 * t_sfc, p_sfc, and dp_sfc as the surface values.
 */
{
	float	li, area, shear;
	float	p_lcl, t_lcl, theta_lcl, theta_e_lcl, p_lower, t_lower;
	float	p_upper, t_upper, p_lfc, ln_p, dt_lower, dt_upper;
	float	area_pos = 0.0, area_neg = 0.0;
	int	i, stop = 0;
/*
 * Get the pressure, temperature, potential temp. and equivalent
 * potential temp. of the LCL
 */
	p_lcl = lcl_pres (t_sfc, dp_sfc, p_sfc);
	t_lcl = lcl_temp (t_sfc, dp_sfc);
	an_printf ("\t LCL pressure: %.1f mb\n", p_lcl);
	theta_lcl = theta_dry (t_lcl, p_lcl);
	theta_e_lcl = theta_e (t_lcl, t_lcl, p_lcl);
/*
 * Print the lifted index info
 */
	li = an_li (p, t, npts, theta_e_lcl, TRUE);

	if (li != BADVAL)
		an_printf ("\t %s index: %.2f\n", 
			Flg_mli ? "Modified lifted" : "Lifted", li);
/*
 * Integrate the area (energy) from the surface up to the LCL
 */
	p_lower = p_sfc;
	t_lower = t_sfc;
	dt_lower = t_lower - theta_lcl;

	for (i = 1; p_lower > p_lcl; i++)
	{
	/*
	 * Make sure we have a good point
	 */
		if (p[i] == BADVAL || t[i] == BADVAL || p[i] == p_lower)
			continue;

		p_upper = p[i];
		t_upper = t[i];
		dt_upper = t_upper - theta_to_t (theta_lcl, p_upper);
	/*
	 * See if we're changing from positive to negative area or vice-versa.
	 * If there is a change, use the point of change for the top of the
	 * current integration step.
	 */
		if (dt_lower * dt_upper < 0)
		{
			float	tslope = (t_upper - t_lower) / 
				log (p_upper / p_lower);
		/*
		 * Find the pressure and temperature at the intersection
		 */
			ln_p = (dt_lower * log (p_upper) -
				dt_upper * log (p_lower)) / 
				(dt_lower - dt_upper);
			p_upper = exp (ln_p);
			t_upper = tslope * (ln_p - log (p_lower)) + t_lower;
			dt_upper = 0.0;
		/*
		 * We aren't using the ith point this time, but we still
		 * want it next time, so decrement i
		 */
			i--;
		}
	/*
	 * Stop at the LCL if we're crossing it
	 */
		else if (p_upper < p_lcl)
		{
			p_upper = p_lcl;
			t_upper = t_lower + (t[i] - t_lower) * 
				log (p_lcl / p_lower) / log (p[i] / p_lower);
			dt_upper = t_upper - theta_to_t (theta_lcl, p_lcl);
		/*
		 * We aren't using the ith point this time, but we still
		 * want it next time, so decrement i
		 */
			i--;
		}
	/*
	 * Numerical integration between the last two good points
	 */
		area = an_area (p_upper, p_lower, t_upper, t_lower, 
			theta_lcl, FALSE);

		if (area > 0.0)
			area_pos += area;
		else
			area_neg += area;
	/*
	 * Update p_lower, t_lower, and dt_lower
	 */
		p_lower = p_upper;
		t_lower = t_upper;
		dt_lower = dt_upper;
	}
/*
 * Integrate the area (energy) from the LCL to the LFC
 */
	p_lfc = an_lfc_calc (t, p, dp, npts, t_sfc, p_sfc, dp_sfc, 0.0, TRUE);

	if (p_lfc != BADVAL)
		an_printf ("\t LFC pressure: %.1f mb\n", p_lfc);
	else
	{
		an_printf ("\t No LFC\n");
		return;
	}

	for (; p_lower > p_lfc && i < npts; i++)
	{
	/*
	 * Make sure we have a good point
	 */
		if (p[i] == BADVAL || t[i] == BADVAL)
			continue;

		p_upper = p[i];
		t_upper = t[i];
		dt_upper = t_upper - t_sat (theta_e_lcl, p_upper);
	/*
	 * See if we're changing from positive to negative area or vice-versa.
	 * If there is a change, use the point of change for the top of the
	 * current integration step.
	 */
		if (dt_lower * dt_upper < 0)
		{
			float	tslope = (t_upper - t_lower) / 
				log (p_upper / p_lower);
		/*
		 * Find the pressure and temperature at the intersection
		 */
			ln_p = (dt_lower * log (p_upper) -
				dt_upper * log (p_lower)) / 
				(dt_lower - dt_upper);
			p_upper = exp (ln_p);
			t_upper = tslope * (ln_p - log (p_lower)) + t_lower;
			dt_upper = 0.0;
		/*
		 * We aren't using the ith point this time, but we still
		 * want it next time, so decrement i
		 */
			i--;
		}
	/*
	 * Stop at the LFC if we're crossing it
	 */
		else if (p_upper < p_lfc)
		{
			p_upper = p_lfc;
			t_upper = t_lower + (t[i] - t_lower) * 
				log (p_lfc / p_lower) / log (p[i] / p_lower);
			dt_upper = t_upper - t_sat (theta_e_lcl, p_lfc);
		/*
		 * We aren't using the ith point this time, but we still
		 * want it next time, so decrement i
		 */
			i--;
		}
	/*
	 * Numerical integration between the last two good points
	 */
		area = an_area (p_upper, p_lower, t_upper, t_lower, 
			theta_e_lcl, TRUE);

		if (area > 0.0)
			area_pos += area;
		else
			area_neg += area;
	/*
	 * Update p_lower and t_lower
	 */
		p_lower = p_upper;
		t_lower = t_upper;
		dt_lower = dt_upper;
	}
/*
 * Print the total areas from the surface to the LFC
 */
	an_printf ("\t Positive area below the LFC: %.0f J/kg\n", 
		area_pos * R_D);
	an_printf ("\t Negative area below the LFC: %.0f J/kg\n", 
		area_neg * R_D);
/*
 * Now integrate up to the top of the positive area or 300 mb, whichever
 * occurs higher.
 */
	area_pos = 0.0;
	area_neg = 0.0;

	for (; i < npts && !stop; i++)
	{
	/*
	 * Make sure we have a good point
	 */
		if (p[i] == BADVAL || t[i] == BADVAL)
			continue;

		if (p[i] >= p_lower)
		{
			ui_warning (
			"Static or increasing pressure ignored at %.1f mb",
				p[i]);
			continue;
		}
	/*
	 * Use this point for the upper limit of the next integration
	 */
		p_upper = p[i];
		t_upper = t[i];
	/*
	 * See if we're changing from positive to negative area or vice-versa
	 * (i.e., does the temperature cross the moist adiabat?)
	 * If there is a change, use the point of change for the top of the
	 * current integration step.
	 */
		dt_upper = t_upper - t_sat (theta_e_lcl, p_upper);

		if (dt_lower * dt_upper < 0)
		{
			float	tslope = (t_upper - t_lower) / 
				log (p_upper / p_lower);
		/*
		 * Find the pressure and temperature at the intersection
		 */
			ln_p = (dt_lower * log (p_upper) -
				dt_upper * log (p_lower)) / 
				(dt_lower - dt_upper);
			p_upper = exp (ln_p);
			t_upper = tslope * (ln_p - log (p_lower)) + t_lower;
			dt_upper = 0.0;
		/*
		 * Stop if we cross into negative area above 300 mb
		 */
			stop = p_upper <= 300.0;
		/*
		 * We aren't using the ith point this time, but we still
		 * want it next time, so decrement i
		 */
			i--;
		}
	/*
	 * If we crossed into negative area before we reached 300 mb, just
	 * stop at 300 mb.
	 */
		stop = stop || (dt_upper > 0.0 && p_upper <= 300.0);
	/*
	 * Integrate between (t_lower,p_lower) and (t_upper,p_upper)
	 */
		area = an_area (p_upper, p_lower, t_upper, t_lower, 
			theta_e_lcl, TRUE);

		if (area > 0.0)
			area_pos += area;
		else
			area_neg += area;
	/*
	 * Update p_lower and t_lower
	 */
		p_lower = p_upper;
		t_lower = t_upper;
		dt_lower = dt_upper;
	}
	an_printf ("\t CAPE: %.0f J/kg\n", area_pos * R_D);
	an_printf ("\t Negative area above the LFC: %.0f J/kg\n", 
		area_neg * R_D);
/*
 * Bulk Richardson number
 */
	shear = an_shear ();
	an_printf ("\t Shear over lowest 6km: %.1f m/s\n", shear);

	if (shear != 0.0)
		an_printf ("\t Bulk Richardson number: %.1f\n", 
			2.0 * R_D * area_pos / (shear * shear));
	else
		an_printf ("\t Bulk Richardson number: NO SHEAR\n");

	an_printf ("\n");
}




float
an_lfc_calc (t, p, dp, npts, t_sfc, p_sfc, dp_sfc, p_lim)
float	*t, *dp, *p, t_sfc, p_sfc, dp_sfc, p_lim;
int	npts;
/*
 * Calculate the LFC from the data arrays and the surface values given
 * If the LFC doesn't exist or the pressure is less than p_lim, return 
 * the bad value flag
 */
{
	float	p_lcl, t_lcl, theta_e_lcl, dt_bot, dt_top, ln_p_lfc;
	float	p_prev, t_prev;
	int	i = 0;
/*
 * Get LCL info
 */
	p_lcl = lcl_pres (t_sfc, dp_sfc, p_sfc);
	t_lcl = lcl_temp (t_sfc, dp_sfc);
	theta_e_lcl = theta_e (t_lcl, t_lcl, p_lcl);
/*
 * Search through the pressure array until we pass the LCL, then go
 * back to the previous good point
 */
	while (p[i] > p_lcl || p[i] == BADVAL)
		if (++i == npts)
			return (BADVAL);

	for (i--; p[i] == BADVAL || t[i] == BADVAL || dp[i] == BADVAL; i--)
		/* nothing */;
/*
 * Go up from here until the temperature crosses the moist adiabat
 */
	p_prev = p[i];
	t_prev = t[i];

	while (t[i] > t_sat (theta_e_lcl, p[i]))
	{
	/*
	 * Return if we pass the pressure limit
	 */
		if (p[i] < p_lim)
			return (BADVAL);
	/*
	 * Move to the next good point
	 */
		p_prev = p[i];
		t_prev = t[i];
		while (p[i] >= p_prev || p[i] == BADVAL || t[i] == BADVAL ||
			dp[i] == BADVAL)
			if (++i == npts)
				return (BADVAL);
	}
/*
 * Check for super-adiabatic case (i.e. we didn't go anywhere because the
 * temperature started to the left of the moist adiabat)
 */
	if (p_prev == p[i])
	{
		an_printf (
			"Super-adiabatic case at %d mb, no LFC calculated\n",
			(int) p_prev);
		return (BADVAL);
	}
/*
 * The LFC is between p_prev and p[i], calculate its pressure and temperature.
 * (Assume that the moist adiabat is straight between these two pressures)
 */
	dt_bot = t_prev - t_sat (theta_e_lcl, p_prev);
	dt_top = t_sat (theta_e_lcl, p[i]) - t[i];

	ln_p_lfc = (dt_bot * log (p[i]) + dt_top * log (p_prev)) / 
		(dt_bot + dt_top);
	return ((float) (exp (ln_p_lfc)));
}




float
an_area (p1, p0, t1, t0, theta, moist)
float	p1, p0, t1, t0, theta;
int	moist;
/*
 * Numerical integration of area between (t0,p0) and (t1,p1), using
 * theta as the potential (or equivalent potential) temperature 
 * of the adiabat.  If 'moist' is true, use the saturated adiabat, 
 * otherwise use the dry.  
 */
{
	float	pres, delta_p, dt, dt_prev, t_adiab, area = 0.0;
	float	d_lnp, temp;
	float	tslope;
/*
 * Sanity check
 */
	if (p1 >= p0)
		ui_error ("Downward or zero depth integration at %.1f mb", p0);
/*
 * Get the slope of the temperature line between the two points
 */
	tslope = (t1 - t0) / log (p1 / p0);
/*
 * Initialize based on the first point
 */
	if (moist)
		t_adiab = t_sat (theta, p0);
	else
		t_adiab = theta_to_t (theta, p0);

	dt_prev = t0 - t_adiab;
/*
 * Integrate using 10 mb steps, assuming the adiabat is straight over
 * the step.
 */
	for (pres = p0 - 10.; pres + 10. > p1; pres -= 10.)
	{
		delta_p = 10.; 
	/*
	 * Check for the top
	 */
		if (pres < p1)
		{
			delta_p = 10. - p1 + pres;
			pres = p1;
		}
	/*
	 * Calculate d(ln p)
	 */
		d_lnp = log (pres / (pres + delta_p));
	/*
	 * Interpolate the temperature to this pressure and
	 * find the temperature of our adiabat here
	 */
		temp = log (pres / p0) * tslope + t0;

		if (moist)
			t_adiab = t_sat (theta, pres);
		else
			t_adiab = theta_to_t (theta, pres);

		dt = temp - t_adiab;
	/*
	 * Add the area of this slice
	 */
		area += 0.5 * d_lnp * (dt_prev + dt);
	/*
	 * Update dt_prev
	 */
		dt_prev = dt;
	}
/*
 * We're done
 */
	return (area);
}



void
an_surface (t, p, dp, npts, t_sfc, p_sfc, dp_sfc)
float	*t, *p, *dp, *t_sfc, *p_sfc, *dp_sfc;
int	npts;
/*
 * Find the surface values.  If the MLI flag is set (i.e., we're using
 * the modified lifted index), the surface dewpoint returned is actually
 * the dewpoint corresponding to the surface pressure and the mean mixing
 * ratio for the lowest 50 mb of the sounding; the surface temperature
 * returned is the temperature corresponding to the surface pressure and
 * the mean potential temperature for the lowest 50 mb of the sounding.
 */
{
	int	i = 0, mr_count = 0, theta_count = 0;
	float	mr_sum = 0.0, mr, theta_sum = 0.0, theta, p_top;
/*
 * Find the lowest point with good values in all three fields; this
 * will be our surface point.
 */
	while (t[i] == BADVAL || p[i] == BADVAL || dp[i] == BADVAL)
		if (++i == npts)
			ui_error ("No surface point for analysis");

	*t_sfc = t[i];
	*p_sfc = p[i];
	*dp_sfc = dp[i];
/*
 * If we're not using the modified lifted index, return now
 */
	if (! Flg_mli)
		return;
/*
 * Using MLI.  Average the mixing ratio and theta over the lowest 50 mb
 */
	p_top = *p_sfc - 50.0;

	for (; (p[i] > p_top || p[i] == BADVAL) && i < npts; i++)
	{
	/*
	 * Don't try to use bad values
	 */
		if (p[i] == BADVAL || dp[i] == BADVAL || t[i] == BADVAL)
			continue;
	/*
	 * Find the mixing ratio and theta and increment our sums
	 */
		if (dp[i] != BADVAL)
		{
			mr_sum += w_sat (dp[i], p[i]);
			mr_count++;
		}
		if (t[i] != BADVAL)
		{
			theta_sum += theta_dry (t[i], p[i]);
			theta_count++;
		}
	}
/*
 * Make sure we spanned 50 mb
 */
	if (i == npts)
		ui_error ("The sounding does not span 50 mb");

/*
 * Find the mean mixing ratio, then get the corresponding dewpoint.
 */
	mr = mr_sum / mr_count;
	*dp_sfc = t_mr (*p_sfc, mr);
/*
 * Find the mean theta and the corresponding surface temperature
 */
	theta = theta_sum / theta_count;
	*t_sfc = theta_to_t (theta, *p_sfc);
/*
 * Done
 */
	return;
}



int
an_700 (t, p, dp, npts, p_sfc, dp_sfc, temp700, dp700, ndx700)
float	*t, *p, *dp, p_sfc, dp_sfc, *temp700, *dp700;
int	npts, *ndx700;
/*
 * Find the 700 mb temperature, the 700 mb dewpoint corresponding to the
 * surface mixing ratio, and the index of the last pressure > 700 mb in 
 * the data arrays.
 */
{
	int	i;
	float	t_prev, p_prev, w;
/*
 * Find the first pressure < 700.0
 */
	for (i = 0; i < npts; i++)
	{
		if (p[i] == BADVAL || t[i] == BADVAL)
			continue;

		if (p[i] < 700.0)
			break;

		p_prev = p[i];
		t_prev = t[i];
	}

	*ndx700 = i - 1;
/*
 * Make sure we have two good points to interpolate between
 */
	if (p_prev == BADVAL || i == npts)
		return (FALSE);
/*
 * Interpolate the 700 mb temperature from the point we just found and the
 * previous good point
 */
	*temp700 = t_prev + (700.0 - p_prev) / (p[i] - p_prev) * 
		(t[i] - t_prev);
/*
 * Find the mixing ratio of our surface dewpoint, then find the
 * corresponding 700 mb dewpoint
 */
	w = w_sat (dp_sfc, p_sfc);
	*dp700 = t_mr (700.0, w);
	return (TRUE);
}




float
an_li (p, t, npts, theta_e_lcl)
float	*p, *t, theta_e_lcl;
int	npts;
/*
 * Find the lifted index (or modified lifted index) for the sounding
 */
{
	float	ref, li;
/*
 * Get the lifted index using the reference temperature, and be verbose
 * if requested
 */
	if ((ref = an_li_ref (p, t, npts)) == BADVAL)
	{
		an_printf ("\t Cannot find %d mb temp. for lifted index\n", 
			LI_PRES);
		return (BADVAL);
	}

	li = ref - t_sat (theta_e_lcl, LI_PRES);
/*
 * Done
 */
	return (li);
}




float
an_li_ref (p, t, npts)
float	*p, *t;
int	npts;
/*
 * Get the sounding temperature at 400 or 500 mb for use in lifted index
 * calculations
 */
{
	int	i;
	float	p_prev = BADVAL, t_prev = BADVAL;
/*
 * Move up until we cross the desired pressure (either 400.0 or 500.0)
 */
	for (i = 0; i < npts; i++)
	{
		if (p[i] == BADVAL || t[i] == BADVAL)
			continue;
	/*
	 * Check if we passed the index level
	 */
		if (p[i] <= LI_PRES)
			break;

		p_prev = p[i];
		t_prev = t[i];
	}
/*
 * Sanity check
 */
	if (p_prev == BADVAL || i == npts)
	{
		return (BADVAL);
	}
/*
 * Interpolate the temperature
 */
	return (t_prev + (t[i] - t_prev) * (LI_PRES - p_prev) /
		(p[i] - p_prev));
}



void
an_printf (fmt, ARGS)
char	*fmt;
int	ARGS;	/* ARGS is defined in ui_param.h */
/*
 * printf-like interface to write to the screen and also to an output
 * file if requested
 */
{
	char	buf[1000];
/*
 * Encode the output (SPRINTARGS is defined in ui_param.h)
 */
	sprintrmt (buf, fmt, SPRINTARGS);
/*
 * ui_printf the buffer
 */
	ui_printf (buf);
/*
 * write to the output file if requested
 */
	if (Write_to_file)
		fprintf (Outfile, buf);
}




float
an_shear ()
/*
 * Find a shear value for use in the Bulk Richardson number calculation
 */
{
	float	wspd[BUFLEN], wdir[BUFLEN], alt[BUFLEN], p[BUFLEN];
	float	p_sum = 0.0, u_sum = 0.0, v_sum = 0.0;
	float	sfc_u, sfc_v, delta_u, delta_v;
	int	sfc_vals = FALSE;
	int	npts, i;
/*
 * Get the wind and altitude data
 */
	ERRORCATCH
		npts = snd_get_data (Id_name, p, BUFLEN, f_pres, BADVAL);
		snd_get_data (Id_name, wspd, BUFLEN, f_wspd, BADVAL);
		snd_get_data (Id_name, wdir, BUFLEN, f_wdir, BADVAL);
		snd_get_data (Id_name, alt, BUFLEN, f_alt, BADVAL);
	ON_ERROR
		return (0.0);
	ENDCATCH
/*
 * Find the mean wind speeds up to 500m and up to 6km
 */
	for (i = 0; i < npts; i++)
	{
	/*
	 * Make sure we have good points
	 */
		if (p[i] == BADVAL || wspd[i] == BADVAL || 
			wdir[i] == BADVAL || alt[i] == BADVAL)
			continue;
	/*
	 * Get the mean surface wind if we passed 500m
	 */
		if (alt[i] > 500.0 && !sfc_vals)
		{
			if (p_sum == 0.0)
			{
				ui_warning ("Unable to calculate shear");
				return (0.0);
			}
			sfc_u = u_sum / p_sum;
			sfc_v = v_sum / p_sum;
			sfc_vals = TRUE;
		}
	/*
	 * Return the shear if we passed 6km
	 */
		if (alt[i] > 6000.0)
		{
			if (p_sum == 0.0)
			{
				ui_warning ("Unable to calculate shear");
				return (0.0);
			}
			delta_u = u_sum / p_sum - sfc_u;
			delta_v = v_sum / p_sum - sfc_v;
			return (hypot (delta_u, delta_v));
		}
	/*
	 * Increment the sums for the pressure and pressure weighted
	 * wind speed
	 */
		u_sum += p[i] * wspd[i] * sin (wdir[i] * DEG_TO_RAD);
		v_sum += p[i] * wspd[i] * cos (wdir[i] * DEG_TO_RAD);
		p_sum += p[i];
	}
/*
 * If we get here, it means the sounding doesn't extend to 6km
 */
	if (p_sum == 0.0)
	{
		ui_warning ("Unable to calculate shear");
		return (0.0);
	}

	ui_warning ("Sounding shallower than 6km; using available data for shear.");
	delta_u = u_sum / p_sum - sfc_u;
	delta_v = v_sum / p_sum - sfc_v;
	return (hypot (delta_u, delta_v));
}

