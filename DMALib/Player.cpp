#include "Player.h"

void RunHealthMod(DMAHandler* dma, DWORD_PTR localPlayer) {
    int newHealth = 999;
    dma->write(localPlayer + HEALTH, newHealth);
    dma->write(localPlayer + ARMOUR, newHealth);
}

void RunAmmoMod(DMAHandler* dma, DWORD_PTR localPlayer) {
    int newAmmo = 999;
    dma->write(localPlayer + ASSAULT_AMMO, newAmmo);
    dma->write(localPlayer + ASSAULT_RESERVE, newAmmo);
    dma->write(localPlayer + PISTOL_AMMO, newAmmo);
    dma->write(localPlayer + PISTOL_RESERVE, newAmmo);
    dma->write(localPlayer + SNIPER_AMMO, newAmmo);
    dma->write(localPlayer + SNIPER_RESERVE, newAmmo);
    dma->write(localPlayer + SMG_AMMO, newAmmo);
    dma->write(localPlayer + SMG_RESERVE, newAmmo);
    dma->write(localPlayer + SHOTGUN_AMMO, newAmmo);
    dma->write(localPlayer + SHOTGUN_RESERVE, newAmmo);
    dma->write(localPlayer + CARBINE_AMMO, newAmmo);
    dma->write(localPlayer + CARBINE_RESERVE, newAmmo);
    dma->write(localPlayer + GRENADE_AMMO, newAmmo);
}