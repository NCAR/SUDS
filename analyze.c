/*
 * $Log: not supported by cvs2svn $
 */
# include <math.h>
# include "globals.h"
# include "fields.h"
# include "flags.h"
# include "derive.h"

# define MIN(x,y)	((x) < (y)) ? (x) : (y)
# define R_D	287.

/*
 * Forward declarations
 */
float	an_lfc_calc (), an_area (), an_li (), an_fmli ();
void	an_surface ();



analyze (cmds)
struct ui_command	*cmds;
/*
 * Perform an analysis of a sounding
 */
{
	float	t[BUFLEN], p[BUFLEN], dp[BUFLEN];
	float	t_sfc, p_sfc, dp_sfc, li;
	float	p_lcl, t_lcl, theta_lcl, theta_e_lcl, p_lower, t_lower;
	float	p_upper, t_upper, p_lfc, ln_p, dt_lower, dt_upper;
	float	area_below = 0.0, area_pos = 0.0, area_neg = 0.0;
	int	sfc = 0, i, npts, trop = 0;
	char	*snd_default (), *id_name;
/*
 * Get the sounding name (or the default)
 */
	if (cmds[0].uc_ctype != UTT_END)
		id_name = UPTR (cmds[0]);
	else
		id_name = snd_default ();
/*
 * Get the necessary sounding data
 */
	npts = snd_get_data (id_name, t, BUFLEN, f_temp, BADVAL);
	snd_get_data (id_name, p, BUFLEN, f_pres, BADVAL);
	snd_get_data (id_name, dp, BUFLEN, f_dp, BADVAL);
/*
 * Get our surface points
 */
	an_surface (t, p, dp, npts, &t_sfc, &p_sfc, &dp_sfc);

	ui_printf ("\nAnalysis for sounding '%s'\n", id_name);
	ui_printf ("-----------------------------------------------\n");
	ui_printf ("Surface potential temperature: %.1f K\n", 
		theta_dry (t_sfc + T_K, p_sfc));
	ui_printf ("Surface mixing ratio%s: %.1f g/kg \n", 
		Flg_mli ? " (50 mb average)" : "",
		w_sat (dp_sfc + T_K, p_sfc));
/*
 * Get the pressure, temperature, potential temp. and equivalent
 * potential temp. of the LCL
 */
	p_lcl = lcl_pres (t_sfc + T_K, dp_sfc + T_K, p_sfc);
	t_lcl = lcl_temp (t_sfc + T_K, dp_sfc + T_K);
	ui_printf ("LCL pressure: %.1f mb\n", p_lcl);
	theta_lcl = theta_dry (t_lcl, p_lcl);
	theta_e_lcl = theta_w (t_lcl, p_lcl);
/*
 * Integrate the area (energy) from the surface up to the LCL
 */
	p_lower = p_sfc;
	t_lower = t_sfc + T_K;

	for (i = sfc + 1; p_lower > p_lcl; i++)
	{
	/*
	 * Make sure we have a good point
	 */
		if (p[i] == BADVAL || t[i] == BADVAL || p[i] == p_lower)
			continue;

		p_upper = p[i];
		t_upper = t[i] + T_K;
	/*
	 * Stop at the LCL
	 */
		if (p_upper < p_lcl)
		{
			p_upper = p_lcl;
			t_upper = t_lower + (t[i] + T_K - t_lower) * 
				log (p_lcl / p_lower) / log (p[i] / p_lower);
		}
	/*
	 * Numerical integration between the last two good points
	 */
		area_below += an_area (p_upper, p_lower, t_upper, t_lower, 
			theta_lcl, FALSE);
	/*
	 * Update p_lower and t_lower
	 */
		p_lower = p_upper;
		t_lower = t_upper;
	}
/*
 * Print the lifted index info
 */
	li = an_li (p, t, npts, theta_e_lcl, TRUE);

	if (Flg_mli)
	{
		ui_printf ("The modified lifted index is %.2f\n", li);
	/*
	 * Get the forecasted modified lifted index, too
	 */
		li = an_fmli (p, t, npts, p_sfc, dp_sfc);
		ui_printf ("The forecasted modified lifted index is %.2f\n", 
			li);
	}
	else
	{
		ui_printf ("The lifted index is %.2f\n", li);
	}

/*
 * Integrate the area (energy) from the LCL to the LFC
 */
	p_lfc = an_lfc_calc (t, p, dp, npts, t_sfc, p_sfc, dp_sfc, 0.0);
	if (p_lfc != BADVAL)
		ui_printf ("LFC pressure: %.1f mb\n", p_lfc);
	else
		ui_printf ("No LFC\n");

	for (i--; p_lower > p_lfc && i < npts; i++)
	{
	/*
	 * Make sure we have a good point
	 */
		if (p[i] == BADVAL || t[i] == BADVAL)
			continue;

		p_upper = p[i];
		t_upper = t[i] + T_K;
	/*
	 * Stop at the LFC
	 */
		if (p_upper < p_lfc)
		{
			p_upper = p_lfc;
			t_upper = t_lower + (t[i] + T_K - t_lower) * 
				log (p_lfc / p_lower) / log (p[i] / p_lower);
		}
	/*
	 * Numerical integration between the last two good points
	 */
		area_below += an_area (p_upper, p_lower, t_upper, t_lower, 
			theta_e_lcl, TRUE);
	/*
	 * Update p_lower and t_lower
	 */
		p_lower = p_upper;
		t_lower = t_upper;
	}
	ui_printf ("The area below the LFC is %.0f J/kg\n", area_below * R_D);
/*
 * Now integrate up to the tropopause, separating positive and negative area
 */
	dt_lower = t_lower - t_sat (theta_e_lcl, p_lower);

	for (i--; i < npts && !trop; i++)
	{
		float	area;
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
		t_upper = t[i] + T_K;
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
		 * Are we at the tropopause?
		 */
			trop = p_upper <= 300.0;
		/*
		 * We aren't using the ith point this time, but we still
		 * want it next time, so decrement i
		 */
			i--;
		}
	/*
	 * Stop at 300mb if we have crossed into negative area
	 */
		trop = trop || (dt_upper > 0.0 && p_upper <= 300.0);
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
	ui_printf ("The positive area above the LFC is %.0f J/kg\n", 
		area_pos * R_D);
	ui_printf ("The negative area above the LFC is %.0f J/kg\n", 
		area_neg * R_D);
	ui_printf ("\n");
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
	float	p_prev, t_prev, vt, vt_prev, e;
	int	i = 0;
/*
 * Get LCL info
 */
	p_lcl = lcl_pres (t_sfc + T_K, dp_sfc + T_K, p_sfc);
	t_lcl = lcl_temp (t_sfc + T_K, dp_sfc + T_K);
	theta_e_lcl = theta_w (t_lcl, p_lcl);
/*
 * Search through the pressure array until we pass the LCL, then go
 * back to the previous good point
 */
	while (p[i] > p_lcl || p[i] == BADVAL || t[i] == BADVAL)
		if (++i == npts)
			ui_bailout ("Sounding has no points above the LCL");
	for (i--; p[i] == BADVAL || t[i] == BADVAL; i--)
		/* nothing */;
	p_prev = p[i];
	t_prev = t[i];
/*
 * Get the vapor pressure and the virtual temperature
 */
	e = e_from_dp (dp[i] + T_K);
	vt = t_v (t[i] + T_K, p[i], e);
	vt_prev = vt;
/*
 * Go up from here until the virtual temperature crosses the moist adiabat
 */
	while (vt > t_sat (theta_e_lcl, p[i]))
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
		vt_prev = vt;
		while (p[i] >= p_prev || p[i] == BADVAL || t[i] == BADVAL ||
			dp[i] == BADVAL)
			if (++i == npts)
				return (BADVAL);
	/*
	 * Get the vapor pressure and the virtual temperature
	 */
		e = e_from_dp (dp[i] + T_K);
		vt = t_v (t[i] + T_K, p[i], e);
	}
