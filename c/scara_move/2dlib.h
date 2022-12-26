#ifndef STAPLER_2DLIB_H
#define STAPLER_2DLIB_H

//#define PI 3.14159265
static const double PI = 3.14159265;

// Anzahl der Punkte
#define NPT	6							

// Dimension of 3x3 matrix
#define DIM 3

// Matrix 3x3
typedef struct tagMATRIX2D {
	double v[DIM][DIM];
}MATRIX2D;

// Punkt im 2D als double
typedef struct tagPOINT2D {
	double x, y;
}POINT2D;

// Vektor 2D in homogenen Koordinaten = 3 Zeilen, 1 Spalte
typedef struct tagPOINT2DHOM {
	double v[DIM];
}POINT2DHOM;

// Initialisiert Skalierungsmatrix
MATRIX2D InitRotMat(double wi);

// Initialisiert Translationsmatrix
MATRIX2D InitTransMat(double tx, double ty);

// Matrix mal Vektor
POINT2D MxV(MATRIX2D a, POINT2D x);

POINT Pint(POINT2D pd);
POINT2D Pdouble(POINT pi);

#endif /* STAPLER_2DLIB_H */