#pragma once

#include "configuration.h"

#ifdef MESHTASTIC_INCLUDE_NICHE_GRAPHICS

#include "graphics/niche/InkHUD/InkHUD.h"

void setupNicheGraphics()
{
    // T-Echo Lite: minimal InkHUD setup (no Applets to save Flash)
    NicheGraphics::InkHUD::InkHUD::getInstance();
}

#endif