/*
 * Check for super-adiabatic case (i.e. we didn't go anywhere because the
 * temperature started to the left of the moist adiabat)
 */
	if (p_prev == p[i])
	{
		ui_printf (
			"Super-adiabatic case at %d mb, no LFC calculated\n",
			(int) p_sfc);
		return (BADVAL);
	}
/*
 * The LFC is between p_prev and p[i], calculate its pressure and temperature.
 * (Assume that the moist adiabat is straight between these two pressures)
 */
	dt_bot = t_prev + T_K - t_sat (theta_e_lcl, p_prev);
	dt_top = t[i] + T_K - t_sat (theta_e_lcl, p[i]);

	ln_p_lfc = (dt_bot * log (p[i]) - dt_top * log (p_prev)) / 
		(dt_bot - dt_top);
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
 * ratio for the lowest 50 mb of the sounding.
 */
{
	int	i = 0;
	float	sum = 0.0, mr, mr_prev, p_prev, p_top;
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
 * Using MLI.  Average the mixing ratio over the lowest 50 mb
 */
	mr_prev = w_sat (*dp_sfc + T_K, *p_sfc);
	p_prev = *p_sfc;
	p_top = *p_sfc - 50.0;

	for (i++; TRUE; i++)
	{
	/*
	 * Don't try to use non-existent or bad values
	 */
		if (i == npts)
			ui_error ("The sounding does not span 50 mb");

		if (p[i] == BADVAL || dp[i] == BADVAL)
			continue;
	/*
	 * Break out if we passed the 50 mb mark
	 */
		if (p[i] < p_top)
			break;
	/*
	 * Find the mixing ratio and increment our sum
	 */
		mr = w_sat (dp[i] + T_K, p[i]);
		sum += (p_prev - p[i]) * (mr + mr_prev) / 2.0;
	/*
	 * Update the prev values
	 */
		mr_prev = mr;
		p_prev = p[i];
	}
/*
 * We have the point after the 50 mb mark.  Interpolate to 50 mb and
 * add the last little bit to our sum
 */
	mr = w_sat (dp[i] + T_K, p[i]);
	mr = mr_prev + (p_top - p_prev) / (p[i] - p_prev) * (mr - mr_prev);

	sum += (p_prev - p_top) * (mr + mr_prev) / 2.0;
/*
 * Divide the sum by 50.0 to get the mean mixing ratio, then get
 * the corresponding dewpoint.
 */
	mr = sum / 50.0;
	*dp_sfc = t_mr (*p_sfc, mr) - T_K;
/*
 * Done
 */
	return;
}




