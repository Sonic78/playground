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
static int VelocityScara = 300;
static ProcSeqThreadState mThreadState;

//Punkte der Roboterarme; Punkt 5 und 6 geben die Gelenkpunkte der Arme (Kreise) an
// Radius ist in x-Koordinaten des 5ten Punktes versteckt: R1 = 150; R2 = 100
POINT2D aArm1[NPT] = { {-15,15},{210,15},{210,-15},{-15,-15},{0,0},{200,0} };
POINT2D aArm2[NPT] = { {-15,15},{210,10},{210,-10},{-15,-15},{0,0},{200,0} };

static POINT2D pos;  // Soll Position x,y
POINT spur[1000];			//Spur der Bewegung
double R1, R2;					//Radien der Arme

/* Anonymes enum für Array size */
enum {
     /* Anzahl der Objekte. */
     NOBJ = 5, /* #define NOBJ 5 */
};

const int NROLLEN = 4;
const int DR = 70;

static OBJEKT Obj[NOBJ];
POINT PKiste = { -200,250 };
POINT PPick = { -300,100 };


// Translation of a Polygon with 8 points
static void MovePalette(POINT pPointsIn[PalettePolygonPoints], POINT pPointsOut[PalettePolygonPoints], double tx, double ty) {
    int i = 0;
    // Copy input polygon points (type POINT) to local matrix that uses POINT2D
    POINT2D polygonPoints[PalettePolygonPoints] = {0};
    for (i = 0; i < PalettePolygonPoints; ++i) {
        polygonPoints[i].x = pPointsIn[i].x;
        polygonPoints[i].y = pPointsIn[i].y;
    }

    // Translationsmatrix
    MATRIX2D translationMx = InitTransMat(tx, ty);
    for (i = 0; i < 4; i++) {
        polygonPoints[i] = MxV(translationMx, polygonPoints[i]);
    }

    // copy transated/moved points to pPointsOut
    for (i = 0; i < 4; ++i) {
        pPointsOut[i].x = (int) polygonPoints[i].x;
        pPointsOut[i].y = (int) polygonPoints[i].y;
    }
}

PalletPolygonPoints CreatePalletPolygonPoints(LONG centerX, LONG centerY, LONG r) {
    PalletPolygonPoints points = { centerX - r,centerY - r,
                                   centerX - r + 15,centerY - r,
                                   centerX - r + 15,centerY - r + 50,
                                   centerX + r - 15,centerY - r + 50,
                                   centerX + r - 15,centerY - r,
                                   centerX + r,centerY - r,
                                   centerX + r, centerY + r,
                                   centerX - r, centerY + r};

    return points;
}


int DrawItems(HDC hdc, POINT2D PObj, int Flag) //Objekte, Kisten und Laufband zeichnen
{
    const double r = 50;  //Größe Objekt und Kisten
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
    RoundRect(hdc, -600, 200, -400 + NROLLEN * DR, 200 - DR, DR, DR);

    //Hier werden die Rollen des oberen Fliessbands erzeugt
    SelectObject(hdc, hPen[1]);
    SelectObject(hdc, hBrush[1]);
    for (i = 0; i < NROLLEN; i++) {
        Ellipse(hdc, -400 + DR * i, 200, (-400 + DR) + DR * i, 200 - DR);
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
    }//for

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
    POINT2D posAlt = {0.0 , 0.0};

    const POINT P0 = { -300, 125 };
    const POINT P1 = { 0, 125 };
    //const POINT P2 = {0, 300};
    const POINT P3 = { -200, 300 };
    const POINT P4 = { 0, 250 };
    //const POINT P5 = { 0,100 };
    const POINT P6 = { -300, 50 };

    const POINT Pm1 = { 0, 212.5 };
    const POINT Pm2 = { 0, 175 };

    /* Objekte initialisieren */
    for (i = 0; i < NOBJ; i++) {
        Obj[i].P.x = -300;
        Obj[i].P.y = -300;
        Obj[i].status = WARTEN;  /* Status warten */
    }
    hdc = GetDC(hwnd);
    hSpPen = CreatePen(PS_DOT, 1, RGB(200, 0, 0));

    Obj[0].status = LAUFEN;  //Erstes Obj losschicken

    // ueber alle Objekte iterieren bis fertig oder Terminierung angefragt
    for (i = 0; i < NOBJ && (mThreadState != ProcSeqThreadTerminationRequested); i++) {
        if (NOBJ > i + 1) { 
            Obj[i + 1].status = LAUFEN;
        }
        //nächstes Objekt losschicken
        PPick.y = Obj[i].P.y;  // PickPosition in Y fixieren
        pos = MoveScaraLin(hdc, Pdouble(PPick), pos, VelocityScara, hSpPen, TRUE);	//PickPos anfahren
        pos = MoveScaraLin(hdc, Pdouble(Obj[i].P), pos, VelocityScara, hSpPen, TRUE);	//greifen

        //PlaySound("click.wav", NULL, SND_ASYNC);
        posAlt = pos;
        Obj[i].status = BEWEGEN;  //Status

        pos = MoveScaraLin(hdc, Pdouble(P0), pos, VelocityScara, hSpPen, TRUE);
        pos = MoveScaraLin(hdc, Pdouble(P1), pos, VelocityScara, hSpPen, TRUE);
        pos = MoveScaraArc(hdc, Pdouble(Pm1), 87.5, -90, 90, VelocityScara, hSpPen, TRUE);
        pos = MoveScaraLin(hdc, Pdouble(P3), pos, VelocityScara, hSpPen, TRUE);

        pos = MoveScaraLin(hdc, Pdouble(PKiste), pos, (VelocityScara + 700), hSpPen, TRUE);

        Obj[i].status = KISTE; //Status
        Obj[i].P.x = -200;
        Obj[i].P.y = 250;
        Obj[i].status = FLIESSBAND;

        pos = MoveScaraLin(hdc, Pdouble(P4), pos, VelocityScara, hSpPen, TRUE);
        pos = MoveScaraArc(hdc, Pdouble(Pm2), 75, 90, -90, VelocityScara, hSpPen, TRUE);
        pos = MoveScaraLin(hdc, Pdouble(P6), pos, VelocityScara, hSpPen, TRUE);
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
    P.y = 0;

    switch (message)  //Nachrichtenschleife
    {
    case WM_SIZE:  //Bildschirmgröße
        cx = LOWORD(lParam);
        cy = HIWORD(lParam);
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
