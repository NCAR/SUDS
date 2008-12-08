/*
 * Cubic spline package
 *
 * $Revision: 1.2 $ $Date: 1990-05-03 15:23:58 $ $Author: burghart $
 *
 * Equations here are taken from "Numerical Analysis",
 * L. W. Johnson and R. D. Riess, Addison-Wesley 1982.
 */
# include <math.h>
# include <stdlib.h>
# include <string.h>

# define TRUE	1
# define FALSE	0

/*
 * The x, f, h, and y'' vectors and value n used in Eqns. 5.43-5.46
 */
static float	*X = 0, *F = 0, *H = 0, *Y_pp = 0;
static int	N = 0;




void
spline (x, f, npts)
float	*x, *f;
int	npts;
/*
 * Find the parameters describing the natural cubic spline through
 * the given points.
 */
{
	float	*b, *u, *l, *y, delterm, delprev;
	int	i;
/*
 * Make sure we have enough points
 */
	if (npts < 2)
		ui_error ("Attempt to spline fewer than three points");

	N = npts - 1;	/* Corresponds to n in Eq. 5.46 of J & R */
/*
 * If we only have 2 points (N == 1), we'll just do a linear interpolation
 * in spline_eval, so we can return now
 */
	if (N == 1)
	{
		if (X)
			free (X);
		X = (float *) malloc (2 * sizeof (float));
		memcpy (X, x, 2 * sizeof (float));

		if (F)
			free (F);
		F = (float *) malloc (2 * sizeof (float));
		memcpy (F, f, 2 * sizeof (float));

		return;
	}
/*
 * Copy the data into local arrays and allocate the h and y'' arrays.
 */
	if (X)
		free (X);
	if (F)
		free (F);
	if (H)
		free (H);
	if (Y_pp)
		free (Y_pp);

	X = (float *) malloc (npts * sizeof (float));
	memcpy (X, x, npts * sizeof (float));

	F = (float *) malloc (npts * sizeof (float));
	memcpy (F, f, npts * sizeof (float));

	H = (float *) malloc (N * sizeof (float));
	Y_pp = (float *) malloc (npts * sizeof (float));
/*
 * Allocate the temporary arrays
 */
	b = (float *) malloc (N * sizeof (float));
	u = (float *) malloc (N * sizeof (float));
	l = (float *) malloc (N * sizeof (float));
	y = (float *) malloc (N * sizeof (float));
/*
 * Build the h and b vectors, as defined just before Eq. 5.41 and in Eq. 5.45
 */
	for (i = 0; i < N; i++)
		H[i] = x[i+1] - x[i];

	delprev = (f[1] - f[0]) / H[0];
	for (i = 1; i < N; i++)
	{
		delterm = (f[i+1] - f[i]) / H[i];
		b[i] = 6 * (delterm - delprev);
		delprev = delterm;
	}
/*
 * Apply the LU decomposition Eq. 2.32 which applies to tridiagonal 
 * matrices.  The matrix used comes from Eq. 5.46.
 * The indices used in the code correspond to the indices used in J & R.
 * Note that N is the n from 5.46; the n used in 2.32-2.34 is equal to N-1.
 *
 * We represent l      with l[i] and u    with u[i].
 *               i,i-1                i,i
 *
 * The following three substitutions, from Eq. 5.46, are made implicitly below
 * in the formulas for l[i] and u[i]:
 *
 *       a      = h     and   a      = h     and   a    = 2*(h  + h   )
 *        i,i-1    i-1         i-1,i    i-1         i,i       i    i-1
 */
	u[1] = 2 * (H[1] + H[0]);

	for (i = 2; i < N; i++)
	{
		l[i] = H[i-1] / u[i-1];
		u[i] = 2 * (H[i] + H[i-1]) - (l[i] * H[i-1]);
	}
/*
 * Solve for the y vector as in 2.33b.  The n used in 2.33 equals N-1. 
 */
	y[1] = b[1];

	for (i = 2; i < N; i++)
		y[i] = b[i] - l[i] * y[i-1];
/*
 * Finally, solve for the y'' vector of 2.46 (the x vector of 2.33b).  
 * Remember that n in 2.33 equals N-1.
 */
	Y_pp[0] = Y_pp[N] = 0.0;	/* zeros for natural cubic spline */

	Y_pp[N-1] = y[N-1] / u[N-1];

	for (i = 1; i < N; i++)
		Y_pp[N-i] = (y[N-i] - H[N-i] * Y_pp[N+1-i]) / u[N-i];
/*
 * Free the temporary arrays
 */
	free (b);
	free (u);
	free (l);
	free (y);
}




float
spline_eval (x, ok)
float	x;
int	*ok;
/*
 * Evaluate the calculated spline at the given x location.  It is assumed
 * that routine spline() has already been called.  Return ok as true unless
 * we are not able to evaluate at x.
 */
{
	int	j;
	float	del1, del2, h, val;
/*
 * Find the appropriate interval for this x; bail out if it's outside 
 * the legal range.
 */
	for (j = 0; j < N; j++)
		if (x >= X[j] && x <= X[j+1])
			break;

	if (j == N)
	{
		*ok = FALSE;
		return (0.0);
	}
/*
 * If N is 1, just do a linear interpolation, since we didn't have enough
 * points for a cubic spline
 */
	if (N == 1)
	{
		*ok = TRUE;
		return ((x - X[0]) / (X[1] - X[0]) * (F[1] - F[0]) + F[0]);
	}
/*
 * x is between X[j] and X[j+1], apply 5.43 from J & R to find the value
 * of the spline.
 */
	del1 = X[j+1] - x;
	del2 = x - X[j];
	h = H[j];

	val = Y_pp[j] / (6 * h) * del1 * del1 * del1  + 
		Y_pp[j+1] / (6 * h) * del2 * del2 * del2 + 
		(F[j+1] / h - Y_pp[j+1] * h / 6) * del2 +
		(F[j] / h - Y_pp[j] * h / 6) * del1;

	*ok = TRUE;

	return (val);
}
