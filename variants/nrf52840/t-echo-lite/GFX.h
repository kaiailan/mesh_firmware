#pragma once
// Shim: InkHUD expects GFX.h with a GFX class, but Adafruit provides Adafruit_GFX
#include <Adafruit_GFX.h>
using GFX = Adafruit_GFX;
