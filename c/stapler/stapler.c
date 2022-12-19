/*scara7.c*/

#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <mmsystem.h>
#include <time.h>
#include <stdlib.h>

#define X 1
#define Y 2
#define XY 3
#define ROT 3
#define DIM 3						// Dimension//
#define PI 3.14159265
#define ABS(x) ((x)<0?-(x):(x))		// Betragmakro
#define NPT 6						//Anzahl der Punkte


#define NOBJ 5
#define NKISTE 1
#define NROLLEN 4
#define DR 70

#define VELO 900

#define WARTEN 1
#define LAUFEN 2
#define BEWEGEN 3
#define KISTE 4

#include "2dlib.c"


typedef struct tagObject
{
	POINT P;
	int typ;
	int status;
} OBJEKT;


/* G L O B A L E   V A R I A B L E N */
int cx,cy; // Bildschirmgröße
HDC hdcMem;		//Speicher DC
HBITMAP hBitmap;// Bitmap

//Punkte der Roboterarme; Punkt 5 und 6 geben die Gelenkpunkte der Arme (Kreise) an
// Radius ist in x-Koordinaten des 5ten Punktes versteckt: R1 = 150; R2 = 100
POINT2D aArm1[NPT] = {{-15,15},{210,15},{210,-15},{-15,-15},{0,0},{200,0}};
POINT2D aArm2[NPT] = {{-15,15},{210,10},{210,-10},{-15,-15},{0,0},{200,0}};

POINT2D aA1T[NPT],aA2T[NPT],aA2T1[NPT]; 	//Transformierte Punkte
POINT aiA1T[NPT], aiA2T[NPT];		//Transformierte Punkte int für Polygon
POINT2D pos, posAlt;			//Soll Position x,y
POINT   spur[1000];			//Spur der Bewegung
double scale;							//Scalierungsfaktor
MATRIX2D mRotA1,mRotA2,mTransA2; // Rot- und Trans Matrizen
int cspur;								//Spurpunkte zählen
double R1, R2;					//Radien der Arme
int Rmax, Rmin;							//Arbeitsraum
double alpha, beta;				//Rotationswinkel A1 und A2
POINT2D pDemo[360];
OBJEKT Obj[NOBJ];

POINT PKiste[NKISTE] = {-200,250};
POINT PPick = {-300,-100};

/*Prototyp*/
int DrawItems(HDC hdc, POINT2D PObj, int Flag);  //Objekte, Kisten und Laufband zeichnen

#include "scaralib.c"

int DrawItems(HDC hdc, POINT2D PObj, int Flag) //Objekte, Kisten und Laufband zeichnen
{
double r = 20;					//Größe Objekt und Kisten
HBRUSH hBrush[NKISTE];			//Pinsel
HPEN hPen[NKISTE], hPenBand;	//Stifte
POINT P;
int i ;

if (!Flag) return 0;			//Wenn nicht Sequenz => zurück

hBrush[0] = CreateSolidBrush(RGB(255,165,0));		//Pinsel anlegen
hPen[0] = CreatePen(PS_SOLID,2,RGB(255,165,0));	//Stift anlegen

hBrush[1] = CreateSolidBrush(RGB(150,150,150));
hPen[1] = CreatePen(PS_SOLID,2,RGB(0,0,0));


SelectObject(hdc,hPen[1]);
RoundRect(hdc,-600,200,-400+NROLLEN*DR,200-DR,DR,DR);
//Polyline(hdc,

SelectObject(hdc,hPen[1]);
SelectObject(hdc,hBrush[1]);

for (i = 0; i < NROLLEN; i++){
Ellipse(hdc,-400+DR*i,200,(-400+DR)+DR*i,200-DR);
}

for(i = 0; i < NOBJ; i++)	//über alle Objekte
	{
		switch(Obj[i].status)	//Position der Objekte anhand des Status ermitteln
		{
			case WARTEN: 	P = Obj[i].P; break;
			case LAUFEN:	Obj[i].P.y += 2; P = Obj[i].P; break;  //Weiter auf Laufband bewegen
			case BEWEGEN: 	P = Pint(PObj);  break;				   // am Greifer
			case KISTE: 	P = PKiste[Obj[i].typ];					//in Kiste
		}
		SelectObject(hdc, hPen[Obj[i].typ]);						//Stift aktivieren
		SelectObject(hdc, hBrush[Obj[i].typ]);						//Pinsel aktivieren
		Ellipse(hdc,(int)P.x-r,(int)P.y-r,(int)P.x+r,(int)P.y+r);	//Objekt zeichnen
}//for
for(i = 0; i< NKISTE; i++){		//Stifte und Pinsel löschen
	DeleteObject(hPen[i]);
	DeleteObject(hBrush[i]);
}
//DeleteObject(hPenBand);
return 0;
}


