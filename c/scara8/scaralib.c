//#include "unistd.h"
//#include "windows.h"

#define ABS(x) ((x)<0?-(x):(x))		// Betragmakro
#define PI 3.14159265
#define UZ 0
#define GUZ 1

extern HDC hdcMem;
/*
typedef struct tagPOINT2D{						// Punkt im 2D als double
	double x,y;
}POINT2D;
*/
/******  CalcAngles  ************************************************************************/
/* Berechnung der Winkel alpha und beta eines Scara Roboters zur beliebigen (x,y)-Position 	*/
/* p1 = Gelenkpunkt Arm1																	*/
/* p2 = Sollposition xy = Endpunkt Arm 2													*/
/* r1, r2 = Länge der Arme																	*/
/* Adressen der der Winkel alpha (Arm1) und beta (Arm1), müssen Ausgangspos enthalten		*/
/* Rückgabewert int = Anzahl der Lösungen 0, 1 oder 2										*/
/* 7.8.2008, A.Fritz ************************************************************************/

int CalcAngles(POINT2D p1, POINT2D p2, double r1, double r2, double *alpha, double *beta)
{
	double a,d,h2,h,dx,dy,ad,hd;			// Geometriegrößen
	double dys[2][2], wi[2][2], dwi[2];		// Deltay, Winkel zu Lösungen
	double s1x,s2x,s1y,s2y;					// Schnittpunkte
	int AnzLsg;								// Anzahl Lösungen = Rüggabewert
	int i,j;								// Laufvariablen
	int min;								// Hilfswert für Auswahl des Winkels

	dx = p2.x-p1.x;							// Delta x
	dy = p2.y-p1.y;							// Delta y
	d = sqrt(dx*dx+dy*dy);					// Abstand = Hypothenuse
	a = (r1*r1-r2*r2+d*d)/(2.*d);			// Hypothenusenabschnitt
	h2 = r1*r1-a*a;							// Quadrat der Höhe

	if(h2 < 0) return 0;					// Keine Lösung => Rückgabewert = 0
	if(h2 == 0) AnzLsg = 1;					// Eine Lösung (Kreise berühren sich)
		else AnzLsg = 2;					// Zwei Lösungen (üblicher Fall)

	h = sqrt(h2);							// Höhe
	ad = a/d;								// Hilfswert
	hd = h/d;								// Hilfswert
	s1x = p1.x+ad*dx-hd*dy;					// Schnittpunkt 1 x
	s1y = p1.y+ad*dy+hd*dx;					// Schnittpunkt 1 y
	s2x = p1.x+ad*dx+hd*dy;					// Schnittpunkt 2 x
	s2y = p1.y+ad*dy-hd*dx;					// Schnittpunkt 2 y

	wi[0][0] = acos((s1x-p1.x)/r1)*180/PI;	// alpha 1
	wi[0][1] = acos((s2x-p1.x)/r1)*180/PI;	// alpha 2
	wi[1][0] = acos((s1x-p2.x)/r2)*180/PI;	// beta 1
	wi[1][1] = acos((s2x-p2.x)/r2)*180/PI;	// beta 2

	dys[0][0] = s1y-p1.y;					// Delta y M1 zum Schnittpunkt 1
	dys[0][1] = s2y-p1.y;					// Delta y M1 zum Schnittpunkt 2
	dys[1][0] = s1y-p2.y;					// Delta y M2 zum Schnittpunkt 1
	dys[1][1] = s2y-p2.y;					// Delta y M2 zum Schnittpunkt 2

	for(i = 0; i < 2; i++)					// über beide Winkel
		for (j = 0; j < 2; j++)				// über beide Löungen
			if(dys[i][j] < 0) 				// delta y < 0 => dann Lösung in Q3 oder Q4
				wi[i][j] = 360.-wi[i][j]; 	// Winkel von 360° abziehen

	for (j = 0; j < 2; j++){				// über beide Lösungen
		dwi[j] = ABS(*alpha-wi[0][j]);		// Betrag delta Winkel rechnen
		if (dwi[j] >= 180) dwi[j]-=360.;	// ggfs. korrigieren
	}

	min = 0;								// minimalen Winkel suchen
	for (j = 0; j < 2; j++)					// über alle Lösungen
		if (ABS(dwi[j]) < ABS(dwi[min])) min = j;

	*alpha = wi[0][min];					// Rückgabe alpha
	*beta = wi[1][min]-180.;				// Rückgabe beta um 180° korrigiert

	if (*alpha >= 360.) *alpha -=360.;		// ggfs alpha auf 0..360° korrigieren
	if (*alpha < 0.) *alpha = ABS(*alpha);	// ggfs alpha auf 0..360° korrigieren
	return AnzLsg;							// Rückgabe Anzahl der Lösungen
}	// CalcAngles


