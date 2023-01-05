/*stapler.c*/

#include <windows.h>
#include <math.h>
#include <stdio.h>
#include <mmsystem.h>
#include <time.h>
#include <stdlib.h>

#include "2dlib.h"
#include "scaralib.h"


typedef enum enumObjectStatus {
    WARTEN = 1,  /* #define WARTEN 1 */
    LAUFEN = 2,  /* #define LAUFEN 2 */
    BEWEGEN = 3,  /* #define BEWEGEN 3 */
    KISTE = 4,  /* #define KISTE 4 */
    FLIESSBAND = 5,  /* #define FLIESSBAND 5 */
} ObjectStatus;

typedef enum enumProcSeqThreadState {
    ProcSeqThreadCreated,
    ProcSeqThreadTerminationRequested,
    ProcSeqThreadTerminated
} ProcSeqThreadState;

enum {
    PalettePolygonPoints = 8,
};

typedef struct tagPalletPolygonPoints {
    POINT points[PalettePolygonPoints];
}PalletPolygonPoints;

typedef struct tagObject
{
    POINT P;  // Mittelpunkt
    ObjectStatus status;
} OBJEKT;

/* G L O B A L E   V A R I A B L E N */
int cx, cy;  // Bildschirmgröße
HDC hdcMem;  // Speicher DC
static HBITMAP hBitmap;  // Bitmap
/* Thread variables */
static HANDLE ThreadHandle = NULL;
static int VelocityScara = 400;
static ProcSeqThreadState mThreadState;

//Punkte der Roboterarme; Punkt 5 und 6 geben die Gelenkpunkte der Arme (Kreise) an
// Radius ist in x-Koordinaten des 5ten Punktes versteckt: R1 = 150; R2 = 100

POINT2D aArm1[NPT] = { {-15,15},{350,15},{350,-15},{-15,-15},{0,0},{340,0} };
POINT2D aArm2[NPT] = { {-15,15},{350,10},{350,-10},{-15,-15},{0,0},{340,0} };


static POINT2D pos;  // Soll Position x,y
POINT spur[1000];			//Spur der Bewegung
double R1, R2;					//Radien der Arme

/* Anonymes enum für Array size */
enum {
     /* Anzahl der Objekte. */
     NOBJ = 20, /* #define NOBJ 5 */
};

const int NROLLEN = 4;
const int DR = 70;

static OBJEKT Obj[NOBJ];
POINT PKiste = { -275,525 };
POINT PPick = { -300,200 };


static PalletPolygonPoints CreatePalletPolygonPoints(LONG centerX, LONG centerY, LONG r) {

	PalletPolygonPoints pp = {0};
    pp.points[0].x = centerX - r;
    pp.points[0].y = centerY - r;
    pp.points[1].x = centerX - r + 25;
    pp.points[1].y = centerY - r;
    pp.points[2].x = centerX - r + 25;
    pp.points[2].y = centerY - r + 50;
    pp.points[3].x = centerX + r - 25;
    pp.points[3].y = centerY - r + 50;
    pp.points[4].x = centerX + r - 25;
    pp.points[4].y = centerY - r;
    pp.points[5].x = centerX + r;
    pp.points[5].y = centerY - r;
    pp.points[6].x = centerX + r;
    pp.points[6].y = centerY + r - 175;
    pp.points[7].x = centerX - r;
    pp.points[7].y = centerY + r - 175;
    return pp;
}