float
an_li (p, t, npts, theta_e_lcl, verbose)
float	*p, *t, theta_e_lcl;
int	npts, verbose;
/*
 * Find the lifted index (or modified lifted index) for the sounding
 */
{
	int	i;
	float	p_prev = BADVAL, t_prev = BADVAL, temp, li;
	float	ndx_level = Flg_mli ? 400.0 : 500.0;
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
		if (p[i] <= ndx_level)
			break;

		p_prev = p[i];
		t_prev = t[i];
	}
/*
 * Sanity check
 */
	if (p_prev == BADVAL)
	{
		ui_printf ("Cannot calculate lifted index\n");
		return (BADVAL);
	}
/*
 * Interpolate the temperature and find the lifted index
 */
	temp = t_prev + (t[i] - t_prev) * (ndx_level - p_prev) /
		(p[i] - p_prev) + T_K;
	li = temp - t_sat (theta_e_lcl, ndx_level);
/*
 * Be verbose (if requested)
 */
	if (verbose)
		ui_printf (
		"%d mb temperature: %.1f K (potential temp.: %.1f K)\n",
			(int) ndx_level, temp, theta_dry (temp, ndx_level));
/*
 * Done
 */
	return (li);
}




float
an_fmli (p, t, npts, p_sfc, dp_sfc)
float	*p, *t, p_sfc, dp_sfc;
int	npts;
/*
 * Find the forecasted modified lifted index
 */
{
	int	i;
	float	temp700, dp700, p_prev, t_prev, p_lcl, t_lcl;
	float	theta_e_lcl, w;
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
/*
 * Make sure we have two good points to interpolate between
 */
	if (p_prev == BADVAL)
	{
		ui_printf ("Unable to find 700 mb temperature\n");
		return (BADVAL);
	}
/*
 * Interpolate the 700 mb temperature from the point we just found and the
 * previous good point
 */
	temp700 = t_prev + (700.0 - p_prev) / (p[i] - p_prev) * 
		(t[i] - t_prev) + T_K;
	ui_printf (
		"700 mb temperature: %.1f K (potential temp.: %.1f K)\n",
		temp700, theta_dry (temp700, 700.0));
/*
 * Find the mixing ratio of our surface dewpoint, then find the
 * corresponding 700 mb dewpoint
 */
	w = w_sat (dp_sfc + T_K, p_sfc);
	dp700 = t_mr (700.0, w);
/*
 * Get LCL info using the surface dewpoint and the 700 mb temperature
 */
	p_lcl = lcl_pres (temp700, dp700, 700.0);
	t_lcl = lcl_temp (temp700, dp700);
	theta_e_lcl = theta_w (t_lcl, p_lcl);
/*
 * Actually find the forecasted lifted index
 */
	return (an_li (p, t, npts, theta_e_lcl, FALSE));
}