POINT2D PosScara(HDC hdc, POINT2D Psoll, HPEN hSpurPen, int OFlag)
// Positioniert den Roboter auf den Punkt Psoll
// Zeichnet Spurm falls hSpurPen ungleich NULL
// OFlag steuert, ob eine Umgebung mit der Funktion DrawItems gezeichnet werden soll
// Rückgabewert ist neue Position
{
	POINT2D posAlt;
	int Lsg;								//Anzahl der Lösungen
	int i;
	HBRUSH hBrush1,hBrush2,hBrush3;			//Pinsel
	HPEN hPen1,hPen2=0,hPen3, hOldPen;		//Stifte

	hBrush1 = CreateSolidBrush(RGB(255,255,255));	//Hintergrund Löschen
	hPen1 = CreatePen(PS_SOLID,1,RGB(255,255,255));
	hOldPen = SelectObject(hdcMem, hPen1);
	//SelectObject(hdc, hBrush1);
	//Rectangle(hdc, -cx/2,cy/2,cx/2,-cy/2);

	Lsg = CalcAngles(aArm1[4],Psoll,R1,R2,&alpha,&beta);  //Winkel rechnen
	if(Lsg == 0){
			MessageBeep(MB_ICONEXCLAMATION);	//Pos nicht möglich
	}
	mRotA1 = InitRotMat(alpha);	//Rotationsmatrizen init
	mRotA2 = InitRotMat(beta);

	for(i = 0; i < NPT; i++){		// Punkte der Arme drehen
		   		aA1T[i] = MxV(mRotA1,aArm1[i]);
				aA2T1[i] = MxV(mRotA2,aArm2[i]);
				}

	mTransA2 = InitTransMat(aA1T[5].x,aA1T[5].y); // Translationsmatrix int

	for(i = 0 ; i < NPT; i++)
			aA2T[i] = MxV(mTransA2,aA2T1[i]); //Arm 2 verschieben

	Psoll = aA2T[5];  //Neue Position

	if(hSpurPen != NULL){
		spur[cspur] = Pint(Psoll);			// Spur zeichnen
		if (cspur < 999) cspur++;			// Spurbereich? Anzahl Spurpunkte
			else cspur = 0;						// rücksetzten
	}
	for(i=0;i<NPT;i++){				//Transformierte Punkte nach int casten
				aiA1T[i] = Pint(aA1T[i]);
				aiA2T[i] = Pint(aA2T[i]);
			}
	/* Koordinatensystem */
	SetMapMode(hdcMem, MM_ISOTROPIC);					//Y nach oben
	SetViewportOrgEx(hdcMem, cx/2, cy/2, NULL); 		// (0,0) in der Mitte
	SetWindowExtEx(hdcMem,cx, cy, NULL);
	SetViewportExtEx(hdcMem, cx*scale, -cy*scale, NULL);
	PatBlt(hdcMem,-cx/2,-cy/2,cx,cy, WHITENESS);

	SetMapMode(hdc, MM_ISOTROPIC);					//Y nach oben
	SetViewportOrgEx(hdc, cx/2, cy/2, NULL); 		// (0,0) in der Mitte
	SetWindowExtEx(hdc,cx, cy, NULL);
	SetViewportExtEx(hdc, cx*scale, -cy*scale, NULL);

	hPen1 = CreatePen(PS_DASHDOT, 1, RGB(0,255,0));	//grün, Strichpunkt
	SelectObject(hdcMem, hPen1);
	hBrush1 = GetStockObject(NULL_BRUSH);
	SelectObject(hdcMem, hBrush1);

//	Ellipse(hdcMem,-Rmax,-Rmax, Rmax, Rmax);  	// Arbeitsraum zeichnen
	Ellipse(hdcMem,-Rmin,-Rmin, Rmin, Rmin);
/*
    MoveToEx(hdcMem, -cx/2, 0, NULL);			// x-Achse ziehen
    LineTo(hdcMem, +cx/2, 0);

    MoveToEx(hdcMem, 0, -cy/2, NULL);			// y-Achse ziehen
    LineTo(hdcMem, 0, cy/2);

	hPen2 = CreatePen(PS_DASH, 1, RGB(0,0,200));	// Blau, gestrichelt
	SelectObject(hdcMem,hPen2);

    MoveToEx(hdcMem, Psoll.x, cy/2, NULL);			// Pos.x-Achse ziehen
    LineTo(hdcMem, Psoll.x, -cy/2);

    // pos.y-Achse ziehen
    MoveToEx(hdcMem, -cx/2, Psoll.y, NULL);			// Pos.y-Achse ziehen
    LineTo(hdcMem, cx/2, Psoll.y);

	SelectObject(hdcMem,hSpurPen);						//Spur zeichnen
	MoveToEx(hdcMem,spur[0].x,spur[0].y, NULL);
	Polyline(hdcMem,spur,cspur);
*/
	if(OFlag) DrawItems(hdcMem, Psoll, OFlag);			// Umgebung zeichnen

	SelectObject(hdcMem, hOldPen);
	hBrush1 = CreateSolidBrush(RGB(255,137,0));		//orange
	hBrush3 = CreateSolidBrush(RGB(100,100,100));	//grau
    SelectObject(hdcMem, hBrush1); 					//Pinsel orange 2 holen
	Polygon(hdcMem,aiA2T,4);							//Arm 2 zeichnen
	SelectObject(hdcMem, hBrush1); 					//Pinsel orange holen
	Polygon(hdcMem,aiA1T,4);							// Arm 1 zeichnen
	SelectObject(hdcMem, hBrush3); 					//Pinsel grau holen
	Ellipse(hdcMem,aiA1T[4].x-3,aiA1T[4].y-3,aiA1T[4].x+3,aiA1T[4].y+3); // Gelenkpkt (0,0) zeich
	SelectObject(hdcMem, hBrush3); 					//Pinsel grau holen
	Ellipse(hdcMem,aiA2T[4].x-3,aiA2T[4].y-3,aiA2T[4].x+3,aiA2T[4].y+3); // Gelenkpkte Arm 2 zeichnen
	Ellipse(hdcMem,aiA2T[5].x-3,aiA2T[5].y-3,aiA2T[5].x+3,aiA2T[5].y+3);

	DeleteObject(hBrush1);  //Stife und Pinsel löschen
	DeleteObject(hBrush3);
	DeleteObject(hPen1);
	DeleteObject(hPen2);
	BitBlt(hdc,-cx/2,-cy/2,cx,cy,hdcMem,-cx/2,-cy/2,SRCCOPY);
return Psoll;				// Neue Position zurückgeben
}

