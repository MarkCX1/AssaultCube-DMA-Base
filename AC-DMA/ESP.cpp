#include "ESP.h"
#include "offsets.h"
#include <windows.h>
#include <iostream>
#include <cmath>
#include "vmmdll.h"

// Global flag to track fullscreen state
static bool isFullscreen = false;
static RECT windowedRect = { 0 }; // To store windowed mode position/size

// Custom window procedure to handle window messages
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    case WM_KEYDOWN:
        if (wParam == VK_F11) { // Allow f11 usage
            isFullscreen = !isFullscreen;
            if (isFullscreen) {
                GetWindowRect(hwnd, &windowedRect);
                HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
                MONITORINFO mi = { sizeof(mi) };
                GetMonitorInfo(hMonitor, &mi);
                SetWindowLongPtr(hwnd, GWL_STYLE, WS_POPUP | WS_VISIBLE);
                SetWindowPos(hwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top,
                    mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_FRAMECHANGED);
            }
            else {
                SetWindowLongPtr(hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
                SetWindowPos(hwnd, HWND_TOP, windowedRect.left, windowedRect.top,
                    windowedRect.right - windowedRect.left, windowedRect.bottom - windowedRect.top, SWP_FRAMECHANGED);
            }
            return 0;
        }
        break;
    case WM_NCHITTEST: {
        LRESULT hit = DefWindowProc(hwnd, msg, wParam, lParam);
        if (hit == HTCLIENT) hit = HTCAPTION;
        return hit;
    }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool WorldToScreen(Vec3 pos, Vec2& screen, float matrix[16], int width, int height) {
    float clipX = pos.x * matrix[0] + pos.y * matrix[4] + pos.z * matrix[8] + matrix[12];
    float clipY = pos.x * matrix[1] + pos.y * matrix[5] + pos.z * matrix[9] + matrix[13];
    float clipW = pos.x * matrix[3] + pos.y * matrix[7] + pos.z * matrix[11] + matrix[15];
    if (clipW < 0.1f) return false;
    screen.x = (width / 2) * (1 + clipX / clipW);
    screen.y = (height / 2) * (1 - clipY / clipW);
    return true;
}

void DrawESP(DMAHandler* dma, HWND gameWindow, DWORD baseAddress, DWORD entityList) {
    static HWND espWindow = nullptr;
    if (!espWindow) {
        // Register window class
        WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, WndProc, 0, 0, GetModuleHandle(nullptr), nullptr, nullptr, (HBRUSH)(COLOR_WINDOW + 1), nullptr, L"ESPWindow", nullptr };
        if (!RegisterClassEx(&wc)) {
            std::cerr << "Failed to register window class. Error: " << GetLastError() << std::endl;
            return;
        }

        // Make a window
        int width = 1920;
        int height = 1080;
        espWindow = CreateWindowEx(0, L"ESPWindow", L"AssaultCube ESP",
            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height,
            nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
        if (!espWindow) {
            std::cerr << "Failed to create window. Error: " << GetLastError() << std::endl;
            return;
        }

        // Show and update the window
        ShowWindow(espWindow, SW_SHOW);
        UpdateWindow(espWindow);
    }

    // Get device context and clear the window with a solid background
    HDC hdc = GetDC(espWindow);
    if (!hdc) return;
    SetBkMode(hdc, TRANSPARENT);
    RECT rect = { 0, 0, 800, 600 };
    GetClientRect(espWindow, &rect);
    HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0)); 
    FillRect(hdc, &rect, brush);
    DeleteObject(brush);

    // Get window dimensions
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    // Draw FOV circle
    const float fovAngle = 30.0f;
    float fovRadius = tan(fovAngle * 3.14159265f / 360.0f) * width * 0.5f;
    HPEN fovPen = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    HBRUSH nullBrush = (HBRUSH)GetStockObject(NULL_BRUSH);
    SelectObject(hdc, fovPen);
    SelectObject(hdc, nullBrush);
    Ellipse(hdc, (int)(width / 2 - fovRadius), (int)(height / 2 - fovRadius),
        (int)(width / 2 + fovRadius), (int)(height / 2 + fovRadius));
    DeleteObject(fovPen);
    DeleteObject(nullBrush);

    // Read view matrix
    float viewMatrix[16];
    dma->read(baseAddress + VIEW_MATRIX, (ULONG64)viewMatrix, sizeof(viewMatrix));

    // Aimbot logic
    bool aimbotActive = GetAsyncKeyState(VK_XBUTTON1) & 0x8000;
    Vec2 closestTarget = { -1, -1 };
    float closestDist = fovRadius + 1;

    // Iterate through players
    for (int i = 0; i < 32; i++) {
        DWORD playerAddr = 0;
        dma->read(entityList + i * 4, (ULONG64)&playerAddr, sizeof(DWORD));
        if (playerAddr) {
            PlayerData data;
            Vec3 feet = { 0, 0, 0 };
            Vec2 feetScreen;
            dma->read(playerAddr + HEAD_X, (ULONG64)&feet.x, sizeof(float));
            dma->read(playerAddr + HEAD_Y, (ULONG64)&feet.y, sizeof(float));
            dma->read(playerAddr + HEAD_Z, (ULONG64)&feet.z, sizeof(float));
            if (ReadPlayerData(dma, playerAddr, &data) && feet.x != 0 && feet.y != 0 && feet.z != 0) {
                if (WorldToScreen(feet, feetScreen, viewMatrix, width, height)) {
                    float espDotY = feetScreen.y - 50.0f;
                    HBRUSH dotBrush = CreateSolidBrush(data.health > 0 ? RGB(0, 255, 0) : RGB(255, 0, 0));
                    SelectObject(hdc, dotBrush);
                    Ellipse(hdc, (int)feetScreen.x - 5, (int)espDotY - 5, (int)feetScreen.x + 5, (int)espDotY + 5);
                    DeleteObject(dotBrush);

                    if (aimbotActive && data.health > 0) {
                        float dist = sqrt(pow(feetScreen.x - width / 2, 2) + pow(feetScreen.y - height / 2, 2));
                        if (dist < fovRadius && dist < closestDist) {
                            closestDist = dist;
                            closestTarget = feetScreen;
                            closestTarget.y -= -25.0f;
                        }
                    }
                }
            }
        }
    }

    // Aimbot mouse movement
    if (aimbotActive && closestTarget.x != -1 && closestTarget.y != -1) {
        POINT currentPos;
        GetCursorPos(&currentPos);

        int deltaX = (int)(closestTarget.x - width / 2);
        int deltaY = (int)(closestTarget.y - height / 2);

        const float smoothing = 5.0f;
        deltaX = (int)(deltaX / smoothing);
        deltaY = (int)(deltaY / smoothing);

        if (deltaX != 0 || deltaY != 0) {
            mouse_event(MOUSEEVENTF_MOVE, deltaX, deltaY, 0, 0);
        }
    }

    ReleaseDC(espWindow, hdc);
}