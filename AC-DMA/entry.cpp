#include "Header.h"
#include "Player.h"
#include "ESP.h"
#include <iostream>
#include <Windows.h>
#include <ctime>
#include <set>
#include <string>
#include "DMAHandler.h"

void ClearConsole() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD dwConSize, dwWritten;
    COORD coord = { 0, 0 };

    GetConsoleScreenBufferInfo(hConsole, &csbi);
    dwConSize = csbi.dwSize.X * csbi.dwSize.Y;
    FillConsoleOutputCharacter(hConsole, ' ', dwConSize, coord, &dwWritten);
    FillConsoleOutputAttribute(hConsole, csbi.wAttributes, dwConSize, coord, &dwWritten);
    SetConsoleCursorPosition(hConsole, coord);
}

void PrintInitialInfo(DWORD pID, DWORD baseAddress, DWORD localPlayer, DWORD entityList, DWORD viewMatrix) {
    std::cout << "Process ID: " << pID << std::endl;
    std::cout << "Base address: 0x" << std::hex << baseAddress << std::endl;
    std::cout << "Local player address: 0x" << std::hex << localPlayer << std::endl;
    std::cout << "Entity list address: 0x" << std::hex << entityList << std::endl;
    std::cout << "View matrix address: 0x" << std::hex << viewMatrix << std::endl;
    std::cout << std::endl;
}

struct PlayerInfo {
    std::string name;
    int health;

    bool operator<(const PlayerInfo& other) const {
        if (name != other.name) return name < other.name;
        return health < other.health;
    }
};

void PrintPlayerList(DMAHandler* dma, DWORD baseAddress, DWORD entityList) {
    int maxPlayers = 0;
    dma->read(baseAddress + entityList, (ULONG64)&maxPlayers, sizeof(int));
    if (maxPlayers <= 0 || maxPlayers > 32) {
        maxPlayers = 32;
    }
    std::cout << "Max players in server: " << std::dec << maxPlayers << std::endl;

    std::set<PlayerInfo> uniquePlayers;
    int playerCount = 0;
    std::cout << std::endl;

    for (int i = 0; i < maxPlayers; ++i) {
        DWORD playerPtr = 0;
        dma->read(entityList + (i * 4), (ULONG64)&playerPtr, sizeof(DWORD));
        if (playerPtr) {
            PlayerData data;
            if (ReadPlayerData(dma, playerPtr, &data)) {
                bool isValid = true;
                for (int j = 0; j < strlen(data.name); ++j) {
                    if (data.name[j] < 32 || data.name[j] > 126) {
                        isValid = false;
                        break;
                    }
                }
                if (isValid && data.name[0] != '\0') {
                    PlayerInfo player = { std::string(data.name), data.health };
                    if (uniquePlayers.insert(player).second) {
                        std::cout << "Player " << playerCount + 1 << ": " << data.name << " (Health: ";
                        if (data.health <= 0) {
                            std::cout << "DEAD";
                        }
                        else {
                            std::cout << data.health;
                        }
                        std::cout << ")" << std::endl;
                        playerCount++;
                    }
                }
            }
        }
    }
    std::cout << "\nValid Players: " << std::dec << playerCount << std::endl;
}

int main() {
    // Initialize DMA
    DMAHandler* d_handler = new DMAHandler(L"ac_client.exe", true);
    if (!d_handler->isInitialized()) {
        std::cout << "Failed to initialize DMA or process not found!" << std::endl;
        delete d_handler;
        return 1;
    }

    // Get process ID
    DWORD pID = d_handler->getPID();
    if (!pID) {
        std::cout << "Process ID not found! Ensure ac_client.exe is running on the target PC." << std::endl;
        DMAHandler::closeDMA();
        delete d_handler;
        return 1;
    }

    // Get base address of ac_client.exe
    DWORD baseAddress = d_handler->getBaseAddress();
    if (!baseAddress) {
        std::cout << "Base address not found!" << std::endl;
        DMAHandler::closeDMA();
        delete d_handler;
        return 1;
    }

    // Get LOCAL_PLAYER address
    DWORD localPlayer = 0;
    d_handler->read(baseAddress + LOCAL_ENTITY, (ULONG64)&localPlayer, sizeof(DWORD));
    if (!localPlayer) {
        std::cout << "Local player not found!" << std::endl;
        DMAHandler::closeDMA();
        delete d_handler;
        return 1;
    }

    // Get ENTITY_LIST address
    DWORD entityList = 0;
    d_handler->read(baseAddress + ENTITY_LIST, (ULONG64)&entityList, sizeof(DWORD));
    if (!entityList) {
        std::cout << "Entity list not found!" << std::endl;
        DMAHandler::closeDMA();
        delete d_handler;
        return 1;
    }

    // Get VIEW_MATRIX address
    DWORD viewMatrix = 0;
    d_handler->read(baseAddress + VIEW_MATRIX, (ULONG64)&viewMatrix, sizeof(DWORD));
    if (!viewMatrix) {
        std::cout << "View matrix not found!" << std::endl;
        DMAHandler::closeDMA();
        delete d_handler;
        return 1;
    }

    // Print initial info once
    PrintInitialInfo(pID, baseAddress, localPlayer, entityList, viewMatrix);

    // Run features and refresh player list every 1 second
    ULONGLONG lastRefresh = GetTickCount64();
    const ULONGLONG refreshInterval = 1000;
    bool running = true;

    while (running) {
        // Process Windows messages
        MSG msg;
        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (!running) break;

        // Run health and ammo mods
        RunHealthMod(d_handler, localPlayer);
        RunAmmoMod(d_handler, localPlayer);

        // Draw ESP
        DrawESP(d_handler, nullptr, baseAddress, entityList);

        // Update console periodically
        ULONGLONG currentTime = GetTickCount64();
        if (currentTime - lastRefresh >= refreshInterval) {
            ClearConsole();
            PrintInitialInfo(pID, baseAddress, localPlayer, entityList, viewMatrix);
            PrintPlayerList(d_handler, baseAddress, entityList);
            lastRefresh = currentTime;
        }

        // Small delay to prevent excessive CPU usage
        Sleep(1);
    }

    // Cleanup
    DMAHandler::closeDMA();
    delete d_handler;
    return 0;
}