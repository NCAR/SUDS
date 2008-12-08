/*
 * Contour a rectangular array
 */
/*
 *		Copyright (C) 1988-91 by UCAR
 *	University Corporation for Atmospheric Research
 *		   All rights reserved
 *
 * No part of this work covered by the copyrights herein may be reproduced
 * or used in any form or by any means -- graphic, electronic, or mechanical,
 * including photocopying, recording, taping, or information storage and
 * retrieval systems -- without permission of the copyright owner.
 * 
 * This software and any accompanying written materials are provided "as is"
 * without warranty of any kind.  UCAR expressly disclaims all warranties of
 * any kind, either express or implied, including but not limited to the
 * implied warranties of merchantibility and fitness for a particular purpose.
 * UCAR does not indemnify any infringement of copyright, patent, or trademark
 * through use or modification of this software.  UCAR does not provide 
 * maintenance or updates for its software.
 */

static char *rcsid = "$Id: contour.c,v 1.7 1998-05-15 21:43:38 burghart Exp $";

# include <errno.h>
# include <math.h>
# include <stdio.h>
# include <stdlib.h>
# include <string.h>

# include <graphics.h>

# define TRUE	1
# define FALSE	0

# define ABS(x)	((x) < 0 ? -(x) : (x))	/* Cheap absolute value */

# define PI		3.141592654
# define RAD_TO_DEG(x)	((x)*57.29577951)

/*
 * The array is contoured by breaking it into triangles so that the
 * contours are uniquely determined.  The nx by ny array is divided 
 * into (nx-1)*(ny-1) triangles thus:
 *
 *	ny-1 --------------------- . . . . -----------
 *	     |0,ny-2,1/|1,ny-2,1/|         |   <--------- nx-2,ny-2,1
 *	     |     _/  |     _/  |         |     _/  |
 *	     |   _/    |   _/    |         |   _/    |
 *	     | _/      | _/      |         | _/    <----- nx-2,ny-2,0
 *	     |/0,ny-2,0|/1,ny-2,0|         |/        |
 *	ny-2 --------------------- . . . . -----------
 *           .         .         .         .         .
 *           .         .         .         .         .
 *           .         .         .         .         .
 *           .         .         .         .         .
 *           .         .         .         .         .
 *	   2 --------------------- . . . . -----------
 *	     | 0,1,1 _/| 1,1,1 _/|         |nx-2,1,1/|
 *	     |     _/  |     _/  |         |     _/  |
 *	     |   _/    |   _/    |         |   _/    |
 *	     | _/      | _/      |         | _/      |
 *	     |/  0,1,0 |/  1,1,0 |         |/nx-2,1,0|
 *	   1 --------------------- . . . . -----------
 *	     | 0,0,1 _/| 1,0,1 _/|         |nx-2,0,1/|
 *	     |     _/  |     _/  |         |     _/  |
 *	     |   _/    |   _/    |         |   _/    |
 *	     | _/      | _/      |         | _/      |
 *	     |/  0,0,0 |/  1,0,0 |         |/nx-2,0,0|
 *	   0 --------------------- . . . . -----------
 *	     0         1         2        nx-2      nx-1
 *
 * Each triangle is designated by the x,y coordinates of its lower left corner
 * and an index (either zero or one) to tell whether it is the lower right
 * or the upper left triangle in the rectangle.  The designation for each 
 * triangle is shown.
 */

/*
 * Triangle and vertex structures
 */
typedef struct
{
	int	i, j, k;
} triangle;

typedef struct
{
	int	i, j;
} vertex;


/*
 * The array we're contouring, its x and y dimensions
 * and a macro to reference it by vertex
 */
static float	*Data;
static int	Nx, Ny;
static float	Badflag;
static int	Use_flag = FALSE;
# define DATA(v)	Data[(v.i) * Ny + (v.j)]

/*
 * The overlay coordinates for the array
 */
static float	Xbottom, Ybottom, Xstep, Ystep;

/*
 * Boolean array to keep track of whether we have done a given triangle yet
 * and a macro to convert the three value triangle number into an index 
 * into the Did_tri array
 */
static char	*Did_tri;
# define DID_TRI(tri)	Did_tri[((tri.i) * (Ny-1) + (tri.j)) * 2 + (tri.k)]