int DrawItems(HDC hdc, POINT2D PObj, int Flag) //Objekte, Kisten und Laufband zeichnen
{
    const long r = 125;  //Größe Objekt und Kisten
    enum PenAndBrushItems {
        BrushAndPenSize = 2,
    };
    HBRUSH hBrush[BrushAndPenSize] = {NULL, NULL};  //Pinsel
    HPEN hPen[BrushAndPenSize] = { NULL, NULL };  //Stifte
    POINT P = {0,0};
    int i = 0;
    // Punkte Scara Ständer
    POINT pPtsScaraBase[4] = { -35,40,35,40,50,-60,-50,-60 };

    if (!Flag) {
        return 0;  //Wenn nicht Sequenz => zurück
    }

    hBrush[0] = CreateSolidBrush(RGB(255, 165, 0));		//Pinsel anlegen
    hPen[0] = CreatePen(PS_SOLID, 2, RGB(255, 165, 0));	//Stift anlegen
    hBrush[1] = CreateSolidBrush(RGB(150, 150, 150));
    hPen[1] = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));

    // Hier wird der "Rahmen" Obere Fliessband erzeugt
    SelectObject(hdc, hPen[1]);
    RoundRect(hdc, -600, 400, -417 + NROLLEN * DR, 400 - DR, DR, DR);

    //Hier werden die Rollen des oberen Fliessbands erzeugt
    SelectObject(hdc, hPen[1]);
    SelectObject(hdc, hBrush[1]);
    for (i = 0; i < NROLLEN; i++) {
        Ellipse(hdc, -417 + DR * i, 400, (-417 + DR) + DR * i, 400 - DR);
    }

    //Hier wird der Ständerblock für den Robotter erzeugt
    SelectObject(hdc, hBrush[1]);
    SelectObject(hdc, hPen[1]);
    Polygon(hdc, pPtsScaraBase, 4);

    for (i = 0; i < NOBJ; i++)  //über alle Objekte
    {
        switch (Obj[i].status)  // Position der Objekte anhand des Status ermitteln
        {
            case WARTEN:
                P = Obj[i].P;
                break;
            case LAUFEN:   // Weiter auf Laufband bewegen
                Obj[i].P.y += 1;
                P = Obj[i].P;
                if ((Obj[i].P.y >= 50) && (i + 1 < NOBJ)) {
                    Obj[i + 1].status = LAUFEN;
                }
                break;
            case BEWEGEN:  // am Greifer
                P = Pint(PObj);
                break;
            case KISTE: //in Kiste
                P = PKiste;
            case FLIESSBAND:
                Obj[i].P.x -= 5;
                P = Obj[i].P;
                break;
            default:
                continue;  /* P not initialized. weiter mit next iteration. */
                break;
        }

        SelectObject(hdc, hPen[1]);  // Schwarzen Stift aktivieren
        SelectObject(hdc, hBrush[0]);  // Pinsel aktivieren
        PalletPolygonPoints polygon = CreatePalletPolygonPoints(P.x, P.y, r);
        //Rectangle(hdc, (int)(P.x - r), (int)(P.y - r), (int)(P.x + r), (int)(P.y + r));	//Objekt zeichnen
        Polygon(hdc, polygon.points, PalettePolygonPoints);
    }

    for (i = 0; i < BrushAndPenSize; i++) {  //Stifte und Pinsel löschen
        DeleteObject(hPen[i]);
        DeleteObject(hBrush[i]);
    }

    return 0;
}

// Thread Funktion
DWORD WINAPI ProcSeq(LPVOID lphwnd)
{
    HANDLE hwnd = (HWND)lphwnd;  /* we expect that the windohandle isprovided with lphwnd */
    HDC hdc;
    HPEN hSpPen;
    int i = 0;
    POINT2D posAlt = {-300 , 300};

	const POINT mitte1 = {0 , 425};
	const POINT mitte2 = {0 , 375};

    const POINT P0 = { -300, 250 };
    const POINT P1 = { 0 , 250 };

    //const POINT P2 = { 0 , 600 };
    const POINT P3 = { -275, 600 };

    const POINT P4 = { 0, 525 };
    const POINT P5 = { 0, 225 };

    /* Objekte initialisieren */
    for (i = 0; i < NOBJ; i++) {
        Obj[i].P.x = -290;
        Obj[i].P.y = -25;
        Obj[i].status = WARTEN;  /* Status warten */
	    }


    hdc = GetDC(hwnd);
    hSpPen = CreatePen(PS_DOT, 1, RGB(200, 0, 0));

	Obj[0].status = LAUFEN;

    // ueber alle Objekte iterieren bis fertig oder Terminierung angefragt
    for (i = 0; i < NOBJ && (mThreadState != ProcSeqThreadTerminationRequested); i++) {

        //nächstes Objekt losschicken
        if ((Obj[i].P.y <= 0) && (i + 1 < NOBJ)) {
            Obj[i+1].status = LAUFEN;
        }

        //PPick.y = Obj[i].P.y;  // PickPosition in Y fixieren

        pos = MoveScaraLin(hdc, Pdouble(PPick), pos, VelocityScara, hSpPen, TRUE);	//PickPos anfahren
        pos = MoveScaraLin(hdc, Pdouble(Obj[i].P), pos, VelocityScara, hSpPen, TRUE);	//greifen


        posAlt = pos;
        Obj[i].status = BEWEGEN;  //Status

        pos = MoveScaraLin(hdc, Pdouble(P0), pos, VelocityScara, hSpPen, TRUE);
        pos = MoveScaraLin(hdc, Pdouble(P1), pos, VelocityScara, hSpPen, TRUE);
        pos = MoveScaraArc(hdc, Pdouble(mitte1), 175, -90, 90, VelocityScara, hSpPen, TRUE);
        pos = MoveScaraLin(hdc, Pdouble(P3), pos, VelocityScara, hSpPen, TRUE);

	    pos = MoveScaraLin(hdc, Pdouble(PKiste), pos, VelocityScara, hSpPen, TRUE);

        Obj[i].status = KISTE; //Status
        Obj[i].P.x = -275;
        Obj[i].P.y = 525;
        Obj[i].status = FLIESSBAND;

        pos = MoveScaraLin(hdc, Pdouble(P4), pos, VelocityScara, hSpPen, TRUE);
        pos = MoveScaraArc(hdc, Pdouble(mitte2), 150, 90, -90, VelocityScara, hSpPen, TRUE);
        pos = MoveScaraLin(hdc, Pdouble(P5), pos, VelocityScara, hSpPen, TRUE);

    }
    posAlt = pos;
    ReleaseDC(hwnd, hdc);
    return 0;
}

