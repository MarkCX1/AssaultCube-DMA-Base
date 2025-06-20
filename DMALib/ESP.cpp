#include "ESP.h"
#include "offsets.h"
#include <windows.h>
#include <iostream>
#include <cmath>
#include "vmmdll.h"

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
    static HWND overlay = nullptr;
    if (!overlay) {
        WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_HREDRAW | CS_VREDRAW, DefWindowProc, 0, 0, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Overlay", nullptr };
        RegisterClassEx(&wc);

        // Create a 1920x1080 overlay at (0, 0) on PC2
        int width = 1920;
        int height = 1080;
        overlay = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST, L"Overlay", L"ESP",
            WS_POPUP, 0, 0, width, height, nullptr, nullptr, GetModuleHandle(nullptr), nullptr);
        SetLayeredWindowAttributes(overlay, RGB(0, 0, 0), 0, LWA_COLORKEY);
        ShowWindow(overlay, SW_SHOW);
    }

    HDC hdc = GetDC(overlay);
    SetBkMode(hdc, TRANSPARENT);
    RECT rect = { 0, 0, 1920, 1080 };
    HBRUSH brush = CreateSolidBrush(RGB(0, 0, 0));
    FillRect(hdc, &rect, brush);
    DeleteObject(brush);

    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

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

    float viewMatrix[16];
    dma->read(baseAddress + VIEW_MATRIX, (ULONG64)viewMatrix, sizeof(viewMatrix));

    bool aimbotActive = GetAsyncKeyState(VK_XBUTTON1) & 0x8000;
    Vec2 closestTarget = { -1, -1 };
    float closestDist = fovRadius + 1;

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

    ReleaseDC(overlay, hdc);
}