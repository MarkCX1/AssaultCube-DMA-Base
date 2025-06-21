#pragma once
#include "Header.h"

bool WorldToScreen(Vec3 pos, Vec2& screen, float matrix[16], int windowWidth, int windowHeight);
void DrawESP(DMAHandler* dma, HWND hwnd, DWORD baseAddress, DWORD entityList);