static void IncreaseVelocity() {
    if (VelocityScara < 3000) {
        VelocityScara += 100;
    }
}

static void ReduceVelocity() {
    if (VelocityScara > 100) {
        VelocityScara -= 100;
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;
    TCHAR szText[80] = "";  //  Textpuffer
    HPEN hSpPen;
    POINT2D P;
    P.x = 0;
    P.y = -180;

    switch (message)  //Nachrichtenschleife
    {
    case WM_SIZE:  //Bildschirmgröße
        cx = LOWORD(lParam);
        cy = HIWORD(lParam)+550;			//mit dem +550 wird das ganze Koordinatensystem um 550 in + y Richtung erweitert
        hdc = GetDC(hwnd);
        if (hBitmap)DeleteObject(hBitmap);	//Bitmap Löschen falls schon vorhanden
        hBitmap = CreateCompatibleBitmap(hdc, cx, cy);	//Neues Bitmap
        hdcMem = CreateCompatibleDC(hdc);	//Speicher DC Anlagen
        ReleaseDC(hwnd, hdc);
        SelectObject(hdcMem, hBitmap);		// Bitmap selektieren
        return 0;

    case WM_CREATE:						//Initialisierungen
        R1 = aArm1[5].x;					//Radius R1
        R2 = aArm2[5].x;					//Radius für R2
        double rmax = aArm1[5].x + aArm2[5].x;
        double rmin = aArm1[5].x - aArm2[5].x;
        pos.x = 100;						//Soll Pos x
        pos.y = 120;						//Soll Pos y
        double scale = 1.0;
        spur[0] = Pint(pos);
        spur[1] = Pint(pos);  // Warum 2x pos?
        int cspur = 2;
        Initialize(rmax, rmin, scale, cspur);
        return 0;

    case WM_PAINT:
        hdc = BeginPaint(hwnd, &ps);
        hSpPen = CreatePen(PS_SOLID, 2, RGB(200, 0, 0));
        pos = PosScara(hdc, pos, hSpPen, 1);
        EndPaint(hwnd, &ps);
        sprintf(szText, "Stapler - Pos.: x = %3.2lf, y = %3.2lf", pos.x, pos.y);
        SetWindowText(hwnd, szText);
        return 0;

    case WM_KEYDOWN:  //Steuerung.
        switch (wParam)
        {
        case VK_UP:  IncreaseVelocity(); return 0;
        case VK_DOWN:  ReduceVelocity(); return 0;
        case VK_CONTROL: return 0;
        case 'A':
            if (ThreadHandle == NULL) {
                ThreadHandle = CreateThread(NULL, 0, ProcSeq, (LPVOID)hwnd, 0, NULL);
                mThreadState = ProcSeqThreadCreated;
            }
            break;
        case 'S':
            if (ThreadHandle != NULL) {
                SuspendThread(ThreadHandle);
            }
            return 0;
        case 'R':
            if (ThreadHandle != NULL) {
                ResumeThread(ThreadHandle);
            }
            return 0;
        case 'T':
            if (ThreadHandle != NULL) {
                mThreadState = ProcSeqThreadTerminationRequested;
                Sleep(10);
                TerminateThread(ThreadHandle, 0);  // Warnung: Funktion ist gefaehrlich
                CloseHandle(ThreadHandle);
                ThreadHandle = NULL;
            }
        }
        InvalidateRect(hwnd, NULL, TRUE);  // Neu Zeichnen
        return 0;

    case WM_DESTROY:
        DeleteDC(hdcMem);
        DeleteObject(hBitmap);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hwnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hI, HINSTANCE hPrI, PSTR szCmdLine, int iCmdShow)
{
    (void)hPrI;  /* ignored parameter hI and suppress the warning*/
    (void)szCmdLine;  /* ignored parameter hPrI and suppress the warning */
    static TCHAR szAppName[] = TEXT("Fensterklasse");
    HWND hwnd;

    WNDCLASS wc;
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hI;
    wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = szAppName;
    RegisterClass(&wc);

    hwnd = CreateWindow(szAppName, TEXT("Stapler"), WS_OVERLAPPEDWINDOW,
        0, 0, 850, 1000, NULL, NULL, hI, NULL);

    ShowWindow(hwnd, iCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return msg.wParam;
}
