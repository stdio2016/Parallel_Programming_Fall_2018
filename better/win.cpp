#ifdef _WIN32
#include<windows.h>
#include<cstdio>
#include<cstring>
#include"BmpReader.h"

HWND hWindow;
char *bmpName;
HBITMAP hBmp;
int retVal = 0xc000013a;

// define this entry point in other file
int my_main(int argc, char *argv[]);

// process message
LRESULT CALLBACK processWinMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
      case WM_CREATE: {
        break;
      }
      case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        HDC hdc2 = CreateCompatibleDC(hdc);
        FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));

        if (hBmp != NULL) {
            // draw image
            HGDIOBJ old = SelectObject(hdc2, hBmp);
            BITMAP bmp;
            GetObject(hBmp, sizeof bmp, &bmp);
            BitBlt(hdc, 0, 0, bmp.bmWidth, bmp.bmHeight, hdc2, 0, 0, SRCCOPY);
            SelectObject(hdc2, old);
        }
        DeleteDC(hdc2);
        EndPaint(hwnd, &ps);
        return 0;
      }
      case WM_DESTROY: {
        PostQuitMessage(retVal);
        return 0;
      }
      case WM_USER+2: {
        // load image
        if (hBmp != NULL) DeleteObject(hBmp);
        hBmp = (HBITMAP) LoadImage(NULL, bmpName, IMAGE_BITMAP, 0, 0, LR_LOADFROMFILE);

        // set new window size
        BITMAP bmp;
        GetObject(hBmp, sizeof bmp, &bmp);
        RECT rect, cliRect;
        GetWindowRect(hwnd, &rect);
        GetClientRect(hwnd, &cliRect);
        int bx = rect.right - rect.left - cliRect.right;
        int by = rect.bottom - rect.top - cliRect.bottom;
        SetWindowPos(hwnd, HWND_TOPMOST, rect.left, rect.top, bmp.bmWidth + bx, bmp.bmHeight + by, SWP_NOZORDER | SWP_NOACTIVATE);
        // update image
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
      }
    }
    // default handler
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

DWORD WINAPI backgroundThread(void *param) {
    puts("entering main");
    retVal = my_main(__argc, __argv);
    puts("leaving main");
    SendMessage(hWindow, WM_CLOSE, 0, 0);
    return retVal ;
}

// entry point on Windows
int main(int argc, char *argv[]) {
    HINSTANCE hInstance = GetModuleHandle(NULL);
    const char ClassName[] = "Gaussian_Blur";
    WNDCLASS cls = {};
    cls.lpfnWndProc = processWinMessage;
    cls.hInstance = hInstance;
    cls.lpszClassName = ClassName;
    RegisterClass(&cls);

    hWindow = CreateWindow(
        ClassName,
        "Current Progress",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, // x
        CW_USEDEFAULT, // y
        CW_USEDEFAULT, // width
        CW_USEDEFAULT, // height
        NULL, // parent window
        NULL, // menu
        hInstance,
        NULL // lpParam
    );
    if (hWindow == 0) {
        fprintf(stderr, "failed to create window!\n");
        return 2;
    }
    puts("show window");
    ShowWindow(hWindow, SW_SHOW);
    puts("finish window");
	DWORD pid;
	CreateThread(NULL, 0, backgroundThread, NULL, 0, &pid);
    MSG m = {};
    while (GetMessage(&m, NULL, 0, 0)) {
        TranslateMessage(&m);
        DispatchMessage(&m);
    }
    TerminateProcess(hInstance, retVal);
    return retVal;
}

void show_image(const char *filename) {
    if (bmpName != NULL) delete[] bmpName;
    int len = strlen(filename);
    bmpName = new char[len+1];
    memcpy(bmpName, filename, len+1);
    SendMessage(hWindow, WM_USER + 2, 0, 0);
}
#endif
