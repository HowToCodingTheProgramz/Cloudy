#include "stdafx.h"
#include "resource.h"
#include <GL/gl.h>
#include <chrono>

#pragma comment(lib, "opengl32.lib")

#define MAX_LOADSTRING 100

HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

BOOL gRunning = TRUE;

struct ScreenInfo {
	HGLRC hGLRC;
	HWND wnd;
	RECT rc;
	HDC dc;

	ScreenInfo() : wnd(0), dc(NULL) {}
};
ScreenInfo screens[2];

struct Star {
	float x;
	float y;
	float z;
	float dx;
	float dy;
	float dz;
};

struct StarLayer {
	Star * stars;
	int num_stars;
	float z;
	float frust;
	float r;
};

DWORD WINAPI RenderFunction(LPVOID lpParam) {
	UNREFERENCED_PARAMETER(lpParam);

	if (screens[0].dc == 0) return 1;

	float aspect_ratio = (screens[0].rc.right - screens[0].rc.left) / (screens[0].rc.bottom - screens[0].rc.top);

	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR),  /* size */
		1,                              /* version */
		PFD_SUPPORT_OPENGL |
		PFD_DRAW_TO_WINDOW |
		PFD_DOUBLEBUFFER,               /* support double-buffering */
		PFD_TYPE_RGBA,                  /* color type */
		16,                             /* prefered color depth */
		0, 0, 0, 0, 0, 0,               /* color bits (ignored) */
		0,                              /* no alpha buffer */
		0,                              /* alpha bits (ignored) */
		0,                              /* no accumulation buffer */
		0, 0, 0, 0,                     /* accum bits (ignored) */
		16,                             /* depth buffer */
		0,                              /* no stencil buffer */
		0,                              /* no auxiliary buffers */
		PFD_MAIN_PLANE,                 /* main layer */
		0,                              /* reserved */
		0, 0, 0,                        /* no layer, visible, damage masks */
	};
	int pixelFormat;

	pixelFormat = ChoosePixelFormat(screens[1].dc, &pfd);
	if (pixelFormat == 0) { return 7; }
	if (SetPixelFormat(screens[1].dc, pixelFormat, &pfd) != TRUE) { return 9; }

	screens[1].hGLRC = wglCreateContext(screens[1].dc);
	wglMakeCurrent(screens[1].dc, screens[1].hGLRC);

	/* set viewing projection */
	glMatrixMode(GL_PROJECTION);
	glFrustum(-0.5F, 0.5F, -0.5F, 0.5F, 1.0F, 10.0F);

	/* position viewer */
	glMatrixMode(GL_MODELVIEW);
	glTranslatef(0.0F, 0.0F, -5.0F);

	const int num_layers = 3;
	StarLayer layers[num_layers] = {
		{ nullptr, 256, 1.3f, 3.64f, 0.2f },
		{ nullptr, 128, 2.4f, 4.2f, 0.6f },
		{ nullptr, 192, 3.5f, 1.6f, 1.0f }
	};
	srand(2635);
	for (int i = 0; i < num_layers; i++) {
		StarLayer & layer = layers[i];
		layer.stars = new Star[layer.num_stars];

		for (int j = 0; j < layer.num_stars; j++) {
			Star & star = layer.stars[j];

			star.x = ((static_cast<float>(rand()) / RAND_MAX) * layer.frust) - (layer.frust * 0.5f);
			star.y = ((static_cast<float>(rand()) / RAND_MAX) * layer.frust) - (layer.frust * 0.5f);
			star.z = layer.z;
			star.dx = 0.0f;
			star.dy = ((rand() / RAND_MAX) * 0.025f) + (0.0025f + (0.001f * i));
			star.dz = 0.0f;
		}
	}

	glEnable(GL_DEPTH_TEST);

	const float expectedFrameTime = 1000.0f / 60.0f;

	float block_w = 0.0005f;
	float block_h = block_w * aspect_ratio;

	while (gRunning) {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glBegin(GL_QUADS);
		for (int i = 0; i < num_layers; i++) {
			StarLayer & layer = layers[i];

			for (int j = 0; j < layer.num_stars; j++) {
				Star & star = layer.stars[j];

				glNormal3f(0.0F, 0.0F, 1.0F);
				glColor3f(layer.r, 0.4f, 0.8f); glVertex3f(star.x + block_w, star.y + block_h, star.z);
				glColor3f(layer.r, 0.4f, 0.8f); glVertex3f(star.x - block_w, star.y + block_h, star.z);
				glColor3f(layer.r, 0.4f, 0.8f); glVertex3f(star.x - block_w, star.y - block_h, star.z);
				glColor3f(layer.r, 0.4f, 0.8f); glVertex3f(star.x + block_w, star.y - block_h, star.z);

				star.y -= star.dy;
				if (star.y < -(layer.frust * 0.5f)) {
					star.y += layer.frust;
				}
			}
		}
		glEnd();

		SwapBuffers(screens[1].dc);
		Sleep(expectedFrameTime);
	}

	ReleaseDC(screens[0].wnd, screens[0].dc);
	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(screens[1].hGLRC);

	for (int i = 0; i < num_layers; i++) {
		delete[] layers[i].stars;
	}
	return 0;
}

