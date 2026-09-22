#pragma once
#include "imgui.h"
#include <string>
#include <vector>

bool GameStyleButton(const char* label, bool active, const ImVec2& size_arg = ImVec2(0, 0), bool hoverIsPink = false);
bool ActionConfirmButton(const char* label, const ImVec2& size_arg = ImVec2(0,0), ImU32 bgCol = IM_COL32(248, 101, 128, 255));
bool SupportConfirmButton(const char* label, ImTextureID icon_tex, const ImVec2& size_arg = ImVec2(0,0), ImU32 bgCol = IM_COL32(240, 175, 20, 255));
bool MenuSelectButton(const char* label, bool active, const ImVec2& size_arg = ImVec2(0, 0));
bool MenuSelectButtonWithBind(const char* label, const char* bindStr, bool active, const ImVec2& size_arg = ImVec2(0, 0));
void DrawGameStyleWindowBorder(ImU32 color = IM_COL32(106, 113, 113, 255));
bool DrawCalibrationTitleBar(const char* title);
bool DrawCustomTitleBar(const char* title, bool& isPinned);
bool DrawSubWindowTitleBar(const char* title, bool& isPinned);
bool CyanActionButton(const char* label, const ImVec2& size_arg);
bool CustomSelector(const char* label, int* current_item, const std::vector<std::string>& items, float width = 200.0f);
bool CustomCheckbox(const char* label, bool* v);
bool CustomSliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.1f");
bool CustomSliderInt(const char* label, int* v, int v_min, int v_max, const char* format = "%d");
