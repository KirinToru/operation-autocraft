#pragma once
#include <windows.h>
#include "config.h"

struct PixelColor {
    unsigned char r, g, b, a;
};

// Initializes DXGI duplication (call once in bot thread)
bool InitVision();

// Captures a frame. Returns true if successful.
bool CaptureFrame();

// Gets the color of a specific pixel relative to a bounding box (if needed) or screen space
PixelColor GetPixel(int x, int y);

// Cleanup
void CleanupVision();

// Replicating Python logic
bool is_prompt_light(int x, int y);
bool area_has_heart_prompt();
bool area_has_start_prompt();
bool area_has_yellow(int rel_x1, int rel_y1, int rel_x2, int rel_y2);
// Returns "FULL_GREY", "FULL_PINK", "PARTIAL_PINK", or "UNKNOWN"
const char* GetMaleBarState();

// Returns true if the female arrow is yellow
bool IsFemaleArrowYellow();

// Returns true if the male arrow is yellow
bool IsMaleArrowYellow();

// Returns true if the Heart L button is present (glowing)
bool IsHeartButtonPresent();

// Returns true if the given gauge position (first or last) is blue
bool IsGaugeSliceBlue(const Point2D& pos);
int get_gauge_fullness();