/*
 * Is the triangle within the bounds?
 */
# define LEGAL_TRI(tri)	((tri.i)>=0&&(tri.i)<=Nx-2&&(tri.j)>=0&&(tri.j)<=Ny-2)

/*
 * Arrays for the polyline used to draw the contours
 */
# define MAXPTS	512
static float	Xpt[MAXPTS], Ypt[MAXPTS];
static int	Npt = 0;

/*
 * Color stuff
 */
static int	Color_center = 1, Ncolor = 1, Color_outrange = 1;

/*
 * Contour color, line style, and label string
 */
static int	C_color, Linestyle;
static char	Label[16];

/*
 * Toggle so labels on adjacent contours are slightly offset from each
 * other
 */
static int	Ltoggle = FALSE;

/*
 * Our overlay
 */
static overlay	C_ov;


/*
 * Forward declarations
 */
int	con_minmax ();
void	con_do_contour (), con_draw_contour ();
void	con_graze (), con_addpoint ();






contour (array, xdim, ydim, ov, xlo, ylo, xhi, yhi, ccenter, cstep)
float	*array, xlo, ylo, xhi, yhi, ccenter, cstep;
int	xdim, ydim;
overlay	ov;
/*
 * Draw contours of the rectangular (xdim x ydim) array into overlay ov.
 * The coordinates (xlo,ylo) and (xhi,yhi) specify the spatial extent of
 * the array.  The values ccenter and cstep are the center contour value
 * and the contour spacing, respectively.
 */
{
	int	cndx, cndx_min, cndx_max, prec;
	float	min, max, cval, maxabs;
	triangle	tri;
	char	testlabel[16];
/*
 * Initialize
 */
	Data = array;
	Nx = xdim;
	Ny = ydim;
	Xbottom = xlo;
	Ybottom = ylo;
	Xstep = (xhi - xlo) / (Nx - 1);
	Ystep = (yhi - ylo) / (Ny - 1);
	C_ov = ov;
/*
 * Traverse the array to find the min and max
 */
	if (! con_minmax (&min, &max))
		return;

	if (max == min)
	{
		ui_warning ("Constant surface in contour, nothing drawn");
		return;
	}
/*
 * Allocate the Did_tri array
 */
	Did_tri = (char *) calloc ((Nx - 1) * (Ny - 1) * 2, sizeof (char));
/*
 * Loop through the contour values
 */
	cndx_min = (int) ceil ((min - ccenter) / cstep);
	cndx_max = (int) floor ((max - ccenter) / cstep);

	for (cndx = cndx_min; cndx <= cndx_max; cndx++)
	{
		cval = ccenter + cndx * cstep;
	/*
	 * Assign the line style and color
	 */
# ifdef notdef
		Linestyle = (cval < 0) ? GPLT_DOT : GPLT_SOLID;
# endif
		Linestyle = GPLT_SOLID;

		if (ABS (cndx) > (Ncolor - 1) / 2)
			C_color = Color_outrange;
		else
			C_color = Color_center + cndx;
	/*
	 * Play around to get the shortest label which will unambiguously 
	 * identify this contour
	 */
		prec = -(int) (log10 (cstep));
		if (prec < 0)
			prec = 0;
		else if ((cstep / pow (10.0, (double)(-prec)) - 1.0) < -1.0e-4)
			prec += 1;
		sprintf (Label, " %.*f ", prec, cval);

		maxabs = fabs (max) > fabs (min) ? fabs (max) : fabs (min);
		prec = (int) (log10 (maxabs / cstep));
		sprintf (testlabel, " %.*E ", prec, cval);
		if (strlen (testlabel) < strlen (Label))
			strcpy (Label, testlabel);
	/*
	 * Alternate the toggle so that adjacent contours have slightly
	 * offset labels
	 */
		Ltoggle = ! Ltoggle;
	/*
	 * Initialize the Did_tri array to all FALSE
	 */
		for (tri.i = 0; tri.i < Nx - 1; tri.i++)
		    for (tri.j = 0; tri.j < Ny - 1; tri.j++)
			for (tri.k = 0; tri.k < 2; tri.k++)
				DID_TRI (tri) = FALSE;
	/*
	 * Loop through the triangles
	 */
		for (tri.i = 0; tri.i < Nx - 1; tri.i++)
		    for (tri.j = 0; tri.j < Ny - 1; tri.j++)
			for (tri.k = 0; tri.k < 2; tri.k++)
			{
				con_do_contour (tri, cval, -1);
				con_draw_contour ();
			}
	}
/*
 * Release Did_tri
 */
	cfree (Did_tri);
}




