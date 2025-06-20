#pragma once
#include <Windows.h>
#include <iostream>
#include "vmmdll.h"
#include "offsets.h"
#include "DMAHandler.h"
using namespace std;

// Vector structures
typedef struct {
    float x, y, z, w;
} Vec4;

typedef struct {
    float x, y, z;
} Vec3;

typedef struct {
    float x, y;
} Vec2;

// Player data structure
struct PlayerData {
    int health;
    char name[16];
};

// Function declarations
bool ReadPlayerData(DMAHandler* dma, DWORD playerBase, PlayerData* data);

// Function implementations
inline bool ReadPlayerData(DMAHandler* dma, DWORD playerBase, PlayerData* data) {
    memset(data, 0, sizeof(PlayerData)); // Initialize to avoid garbage data
    dma->read(playerBase + HEALTH, (ULONG64)&data->health, sizeof(int));
    dma->read(playerBase + PLAYER_NAME, (ULONG64)&data->name, sizeof(data->name));
    // Assume success if name is non-empty or health is reasonable; adjust as needed
    return data->name[0] != '\0' || data->health >= 0;
}