POINT2D MoveScaraLin(HDC hdc, POINT2D PZiel, POINT2D PAkt, int velo, HPEN hSpurPen, int OFlag)
// Bewegt den Scara linear von PAkt zu PZiel mit der Geschwingkeit velo
// Zeichnet Spur falls hSpurPen ungleich NULL
// OFlag steuert, ob eine Umgebung mit der Funktion DrawItems gezeichnet werden soll
// Rückgabewert ist neue Position

{
	POINT2D P;					//Zwischenpunkt
	double dx, dy, distance;	//hilfvariablen
	double time;				//Zeit
	int n;						//Anzahl der Schritte
	int i;						//Laufvariablen

	dx = PZiel.x-PAkt.x;			//Delta X
	dy = PZiel.y-PAkt.y;			//Delta Y
	distance = sqrt(dx*dx+dy*dy); 	//Länge
	n = distance/20.+1;				// Anz der Schritte
	time = distance/(double)velo/n*1000;	//Zeit
	P = PAkt;
    for (i = 1 ; i <= n; i++){	//Schleife über die n Schritte
		P.x += dx/(double)n;	//Zwischenpunkte
   		P.y += dy/(double)n;
		PosScara(hdc, P, hSpurPen, OFlag);  //Positionieren
		//sleep((int)time);					//verzögern - POSIX sleep
		Sleep((int)time);					//verzögern
	}
	return P;			//neue Pos zurückgeben
}