BOOL CALLBACK FindWorker(HWND wnd, LPARAM lp) {
	HWND *pworker = (HWND*)lp;
	if (!FindWindowExA(wnd, 0, "SHELLDLL_DefView", 0)) { return TRUE; }
	*pworker = FindWindowExA(0, wnd, "WorkerW", 0);
	if (*pworker) { return FALSE; }
	return TRUE;
}
BOOL AfterWin8() {
	OSVERSIONINFOEX osvi;
	DWORDLONG dwlConditionMask = 0;

	ZeroMemory(&osvi, sizeof(OSVERSIONINFOEX));
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	osvi.dwMajorVersion = 6;
	osvi.dwMinorVersion = 2;

	VER_SET_CONDITION(dwlConditionMask, VER_MAJORVERSION, VER_GREATER_EQUAL);
	VER_SET_CONDITION(dwlConditionMask, VER_MINORVERSION, VER_GREATER_EQUAL);

	return VerifyVersionInfo(&osvi, VER_MAJORVERSION | VER_MINORVERSION, dwlConditionMask);
}
HWND SpawnWorker() {
	HWND wnd_worker;
	HWND progman = FindWindowA("Progman", 0);

	if (!progman) {
		OutputDebugStringA("failed to find Progman");
		return 0;
	}

	SendMessageA(progman, 0x052C, 0xD, 0);
	SendMessageA(progman, 0x052C, 0xD, 1);
	EnumWindows(FindWorker, (LPARAM)&wnd_worker);

	if (!wnd_worker) {
		OutputDebugStringA("W: couldn't spawn WorkerW window, trying old method");
		SendMessageA(progman, 0x052C, 0, 0);
		EnumWindows(FindWorker, (LPARAM)&wnd_worker);
	}
	if (wnd_worker && !AfterWin8()) {
		OutputDebugStringA("detected windows 7, hiding worker window");
		ShowWindow(wnd_worker, SW_HIDE);
		wnd_worker = progman;
	}
	if (!wnd_worker) {
		OutputDebugStringA("W: couldnt spawn window behind icons, falling back to Progman");
		wnd_worker = progman;
	}
	return wnd_worker;
}
int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
	LoadStringW(hInstance, IDC_CLOUDYMASTER, szWindowClass, MAX_LOADSTRING);
	MyRegisterClass(hInstance);

	screens[0].wnd = SpawnWorker();
	if (!screens[0].wnd) { return FALSE; }

	HANDLE hThread = INVALID_HANDLE_VALUE;
	screens[0].dc = GetDC(screens[0].wnd);
	GetWindowRect(screens[0].wnd, &screens[0].rc);

	if (!InitInstance (hInstance, SW_SHOW)) { return FALSE; }
	screens[1].dc = GetDC(screens[1].wnd);
	GetWindowRect(screens[1].wnd, &screens[1].rc);

	hThread = CreateThread(NULL, 0, &RenderFunction, NULL, 0, NULL);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_CLOUDYMASTER));
    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0)) {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
	if (hThread != INVALID_HANDLE_VALUE) {
		gRunning = FALSE;
		WaitForSingleObject(hThread, 1000);
		CloseHandle(hThread);
	}
    return (int) msg.wParam;
}
ATOM MyRegisterClass(HINSTANCE hInstance) {
    WNDCLASSEX wcex;
	ZeroMemory(&wcex, sizeof(WNDCLASSEX));

    wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wcex.lpfnWndProc    = WndProc;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CLOUDYMASTER));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
   hInst = hInstance;
   screens[1].wnd = CreateWindowW(szWindowClass, szTitle, WS_CHILD, 0, 0, screens[0].rc.right - screens[0].rc.left, screens[0].rc.bottom - screens[0].rc.top, screens[0].wnd, nullptr, hInstance, nullptr);

   if (!screens[1].wnd) { return FALSE; }

   ShowWindow(screens[1].wnd, nCmdShow);
   UpdateWindow(screens[1].wnd);
   return TRUE;
}
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	if (message == WM_DESTROY) {
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, message, wParam, lParam);
}