void
contour_init (center, count, c_outrange, flagged, flagval)
int	center, count, c_outrange, flagged;
float	flagval;
/*
 * Initialize colors and data flagging
 *
 * CENTER	center color index
 * COUNT	number of colors in range to use for contouring
 * C_OUTRANGE	color index for contours which fall outside of the specified
 *		range of colors
 * FLAGGED	Boolean value telling whether the bad values in the data
 * 		will be flagged
 * FLAGVAL	the bad value used to identify flagged data
 */
{
/*
 * Center color index and number of colors.
 * Make sure Ncolor is odd, so the Color_center is actually at the center
 */
	Color_center = center;
	Ncolor = (count % 2) ? count : count - 1;
/*
 * Color index for out-of-range contours
 */
	Color_outrange = c_outrange;
/*
 * Handle data flagging, if any
 */
	Use_flag = flagged;
	Badflag = flagval;
}




int
con_minmax (min, max)
float	*min, *max;
/*
 * Find the min and max values in the data array.
 * Return TRUE iff we are able to find min and max values.
 */
{
	int	i;
/*
 * Start with the first good value
 */
	for (i = 0; i < Nx * Ny; i++)
	{
		if (Use_flag && Data[i] == Badflag)
			continue;
		else
		{
			*max = Data[i];
			*min = Data[i];
			break;
		}
	}

	if (i == Nx * Ny)
	{
		ui_warning ("No good values in array to be contoured!");
		return (FALSE);
	}
/*
 * Find the max and min
 */
	for (; i < Nx * Ny; i++)
	{
		if (Use_flag && Data[i] == Badflag)
			continue;
		if (Data[i] > *max)
			*max = Data[i];
		if (Data[i] < *min)
			*min = Data[i];
	}

	return (TRUE);
}