POINT2D MoveScaraSpiral(HDC hdc, POINT2D PZiel, POINT2D PAkt, int velo, int uz, HPEN hSpurPen, int OFlag)
// Bewegt den Scara auf einer Spiralförmeigen Bahn von PAkt zu PZiel mit der Geschwingkeit velo
// uz gibt an ob im oder gegen den Uhrzeugersinn (Konstanten UZ und GUZ)
// Zeichnet Spur falls hSpurPen ungleich NULL
// OFlag steuert, ob eine Umgebung mit der Funktion DrawItems gezeichnet werden soll
// Rückgabewert ist neue Position
{
	POINT2D P;				//Zwischenpunkt
	double distance, ralt, rneu, wialt, wineu, dr, dwi, wi; //Hilfsgrößen
	double time;			//Zeit
	int n;					//Anzahl der Schritte
	int i;					//Laufvariablen

	ralt = sqrt(PAkt.x*PAkt.x + PAkt.y*PAkt.y);		//alter Radius
	rneu = sqrt(PZiel.x*PZiel.x + PZiel.y*PZiel.y);	//neuer Radius
	wialt = acos(PAkt.x/ralt);						//alter Winkel
	wineu = acos(PZiel.x/rneu);						//neuer Winkel

	if(PAkt.y < 0) wialt = 2.*PI-wialt;				//ggfs Korrekturen
	if(PZiel.y < 0) wineu = 2.*PI-wineu;

	if(uz == GUZ && wineu < wialt) wineu +=2*PI;	//GUZ
	if(uz == UZ && wineu > wialt) wineu -=2*PI;		//UZ

	distance = ABS((ralt+rneu)/2.*(wialt-wineu));	//Bogenlänge

	n = distance/20.+1;								//Schritte
	time = distance/(double)velo/n*1000;			//Zeit

	P = PAkt;										//Zwischenpunkt
  	dwi = (wineu-wialt)/(double)n;					// Delta Winkel
	dr = (rneu-ralt)/(double)n;						// Delta Radius

	for (i = 1 ; i <= n; i++){						//Über alle Schritte
		wi = wialt+(double)i*dwi;					//Winkel zum Schritt
   		P.x = (ralt+(double)i*dr)*cos(wi);			//Pos zum Schritt
		P.y = (ralt+(double)i*dr)*sin(wi);
		PosScara(hdc, P, hSpurPen, OFlag);			//Positionieren
		//sleep((int)time);							//verzögern - POSIX sleep
		Sleep((int)time);					//verzögern
	}
	return P;										//Rückgabe
}

POINT2D MoveScaraArc(HDC hdc, POINT2D Pm, double radius, double wia , double wie, int velo, HPEN hSpurPen, int OFlag)
// Bewegt den Scara auf einem Kreisbogen mit Mitte PM mit Radius radius vom Anfangswinkel wia zum Endwinkel wie
// Funktion analog AngleArc(), d.h. es wird von der aktuellen Pos aus gestartet
// Geschwingkeit velo
// uz gibt an ob im oder gegen den Uhrzeugersinn (Konstanten UZ und GUZ)
// Zeichnet Spur falls hSpurPen ungleich NULL
// OFlag steuert, ob eine Umgebung mit der Funktion DrawItems gezeichnet werden soll
// Rückgabewert ist neue Position

{
	POINT2D P;					//Zwischenpunkt
	double dwi, wi, distance;	//Hilfsvariablen
	double time;				//Zeit
	int n;						//Schritte
	int i;

	dwi = wie - wia;							// delta Winkel gesamt
	distance = ABS(radius*(wie-wia)*PI/180.);	// Bogenlänge
	n = distance/10.+1;							// Anzahl der Schritte
	time = distance/(double)velo/n*1000;		// Zeit
	dwi = (wie-wia)/(double)n;					// Delta Winkel pro Schritt

	for (i = 0 ; i <= n; i++){					// Über alle Schritt
		wi = wia+(double)i*dwi;					// neuer Winkel
   		P.x = Pm.x+radius*cos(wi*PI/180.);		// neuer Punkt
		P.y = Pm.y+radius*sin(wi*PI/180.);
		PosScara(hdc, P, hSpurPen, OFlag);		// Positionieren
		//sleep((int)time);						// verzögern - POSIX sleep
		Sleep((int)time);					//verzögern
	}
	return P;									// neue Pos
}

POINT2D MoveScaraPoly(HDC hdc, POINT2D *PList, POINT2D PAkt, int nPkte, int velo, HPEN hSpurPen, int OFlag)
// Bewegt den Scara auf einer Polyline (PList mit Anzahl nPkte
// Funktion analog Polyline(), d.h. es wird von der aktuellen Pos aus gestartet
// Geschwingkeit velo
// uz gibt an ob im oder gegen den Uhrzeugersinn (Konstanten UZ und GUZ)
// Zeichnet Spur falls hSpurPen ungleich NULL
// OFlag steuert, ob eine Umgebung mit der Funktion DrawItems gezeichnet werden soll
// Rückgabewert ist neue Position
{
	POINT2D P;	//Zwischenpunkt
	int n;		//Schritte
	int i;		//Laufvariable
	// vom akt. Punkt zum anfangspunkt der Polylinie
	P = MoveScaraLin(hdc, PList[0], PAkt, velo, hSpurPen, OFlag);
	for (i = 0; i < nPkte-1; i++){ //Über alle Schritte
		//Linear bewegen
		P = MoveScaraLin(hdc, PList[i+1], P, velo, hSpurPen, OFlag);
	}
	return P;	//Neue Position
}

