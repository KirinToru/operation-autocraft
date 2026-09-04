#pragma once
#include "core/config.h"

extern bool g_ShowMainMenu;
extern bool g_ShowDiagnostics;
extern bool g_ShowControlPanel;
extern bool g_ShowSettings;
extern bool g_ShowKeybindings;
extern bool g_CalibrationMode;
extern bool g_AppVisible;
extern KeyBind* g_CurrentBinding;

void RenderGUI();
void AddLog(const std::string& msg);