void
con_do_contour (tri, cval, side_from)
triangle	tri;
int	side_from;
float	cval;
/*
 * Continue a contour of value cval in the given triangle.
 * Parameter 'side_from' identifies the side this triangle
 * has in common with the previous triangle.  If side_from is less than
 * zero, we're trying to start a contour here.
 */
{
	int	end_side = -1, lower_right, i;
	int	side, ep0, ep1;
	float	v_val[3], val0, val1, frac, xpt, ypt;
	vertex	v[3];
	triangle	next;
/*
 * Return if the triangle is outside the defined boundaries
 */
	if (! LEGAL_TRI (tri))
		return;
/*
 * Return if we already did this triangle.
 */
	if (DID_TRI (tri))
		return;
/*
 * Get the vertices of the triangle.
 * The points are defined such that sides are consistent for both
 * lower right and upper left triangles.
 *
 *	Vertices	Side defined
 *	----------------------------------------
 *	 2 and 0	horizontal side (side 0)
 *	 0 and 1	diagonal side	(side 1)
 *	 1 and 2	vertical side	(side 2)
 */
	lower_right = (tri.k == 0);

	if (lower_right)	/* lower right triangle */
	{
		v[0].i = tri.i;		v[0].j = tri.j;
		v[1].i = tri.i + 1;	v[1].j = tri.j + 1;
		v[2].i = tri.i + 1;	v[2].j = tri.j;
	}
	else			/* upper left triangle */
	{
		v[0].i = tri.i + 1;	v[0].j = tri.j + 1;
		v[1].i = tri.i;		v[1].j = tri.j;
		v[2].i = tri.i;		v[2].j = tri.j + 1;
	}

	v_val[0] = DATA (v[0]);
	v_val[1] = DATA (v[1]);
	v_val[2] = DATA (v[2]);
/*
 * Bail out when all vertices are the same
 */
	if (v_val[0] == v_val[1] && v_val[0] == v_val[2])
		return;
/*
 * Deal with contours which pass through a vertex
 */
	for (i = 0; i < 3; i++)
		if (v_val[i] == cval)
		{
			con_graze (v[i], cval);
			con_draw_contour ();
		}
/*
 * Return now if this triangle was finished as the result of a con_graze
 * call, otherwise mark is as done and go on.  This must be done before
 * a possible recursive call to con_do_contour, so we don't try to do
 * this triangle again.
 */
	if (DID_TRI (tri))
		return;
	else
		DID_TRI (tri) = TRUE;
/*
 * Run through the sides of the triangle to see if the contour
 * crosses them.  If so, add a point to our polyline list
 */
	for (side = 0; side < 3; side++)
	{
	/*
	 * If this is the side we have in common with the previous triangle,
	 * skip it.
	 */
		if (side == side_from)
			continue;
	/*
	 * Get the data values at either endpoint of the side.
	 */
		ep1 = side;
		ep0 = (side == 0) ? 2 : side - 1;

		val0 = v_val[ep0];
		val1 = v_val[ep1];
	/*
	 * Bail out if either endpoint is bad
	 */
		if (Use_flag && (val0 == Badflag || val1 == Badflag))
			continue;
	/*
	 * If we just grazed this side, go on to the next side
	 */
		if (cval == val0 || cval == val1)
			continue;
	/*
	 * If the contour doesn't cross this side, go on to the next side
	 */
		if ((cval < val0 && cval < val1)||(cval > val0 && cval > val1))
			continue;	
	/*
	 * Fractional distance along side where contour crosses
	 */
		frac = (cval - val0) / (val1 - val0);
	/*
	 * Add a point to the list for this contour
	 */
		xpt = Xbottom + v[ep0].i * Xstep + 
			(v[ep1].i - v[ep0].i) * frac * Xstep;
		ypt = Ybottom + v[ep0].j * Ystep + 
			(v[ep1].j - v[ep0].j) * frac * Ystep;
		con_addpoint (xpt, ypt);
	/*
	 * Remember the last side to which we add a point
	 */
		end_side = side;
	}
/*
 * Return now if the contour didn't touch this triangle at all
 */
	if (end_side < 0)
		return;
/*
 * Find the triangle which has end_side in common with the current triangle
 */ 
	switch (end_side)
	{
	    case 0:
		next.i = tri.i;
		next.j = (lower_right) ? tri.j - 1 : tri.j + 1;
		break;
	    case 1:
		next.i = tri.i;
		next.j = tri.j;
		break;
	    case 2:
		next.i = (lower_right) ? tri.i + 1 : tri.i - 1;
		next.j = tri.j;
		break;
	    default:
		ui_error ("*BUG* bad triangle side number %d in contour", 
			end_side);
	}

	next.k = (lower_right) ? 1 : 0;
/*
 * Continue the contour through the triangle we just found
 */
	con_do_contour (next, cval, end_side);
}