POINT2D ProcSeq(HWND hwnd)
{
	HDC hdc;
	HBRUSH hSpPen;
	int i;

	for (i = 0; i < NOBJ; i++){		//Objekte erzeugen
		Obj[i].P.x = -300;
		Obj[i].P.y = -500;
		Obj[i].status = WARTEN;			//Status warten
}
	hdc = GetDC(hwnd);
	hSpPen = CreatePen(PS_DOT,1,RGB(200,0,0));

	Obj[0].status = LAUFEN;				//Erstes Obj losschicken
	pos = MoveScaraSpiral(hdc, Pdouble(PKiste[0]), pos, 1000,UZ, NULL, TRUE);
	/*Obj[1].status = LAUFEN;				//Zweites Obj losschicken
	pos = MoveScaraSpiral(hdc, Pdouble(PKiste[1]), pos, 1000,UZ, NULL, TRUE);
	pos = MoveScaraSpiral(hdc, Pdouble(PKiste[2]), pos, 1000,UZ, NULL, TRUE);
	Obj[2].status = LAUFEN;				//Drittes Obj losschicken
	pos = MoveScaraSpiral(hdc, Pdouble(PKiste[3]), pos, 1000,UZ, NULL, TRUE);
	pos = MoveScaraSpiral(hdc, Pdouble(PKiste[4]), pos, 1000,UZ, NULL, TRUE);
	pos = MoveScaraSpiral(hdc, Pdouble(PKiste[5]), pos, 1000,UZ, NULL, TRUE);
	pos = MoveScaraSpiral(hdc, Pdouble(PKiste[6]), pos, 1000,UZ, NULL, TRUE);
	pos = MoveScaraSpiral(hdc, Pdouble(PKiste[7]), pos, 1000,UZ, NULL, TRUE);*/

	for(i = 0; i < NOBJ; i++){			//über alle Objekte
		if (NOBJ > i+1) Obj[i+1].status = LAUFEN;	//nächstes Objekt losschicken
		PPick.y = Obj[i].P.y;			//PickPosition in Y fixieren
		pos = MoveScaraLin(hdc, Pdouble(PPick), pos, 1000,hSpPen, TRUE);	//PickPos anfahren
		pos = MoveScaraLin(hdc, Pdouble(Obj[i].P), pos, 1000,hSpPen, TRUE);	//greifen
		PlaySound("click.wav", NULL, SND_ASYNC);
		posAlt = pos;
		Obj[i].status = BEWEGEN;				//Status
		pos = MoveScaraLin(hdc, Pdouble(PKiste[Obj[i].typ]), pos, 1000, hSpPen, TRUE); //zur Kiste
		PlaySound("click.wav", NULL, SND_ASYNC);
		Obj[i].status = KISTE;					//Status

	}
	posAlt = pos;
	ReleaseDC(hwnd, hdc);
	return pos;
}

