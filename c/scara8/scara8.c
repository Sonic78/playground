/*scara8.c*/

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


#define NOBJ 10
#define NKISTE 10


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
POINT PKiste[NKISTE] = {{-300,0},{-250,130},{-160,225},{-70,300},{70,300},{160,225},{250,130},{300,0}, {0,-370},{0,370}};
POINT PPick = {100,-250};

/* THREADING */
HANDLE ThreadHandle = NULL;
int VeloBand = 2;
int VeloScara = 1200;
int BandCol = 150;
int r = 20;					//Größe Objekt und Kisten

/*Prototyp*/
int DrawItems(HDC hdc, POINT2D PObj, int Flag);  //Objekte, Kisten und Laufband zeichnen

#include "scaralib.c"



int DrawItems(HDC hdc, POINT2D PObj, int Flag) //Objekte, Kisten und Laufband zeichnen
{

HBRUSH hBrush[NKISTE], hBrushBand, hOldBrush;	//Pinsel
HPEN hPen[NKISTE], hPenBand, hOldPen;		//Stifte
POINT P;
int i ;
static int ursprung = 0;

if (!Flag) return 0;			//Wenn nicht Sequenz => zurück

hBrush[0] = CreateSolidBrush(RGB(255,0,0));		//Pinsel anlegen
hPen[0] = CreatePen(PS_SOLID,2,RGB(255,0,0));	//Stift anlegen

hBrush[1] = CreateSolidBrush(RGB(0,255,0));
hPen[1] = CreatePen(PS_SOLID,2,RGB(0,255,0));

hBrush[2] = CreateSolidBrush(RGB(0,0,225));
hPen[2] = CreatePen(PS_SOLID,2,RGB(0,0,255));

hBrush[3] = CreateSolidBrush(RGB(230,230,0));
hPen[3] = CreatePen(PS_SOLID,2,RGB(230,230,0));

hBrush[4] = CreateSolidBrush(RGB(255,180,0));
hPen[4] = CreatePen(PS_SOLID,2,RGB(255,180,0));

hBrush[5] = CreateSolidBrush(RGB(180,0,255));
hPen[5] = CreatePen(PS_SOLID,2,RGB(180,0,255));

hBrush[6] = CreateSolidBrush(RGB(255,0,255));
hPen[6] = CreatePen(PS_SOLID,2,RGB(255,0,255));

hBrush[7] = CreateSolidBrush(RGB(0,255,255));
hPen[7] = CreatePen(PS_SOLID,2,RGB(0,255,255));

hBrush[8] = CreateSolidBrush(RGB(0,0,0));
hPen[8] = CreatePen(PS_SOLID,2,RGB(0,0,0));

hBrush[9] = CreateSolidBrush(RGB(200,100,0));
hPen[9] = CreatePen(PS_SOLID,2,RGB(200,100,0));

hPenBand = CreatePen(PS_SOLID,2,RGB(100,100,100));
hBrushBand = CreateHatchBrush(HS_VERTICAL, RGB(100,100,100));

SetBkMode(hdc, OPAQUE);
SetBkColor(hdc, RGB(BandCol,BandCol,BandCol));
hOldPen = SelectObject(hdc, hPenBand);
hOldBrush = SelectObject(hdc, hBrushBand);
ursprung += VeloBand;
SetBrushOrgEx(hdc, ursprung, 0, NULL);
Rectangle(hdc,-cx/2,-150,cx/2,-340);	//Laufband zeichnen

SelectObject(hdc, hOldBrush);

for (i = 0; i < NKISTE; i++){		//Kisten zeichnen
	SelectObject(hdc, hPen[i]);

	Rectangle(hdc,PKiste[i].x-1.2*(double)r,PKiste[i].y-1.2*(double)r,PKiste[i].x+1.2*(double)r,PKiste[i].y+1.2*(double)r);
}

for(i = 0; i < NOBJ; i++)	//über alle Objekte
	{
		switch(Obj[i].status)	//Position der Objekte anhand des Status ermitteln
		{
			case WARTEN: 	P = Obj[i].P; break;
			case LAUFEN:	Obj[i].P.x += VeloBand; P = Obj[i].P; break;  //Weiter auf Laufband bewegen
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
DeleteObject(hPenBand);
DeleteObject(hBrushBand);
DeleteObject(hOldBrush);
SelectObject(hdc, hOldPen);

return 0;
}


DWORD WINAPI ProcSeq(LPVOID lphwnd)  	// Thread Procedure
{
	HDC hdc;
	HBRUSH hSpPen;
	HANDLE hwnd = (HWND)lphwnd;			// Parameter kovertieren
	int i;
	TCHAR szText[80];
	srand(time(NULL));					//Zufallsgenerator setzen
	for (i = 0; i < NOBJ; i++){			//Objekte erzeugen
		Obj[i].P.x = -500;
		Obj[i].P.y = -170-rand()%150;	//zufällige Y Position
		Obj[i].typ = rand()%NKISTE;		//zufälliger Typ (Farbe)
		Obj[i].status = WARTEN;			//Status warten
}
	hdc = GetDC(hwnd);
	//hSpPen = CreatePen(PS_DOT,1,RGB(200,0,0));

	Obj[0].status = LAUFEN;				//Erstes Obj losschicken
	pos = MoveScaraSpiral(hdc, Pdouble(PKiste[0]), pos, VeloScara,UZ, NULL, TRUE);
	Obj[1].status = LAUFEN;				//Zweites Obj losschicken
	pos = MoveScaraSpiral(hdc, Pdouble(PKiste[1]), pos, VeloScara,UZ, NULL, TRUE);
	pos = MoveScaraSpiral(hdc, Pdouble(PKiste[2]), pos, VeloScara,UZ, NULL, TRUE);
	Obj[2].status = LAUFEN;				//Drittes Obj losschicken
	pos = MoveScaraSpiral(hdc, Pdouble(PKiste[3]), pos, VeloScara,UZ, NULL, TRUE);
	pos = MoveScaraSpiral(hdc, Pdouble(PKiste[4]), pos, VeloScara,UZ, NULL, TRUE);
	pos = MoveScaraSpiral(hdc, Pdouble(PKiste[5]), pos, VeloScara,UZ, NULL, TRUE);
	pos = MoveScaraSpiral(hdc, Pdouble(PKiste[6]), pos, VeloScara,UZ, NULL, TRUE);
	pos = MoveScaraSpiral(hdc, Pdouble(PKiste[7]), pos, VeloScara,UZ, NULL, TRUE);

	for(i = 0; i < NOBJ; i++){			//über alle Objekte
		if (NOBJ > i+3) Obj[i+3].status = LAUFEN;	//nächstes Objekt losschicken
		PPick.y = Obj[i].P.y;			//PickPosition in Y fixieren
		pos = MoveScaraLin(hdc, Pdouble(PPick), pos, VeloScara, NULL, TRUE);	//PickPos anfahren
		pos = MoveScaraLin(hdc, Pdouble(Obj[i].P), pos, VeloScara, NULL, TRUE);	//greifen
		PlaySound("click.wav", NULL, SND_ASYNC);
		posAlt = pos;
		Obj[i].status = BEWEGEN;				//Status
		pos = MoveScaraLin(hdc, Pdouble(PKiste[Obj[i].typ]), pos, VeloScara, NULL, TRUE); //zur Kiste
		PlaySound("click.wav", NULL, SND_ASYNC);
		Obj[i].status = KISTE;					//Status
		sprintf(szText, "Scara8 - VeloScara = %i, VeloBand = %i",VeloScara, VeloBand);
		SetWindowText(hwnd, szText);

	}
	posAlt = pos;
	ReleaseDC(hwnd, hdc);
	ThreadHandle = NULL;
	
	return 0;
}

LRESULT CALLBACK WndProc( HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam )
{
       	HDC hdc;
 	   	PAINTSTRUCT ps;
		TCHAR szText[80];						//Textpuffer
		HPEN hSpPen;
		int i;
		double wi, r0, r = 0;
		POINT2D P;
		static char ReDraw = TRUE;

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
			ReDraw = TRUE;
		 return 0;

        case WM_PAINT:
			hdc = BeginPaint(hwnd, &ps);
			hSpPen = CreatePen(PS_SOLID,2,RGB(200,0,0));
			//pos = PosScara(hdc, pos, hSpPen, 0);

			EndPaint(hwnd, hdc);
			sprintf(szText, "Scara8 - VeloScara = %i, VeloBand = %i",VeloScara, VeloBand);
			SetWindowText(hwnd, szText);
		return 0;

		case WM_KEYDOWN:		//Steuerung.
			switch(wParam)
			{

				case 'A':	if( ThreadHandle == NULL)
								ThreadHandle = CreateThread(NULL, 0, ProcSeq, (LPVOID)hwnd , 0, NULL);
							break;
				case 'S':  	SuspendThread(ThreadHandle);ReDraw = FALSE; break;
				case 'R':  	ResumeThread(ThreadHandle);ReDraw = TRUE; break;
				case 'T':	TerminateThread(ThreadHandle, 0); CloseHandle(ThreadHandle);break;
				case VK_UP: 		if(VeloBand < 5) VeloBand++; break;
				case VK_DOWN:		if(VeloBand > 1) VeloBand--; break;
				case VK_RIGHT: 		if(VeloScara < 5000) VeloScara += 100; break;
				case VK_LEFT:		if(VeloScara > 200) VeloScara -= 100; break;
				case VK_PRIOR: 		if(BandCol < 240) BandCol += 10; break;
				case VK_NEXT:		if(BandCol >= 10) BandCol -= 10; break;
				case VK_ADD: 		if(r < 30) r++; break;
				case VK_SUBTRACT:	if(r >= 10) r--; break;

			}
			sprintf(szText, "Scara8 - VeloScara = %i, VeloBand = %i",VeloScara, VeloBand);
			SetWindowText(hwnd, szText);
			if(ReDraw) InvalidateRect(hwnd, NULL, TRUE);									//Neu Zeichnen
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

     hwnd = CreateWindow (szAppName, TEXT("Scara 8"), WS_OVERLAPPEDWINDOW,
                          0, 0, 850, 850, NULL, NULL, hI, NULL);

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