void
con_graze (v_in, cval)
vertex	v_in;
float	cval;
/*
 * The cval contour passes through the given vertex, so test the six
 * adjacent triangles to find where to continue the contour
 */
{
	int		n, next, prev, side;
	float		val0, val1, frac, x_in, y_in, xpt, ypt;
	vertex		v[6];
	triangle	t[6], next_tri;
/*
 * Add a point to the contour at v_in (if it's not the same as the
 * previous point)
 */
	x_in = Xbottom + v_in.i * Xstep;
	y_in = Ybottom + v_in.j * Ystep;

	if (Xpt[Npt-1] != x_in || Ypt[Npt-1] != y_in)
		con_addpoint (x_in, y_in);
/*
 * We index the adjacent triangles and vertices thus:
 *
 *			v0    v1
 *			+-----+
 *		       /| t0 /|
 *		     /  |  /  |
 *		   / t5 |/ t1 |
 *	       v5 +-----#-----+ v2
 *		  | t4 /| t2 /
 *		  |  /  |  /
 *		  |/ t3 |/
 *		  +-----+
 *		  v4     v3
 *
 * where the '#' represents the vertex v_in
 */
	v[0].i = v_in.i;	v[0].j = v_in.j + 1;
	v[1].i = v_in.i + 1;	v[1].j = v_in.j + 1;
	v[2].i = v_in.i + 1;	v[2].j = v_in.j;
	v[3].i = v_in.i;	v[3].j = v_in.j - 1;
	v[4].i = v_in.i - 1;	v[4].j = v_in.j - 1;
	v[5].i = v_in.i - 1;	v[5].j = v_in.j;

	t[0].i = v_in.i;	t[0].j = v_in.j;	t[0].k = 1;
	t[1].i = v_in.i;	t[1].j = v_in.j;	t[1].k = 0;
	t[2].i = v_in.i;	t[2].j = v_in.j - 1;	t[2].k = 1;
	t[3].i = v_in.i - 1;	t[3].j = v_in.j - 1;	t[3].k = 0;
	t[4].i = v_in.i - 1;	t[4].j = v_in.j - 1;	t[4].k = 1;
	t[5].i = v_in.i - 1;	t[5].j = v_in.j;	t[5].k = 0;
/*
 * Loop through the adjacent triangles
 */
	for (n = 0; n < 6; n++)
	{
		next = (n < 5) ? n + 1 : 0;
		prev = (n > 0) ? n - 1 : 5;
	/*
	 * Don't use this triangle if it's out of bounds or if it has been
	 * done already
	 */
		if (! LEGAL_TRI (t[n]) || DID_TRI (t[n]))
			continue;
	/*
	 * Mark this triangle as done
	 */
		DID_TRI (t[n]) = TRUE;
	/*
	 * Find the values at the endpoints of the outer side of the 
	 * current triangle
	 */
		val0 = DATA (v[n]);
		val1 = DATA (v[next]);
	/*
	 * Bail out if either endpoint is bad
	 */
		if (Use_flag && (val0 == Badflag || val1 == Badflag))
			continue;
	/*
	 * Check for a constant value triangle.  If it is constant value
	 * mark it as done and continue
	 */
		if (cval == val0 && cval == val1)
			continue;
	/*
	 * Add the point at v_in to the polyline list
	 */
		xpt = Xbottom + v_in.i * Xstep;
		ypt = Ybottom + v_in.j * Ystep;
		con_addpoint (xpt, ypt);
	/*
	 * If the contour passes through one of the triangle's outside
	 * vertices, mark the adjacent triangles as done and make a 
	 * recursive call to con_graze at that vertex
	 */
		if (val0 == cval)
		{
			if (LEGAL_TRI (t[prev]))
				DID_TRI (t[prev]) = TRUE;

			con_graze (v[n], cval);

			con_draw_contour ();
			con_addpoint (x_in, y_in);
		}
		else if (val1 == cval)
		{
			if (LEGAL_TRI (t[next]))
				DID_TRI (t[next]) = TRUE;

			con_graze (v[next], cval);

			con_draw_contour ();
			con_addpoint (x_in, y_in);
		}
	/*
	 * Otherwise, see if the contour passes through the side 
	 * opposite v_in
	 */
		if ((cval > val0 && cval < val1)||(cval > val1 && cval < val0))
		{
			frac = (cval - val0) / (val1 - val0);
		/*
		 * Add a point to the list
		 */
			xpt = Xbottom + v[n].i * Xstep + 
				(v[next].i - v[n].i) * frac * Xstep;
			ypt = Ybottom + v[n].j * Ystep + 
				(v[next].j - v[n].j) * frac * Ystep;
			con_addpoint (xpt, ypt);
		/*
		 * Find the side we're going through and the triangle
		 * we're entering.  (The equations are not obvious, 
		 * but they are correct)
		 */
			side = (3 - n) % 3;

			next_tri.i = v_in.i + ABS (3-prev) - 2;
			next_tri.j = v_in.j + ABS (3-n) - 2;
			next_tri.k = (n % 2) ? 1 : 0;
		/*
		 * Continue the contour from here
		 */
			con_do_contour (next_tri, cval, side);

			con_draw_contour ();
			con_addpoint (x_in, y_in);
		}
	}
/*
 * If we make it here, the contour was not continued
 */
	return;
}




void
con_addpoint (x, y)
float	x, y;
/*
 * Add a point to the polyline for the contour
 */
{
/*
 * Put the point in the list
 */
	Xpt[Npt] = x;
	Ypt[Npt] = y;
	Npt++;
/*
 * If the list isn't full, return
 */
	if (Npt < MAXPTS)
		return;
/*
 * The list is full, so draw the contour and make the current point
 * the first point of the next polyline
 */
	con_draw_contour ();

	Xpt[0] = x;
	Ypt[0] = y;
	Npt = 1;
}




