#ifndef STAPLER_SCARA_LIB_H
#define STAPLER_SCARA_LIB_H

#include "2dlib.h"

#define UZ 0
#define GUZ 1

void Initialize(double rmax, double rmin, double scale, int cspur);

// Bewegt den Scara auf einer Spiralförmeigen Bahn von PAkt zu PZiel mit der Geschwingkeit velo
// uz gibt an ob im oder gegen den Uhrzeugersinn (Konstanten UZ und GUZ)
// Zeichnet Spur falls hSpurPen ungleich NULL
// OFlag steuert, ob eine Umgebung mit der Funktion DrawItems gezeichnet werden soll
// Rückgabewert ist neue Position
POINT2D MoveScaraSpiral(HDC hdc, POINT2D PZiel, POINT2D PAkt, int velo, int uz, HPEN hSpurPen, int OFlag);

// Bewegt den Scara linear von PAkt zu PZiel mit der Geschwingkeit velo
// Zeichnet Spur falls hSpurPen ungleich NULL
// OFlag steuert, ob eine Umgebung mit der Funktion DrawItems gezeichnet werden soll
// Rückgabewert ist neue Position
POINT2D MoveScaraLin(HDC hdc, POINT2D PZiel, POINT2D PAkt, int velo, HPEN hSpurPen, int OFlag);

// Positioniert den Roboter auf den Punkt Psoll
// Zeichnet Spurm falls hSpurPen ungleich NULL
// OFlag steuert, ob eine Umgebung mit der Funktion DrawItems gezeichnet werden soll
// Rückgabewert ist neue Position
POINT2D PosScara(HDC hdc, POINT2D Psoll, HPEN hSpurPen, int OFlag);

// Bewegt den Scara auf einem Kreisbogen mit Mitte PM mit Radius radius vom Anfangswinkel wia zum Endwinkel wie
// Funktion analog AngleArc(), d.h. es wird von der aktuellen Pos aus gestartet
// Geschwingkeit velo
// uz gibt an ob im oder gegen den Uhrzeugersinn (Konstanten UZ und GUZ)
// Zeichnet Spur falls hSpurPen ungleich NULL
// OFlag steuert, ob eine Umgebung mit der Funktion DrawItems gezeichnet werden soll
// Rückgabewert ist neue Position
POINT2D MoveScaraArc(HDC hdc, POINT2D Pm, double radius, double wia, double wie, int velo, HPEN hSpurPen, int OFlag);

#endif /* STAPLER_SCARA_LIB_H */