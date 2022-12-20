#include <windows.h>
#include <math.h>

#include "2dlib.h"

/* Transformationsmatrizen initialisieren */

MATRIX2D InitRotMat(double wi)	//Initialisiert Rotationsmatrix
{
	MATRIX2D t;
	wi = wi*PI/180.;
	t.v[0][0] = cos(wi);
	t.v[0][1] = -sin(wi);
	t.v[0][2] = 0.;
	t.v[1][0] = sin(wi);
	t.v[1][1] = cos(wi);
	t.v[1][2] = 0.;
	t.v[2][0] = 0.;
	t.v[2][1] = 0.;
	t.v[2][2] = 1.;
	return t;
}

MATRIX2D InitScalMat(double s) //Initialisiert Skalierungsmatrix
{
	MATRIX2D t;
	t.v[0][0] = s;
	t.v[0][1] = 0.;
	t.v[0][2] = 0.;
	t.v[1][0] = 0.;
	t.v[1][1] = s;
	t.v[1][2] = 0.;
	t.v[2][0] = 0.;
	t.v[2][1] = 0.;
	t.v[2][2] = 1.;
	return t;
}

MATRIX2D InitTransMat(double tx, double ty) //Initialisiert Translationsmatrix
{
	MATRIX2D t;
	t.v[0][0] = 1.;
	t.v[0][1] = 0.;
	t.v[0][2] = tx;
	t.v[1][0] = 0.;
	t.v[1][1] = 1.;
	t.v[1][2] = ty;
	t.v[2][0] = 0.;
	t.v[2][1] = 0.;
	t.v[2][2] = 1.;
	return t;
}

/* Matrix Vektor Muliplikation */

POINT2D MxV(MATRIX2D a, POINT2D x) // Matrix mal Vektor
{	int i, k;			/* multipliziert a[i,k] * b[k] = c[i] */
	POINT2DHOM b,c;
	POINT2D erg;
	b.v[0] = x.x;	//Homogene Koordianten generieren
	b.v[1] = x.y;
	b.v[2] = 1.;
	for (i = 0; i < DIM; i++)
			for (c.v[i] = 0, k = 0; k < DIM; k++)	/* Initialisierung!*/
				 c.v[i] += a.v[i][k] * b.v[k];		/* Summenbildung */
	erg.x = c.v[0]/c.v[2];							// Normieren mit 3tem Element
	erg.y = c.v[1]/c.v[2];
	return erg;
}

/* Matrix Matrix Multiplikation */

MATRIX2D MxM(MATRIX2D a, MATRIX2D b)
{
	MATRIX2D c;
	int i, j, k;								/* multipliziert a[J,K] * b[K,J] = c[I,J] */
	for (i = 0; i < DIM; i++)
		for (j = 0; j < DIM; j++)
				for (c.v[i][j] = 0, k = 0; k < DIM; k++)	/* Initialisierung!*/
				 	c.v[i][j] += a.v[i][k] * b.v[k][j];		/* Summenbildung */
	return c;
}

POINT Pint(POINT2D pd)
{
	POINT pi;
	pi.x = (int)pd.x;
	pi.y = (int)pd.y;
	return pi;
}

POINT2D Pdouble(POINT pi)
{
	POINT2D pd;
	pd.x = (double)pi.x;
	pd.y = (double)pi.y;
	return pd;
}