LRESULT CALLBACK WndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
       	HDC hdc;
 	   	PAINTSTRUCT ps;
		TCHAR szText[80];						//Textpuffer
		HPEN hSpPen;
		int i;
		double wi, r0, r;
		POINT2D P;


      	switch(message)							//Nachrichtenschleife
       	{
         case WM_SIZE:							//Bildschirmgröße
			 cx = LOWORD(lParam);
      		 cy = HIWORD(lParam);
			 hdc = GetDC(hwnd);
			 if(hBitmap)DeleteObject(hBitmap);	//Bitmap Löschen falls schon vorhanden
			 hBitmap = CreateCompatibleBitmap(hdc, cx, cy);	//Neues Bitmap
			 hdcMem = CreateCompatibleDC(hdc);	//Speicher DC Anlagen
			 ReleaseDC(hwnd,hdc);
			 SelectObject(hdcMem, hBitmap);		// Bitmap selektieren
		return 0;

		 case WM_CREATE:						//Initialisierungen
			R1 = aArm1[5].x;					//Radius R1
			R2 = aArm2[5].x;					//Radius für R2
			Rmax = aArm1[5].x+aArm2[5].x;
			Rmin = aArm1[5].x-aArm2[5].x;
			pos.x = 100;						//Soll Pos x
			pos.y = 120;						//Soll Pos y
			scale = 1.0;
			spur[0] = Pint(pos);
			spur[1] = Pint(pos);
			cspur = 2;
		 return 0;

        case WM_PAINT:
			hdc = BeginPaint(hwnd, &ps);
			hSpPen = CreatePen(PS_SOLID,2,RGB(200,0,0));
			pos = PosScara(hdc, pos, hSpPen, 0);
			EndPaint(hwnd, hdc);
			sprintf(szText, "Scara7 - Pos.: x = %3.2lf, y = %3.2lf",pos.x, pos.y);
			SetWindowText(hwnd, szText);
		return 0;

		case WM_KEYDOWN:		//Steuerung.
			switch(wParam)
			{
				case VK_LEFT: 	 pos.x -= 6.; break;	//Position verändern
				case VK_RIGHT: 	 pos.x += 6.; break;
				case VK_UP: 	 pos.y += 6.; break;
				case VK_DOWN: 	 pos.y -= 6.; break;
				case VK_PRIOR: 	 scale *=1.1; break;
				case VK_NEXT:	 scale /=1.1; break;
				case VK_CONTROL: return 0;
				case 'A':	ProcSeq(hwnd);return 0;
		}
		InvalidateRect(hwnd, NULL, TRUE);					//Neu Zeichnen

		return 0;

		case WM_LBUTTONDOWN:
			posAlt = pos;
			pos.x = LOWORD(lParam)-cx/2;
			pos.y = cy/2-HIWORD(lParam);
			hdc = GetDC(hwnd);
			hSpPen = CreatePen(PS_SOLID,2,RGB(200,0,0));
			pos = MoveScaraLin(hdc,pos,posAlt,1000,hSpPen,0);
			sprintf(szText, "Scara7 - Pos.: x = %3.2lf, y = %3.2lf",pos.x, pos.y);
			SetWindowText(hwnd, szText);
			ReleaseDC(hwnd, hdc);
		return 0;

		case WM_RBUTTONDOWN:
			posAlt = pos;
			pos.x = LOWORD(lParam)-cx/2;
			pos.y = cy/2-HIWORD(lParam);
			hdc = GetDC(hwnd);
			hSpPen = CreatePen(PS_SOLID,2,RGB(200,0,0));

			if (wParam & MK_CONTROL)
				pos = MoveScaraSpiral(hdc,pos,posAlt,1000,UZ,hSpPen,0);
			else
				pos = MoveScaraSpiral(hdc,pos,posAlt,1000,GUZ, hSpPen,0);

			sprintf(szText, "Scara7 - Pos.: x = %3.2lf, y = %3.2lf",pos.x, pos.y);
			SetWindowText(hwnd, szText);
			ReleaseDC(hwnd, hdc);
		return 0;

		case WM_MBUTTONDOWN:
			posAlt = pos;
			pos.x = LOWORD(lParam)-cx/2;
			pos.y = cy/2-HIWORD(lParam);
			hdc = GetDC(hwnd);
			hSpPen = CreatePen(PS_SOLID,2,RGB(200,0,0));
			pos = MoveScaraArc(hdc,pos,100,-45,225,200,hSpPen,0);
			sprintf(szText, "Scara7 - Pos.: x = %3.2lf, y = %3.2lf",pos.x, pos.y);
			SetWindowText(hwnd, szText);
			ReleaseDC(hwnd, hdc);
		return 0;

		case WM_DESTROY:
		 	DeleteDC(hdcMem);
		  	DeleteObject(hBitmap);
          	PostQuitMessage(0);
        return 0;
       }
       return DefWindowProc( hwnd, message, wParam, lParam );
}

int WINAPI WinMain( HINSTANCE hI, HINSTANCE hPrI, PSTR szCmdLine, int iCmdShow )
{
     static TCHAR szAppName[] = TEXT("Fensterklasse");
     HWND hwnd;

     WNDCLASS wc;
     wc.style         = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
     wc.lpfnWndProc   = WndProc;
     wc.cbClsExtra    = 0;
     wc.cbWndExtra    = 0;
     wc.hInstance     = hI;
     wc.hIcon         = LoadIcon (NULL, IDI_WINLOGO);
     wc.hCursor       = LoadCursor (NULL, IDC_ARROW);
     wc.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH);
     wc.lpszMenuName  = NULL;
     wc.lpszClassName = szAppName;
     RegisterClass (&wc);

     hwnd = CreateWindow (szAppName, TEXT("Scara 7"), WS_OVERLAPPEDWINDOW,
                          0, 0, 850, 1000, NULL, NULL, hI, NULL);

     ShowWindow  ( hwnd, iCmdShow );
     UpdateWindow( hwnd);

     MSG msg;
     while( GetMessage( &msg, NULL, 0, 0) )
     {
        TranslateMessage( &msg );
        DispatchMessage ( &msg );
     }
     return msg.wParam;
}