void
con_draw_contour ()
/*
 * Draw the contour polyline which has been built
 */
{
	int	prev, start, i, label_now, count, hjust;
	float	width = ((Nx - 1) * Xstep);
	float	height = ((Ny - 1) * Ystep);
	float	angle, charsize, dist, del_x, del_y, x0, y0, x1, y1;
	float	test1, test2, fudge;

	if (Npt <= 1)
	{
		Npt = 0;
		return;
	}
/*
 * Set the character size
 */
	charsize = 0.025 * height;
/*
 * Set things up as if we've gone some distance through a contour, so 
 * we get the first label quickly
 */
	dist = Ltoggle ? 0.445 : 0.495;
/*
 * Run through the points of the contour, drawing polylines and inserting
 * labels where appropriate 
 */
	start = 0;
	count = 1;
	label_now = FALSE;

	for (i = 1; i < Npt; i++)
	{
		count++;

		if (! label_now)
		{
		/*
		 * This point will be in the polyline.  See if it takes
		 * us far enough to do the next label
		 */
			prev = i - 1;
			del_x = Xpt[i] - Xpt[prev];
			del_y = Ypt[i] - Ypt[prev];

			dist += hypot (del_x / width, del_y / height);

			if (dist > 0.5)
			{
			/*
			 * We're far enough for the next label, draw 
			 * the line up to here
			 */
				label_now = TRUE;

				G_polyline (C_ov, Linestyle, C_color, count, 
					Xpt + start, Ypt + start);

				dist = 0.0;
				start = i;
				count = 1;
			}
		}
		else
		{
		/*
		 * See if we can fit the label between 'start' and the current 
		 * point
		 */
			del_x = Xpt[i] - Xpt[start];
			del_y = Ypt[i] - Ypt[start];

			if (del_x == 0.0 && del_y == 0.0)
				continue;

			angle = atan2 (del_y / height, del_x / width);

			if (angle > PI/2)
			{
				hjust = GT_RIGHT;
				angle -= PI;
			}
			else if (angle < -PI/2)
			{
				hjust = GT_RIGHT;
				angle += PI;
			}
			else
				hjust = GT_LEFT;
		/*
		 * Get the text box that would result if we put the label here
		 */
			G_wr_box (C_ov, GTF_MINSTROKE, charsize, hjust, 
				GT_CENTER, Xpt[start], Ypt[start], 
				RAD_TO_DEG (angle), Label, &x0, &y0, &x1, &y1);
		/*
		 * Go on to the next point if the current point would be 
		 * inside the text box
		 *
		 * We use the fudge factor in the test because G_wr_box returns
		 * values which are rounded to the nearest pixel location
		 */
			test1 = atan2 ((Ypt[i] - y0) / height, 
				(Xpt[i] - x0) / width) - angle;
			while (test1 < 0.0)
				test1 += 2 * PI;

			test2 = atan2 ((Ypt[i] - y1) / height, 
				(Xpt[i] - x1) / width) - angle - PI;
			while (test2 < 0.0)
				test2 += 2 * PI;

			fudge = 0.4;	/* angle fudge factor */
			if (test1 < (PI/2 + fudge) && test2 < (PI/2 + fudge))
				continue;
		/*
		 * This point is good, write in the label and continue the
		 * line from just after the label
		 */
			G_write (C_ov, C_color, GTF_MINSTROKE, charsize,
				hjust, GT_CENTER, Xpt[start], Ypt[start],
				RAD_TO_DEG (angle), Label);

			if (hjust == GT_LEFT)
			{
				Xpt[i-1] = x1 + 0.5 * charsize * sin (angle) *
					width / height;
				Ypt[i-1] = y1 - 0.5 * charsize * cos (angle);
			}
			else
			{
				Xpt[i-1] = x0 - 0.5 * charsize * sin (angle) * 
					width / height;
				Ypt[i-1] = y0 + 0.5 * charsize * cos (angle);
			}


			label_now = FALSE;
			start = i - 1;
			count = 2;
		}
	}
/*
 * Draw the remainder of the line
 */
	if (count > 1)
		G_polyline (C_ov, Linestyle, C_color, count, Xpt + start, 
			Ypt + start);
/*
 * Reset
 */
	Npt = 0;
}
