#include "gui.h"
#include "imgui.h"
#include <windows.h>
#include <string>
#include <atomic>

#include <vector>
#include <mutex>
#include "config.h"
#include "vision.h"

bool g_ShowMainMenu = true;
bool g_ShowDiagnostics = false;
bool g_ShowControlPanel = true;
bool g_ShowSettings = false;
bool g_ShowKeybindings = false;

// Calibration Restore State
bool g_RestoreMainMenu = false;
bool g_RestoreDiagnostics = false;
bool g_RestoreControlPanel = false;
bool g_RestoreSettings = false;
bool g_RestoreKeybindings = false;
bool g_WasInCalibration = false;
bool g_CalibrationMode = false;
bool g_AppVisible = true;
KeyBind* g_CurrentBinding = nullptr;

bool g_PinMainMenu = true;
bool g_PinControlPanel = true;
bool g_PinSettings = true;
bool g_PinDiagnostics = true;
bool g_PinKeybindings = true;

extern std::atomic<bool> g_bRunning;
extern HWND g_hwnd;

std::vector<std::string> g_Logs;
std::mutex g_LogMutex;

void AddLog(const std::string& message)
{
    std::lock_guard<std::mutex> lock(g_LogMutex);
    g_Logs.push_back(message);
    if (g_Logs.size() > 20) g_Logs.erase(g_Logs.begin());
}

bool HoverTextButton(const char* label, bool active, const ImVec2& size = ImVec2(0, 0), bool hoverIsRed = false)
{
    ImVec2 p = ImGui::GetCursorScreenPos();
    
    // Transparent background
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0,0,0,0));

    // Transparent text to let Button do sizing and hit detection without drawing the text
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0,0,0,0)); 
    bool clicked = ImGui::Button(label, size);
    ImGui::PopStyleColor(); // pop transparent text

    bool hovered = ImGui::IsItemHovered();
    ImU32 textColor;
    if (hovered) 
    {
        if (hoverIsRed) textColor = IM_COL32(255, 50, 50, 255); // Red
        else textColor = IM_COL32(255, 128, 0, 255); // Orange
    }
    else if (active) textColor = IM_COL32(255, 255, 255, 255); // White
    else textColor = IM_COL32(128, 128, 128, 255); // Grey

    // Draw the text manually centered
    ImVec2 min = ImGui::GetItemRectMin();
    ImVec2 max = ImGui::GetItemRectMax();
    ImVec2 textSize = ImGui::CalcTextSize(label);
    ImVec2 textPos = ImVec2(min.x + (max.x - min.x - textSize.x) * 0.5f, min.y + (max.y - min.y - textSize.y) * 0.5f);
    
    ImGui::GetWindowDrawList()->AddText(textPos, textColor, label);

    ImGui::PopStyleColor(3); // pop Button colors
    return clicked;
}

void ApplyTheme()
{
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_Text] = ImVec4(0.70f, 0.70f, 0.70f, 1.00f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.04f, 0.04f, 0.04f, 0.50f); // 50% transparent dark
    style.Colors[ImGuiCol_TitleBg] = ImVec4(0.1f, 0.1f, 0.1f, 0.45f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.15f, 0.15f, 0.15f, 0.8f);
    
    // Orange borders only around windows, not buttons
    style.Colors[ImGuiCol_Border] = ImVec4(1.0f, 0.5f, 0.0f, 0.6f);
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f; // No borders on buttons
    
    // Buttons (Transparent by default)
    style.Colors[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
}

void UpdateClickThroughState()
{
    // Make clickable ONLY if the mouse is hovering an ImGui window.
    // main.cpp feeds mouse position manually, so ImGui knows when it's hovered even if WS_EX_TRANSPARENT is active.
    bool shouldBeClickable = ImGui::GetIO().WantCaptureMouse;
    
    LONG_PTR exStyle = GetWindowLongPtr(g_hwnd, GWL_EXSTYLE);
    if (shouldBeClickable)
    {
        if (exStyle & WS_EX_TRANSPARENT)
        {
            SetWindowLongPtr(g_hwnd, GWL_EXSTYLE, exStyle & ~WS_EX_TRANSPARENT);
            SetWindowPos(g_hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        if (!(exStyle & WS_EX_TRANSPARENT))
        {
            SetWindowLongPtr(g_hwnd, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT);
            SetWindowPos(g_hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        }
    }
}

void ForceTopMost()
{
    SetWindowPos(g_hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
}

bool IsGameActive(); // Defined in bot_logic.cpp

void RenderGUI()
{
    if (!g_AppVisible || !IsGameActive())
    {
        // Must update click-through even when invisible, so it falls back to transparent
        LONG_PTR exStyle = GetWindowLongPtr(g_hwnd, GWL_EXSTYLE);
        if (!(exStyle & WS_EX_TRANSPARENT)) {
            SetWindowLongPtr(g_hwnd, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT);
            SetWindowPos(g_hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
        }
        return;
    }

    static bool theme_applied = false;
    if (!theme_applied)
    {
        ApplyTheme();
        theme_applied = true;
    }

    if (g_CalibrationMode && !g_WasInCalibration)
    {
        // Just entered calibration mode, save states and close all windows
        g_RestoreMainMenu = g_ShowMainMenu;
        g_RestoreDiagnostics = g_ShowDiagnostics;
        g_RestoreControlPanel = g_ShowControlPanel;
        g_RestoreSettings = g_ShowSettings;
        g_RestoreKeybindings = g_ShowKeybindings;

        g_ShowMainMenu = false;
        g_ShowControlPanel = false;
        g_ShowSettings = false;
        g_ShowKeybindings = false;
        // Keep diagnostics open to see instructions
        g_ShowDiagnostics = true;
        
        g_WasInCalibration = true;
    }
    else if (!g_CalibrationMode && g_WasInCalibration)
    {
        // Just exited calibration mode, restore states
        g_ShowMainMenu = g_RestoreMainMenu;
        g_ShowDiagnostics = g_RestoreDiagnostics;
        g_ShowControlPanel = g_RestoreControlPanel;
        g_ShowSettings = g_RestoreSettings;
        g_ShowKeybindings = g_RestoreKeybindings;
        g_WasInCalibration = false;
    }

    UpdateClickThroughState();
    ForceTopMost();

    auto CheckImGuiBind = [](const KeyBind& bind) {
        if (bind.vkCode == 0) return false;
        ImGuiIO& io = ImGui::GetIO();
        
        bool modsMatch = (bind.shift == io.KeyShift) && (bind.ctrl == io.KeyCtrl) && (bind.alt == io.KeyAlt);
        if (!modsMatch) return false;

        if (bind.vkCode == VK_LBUTTON) return ImGui::IsMouseClicked(0);
        if (bind.vkCode == VK_RBUTTON) return ImGui::IsMouseClicked(1);
        if (bind.vkCode == VK_MBUTTON) return ImGui::IsMouseClicked(2);
        if (bind.vkCode == VK_XBUTTON1) return ImGui::IsMouseClicked(3);
        if (bind.vkCode == VK_XBUTTON2) return ImGui::IsMouseClicked(4);
        
        // Use ImGui IsKeyPressed for debounced keyboard checks
        return ImGui::IsKeyPressed((ImGuiKey)bind.vkCode, false);
    };

    auto HandlePinning = [&](bool& isPinned) {
        if (ImGui::IsWindowHovered()) {
            bool pinAll = CheckImGuiBind(BIND_PIN_ALL);
            bool pinSingle = CheckImGuiBind(BIND_PIN_WINDOW);

            if (pinAll) {
                bool newState = !isPinned;
                g_PinMainMenu = g_PinControlPanel = g_PinSettings = g_PinDiagnostics = g_PinKeybindings = newState;
            } else if (pinSingle) {
                isPinned = !isPinned;
            }
        }
    };

    if (g_CalibrationMode)
    {
        ImGuiWindowFlags calibFlags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | 
                                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | 
                                 ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoMove;

        ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, 50.0f), ImGuiCond_Always, ImVec2(0.5f, 0.0f));
        ImGui::SetNextWindowBgAlpha(0.9f);
        if (ImGui::Begin("CalibrationText", nullptr, calibFlags))
        {
            ImGui::SetWindowFontScale(3.0f);
            ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.0f, 1.0f), LANGUAGE == "RU" ? "РЕЖИМ КАЛИБРОВКИ" : "CALIBRATION MODE");
            ImGui::SetWindowFontScale(1.0f);
            ImGui::Separator();
            ImGui::Text(LANGUAGE == "RU" ? "Нажмите 1, 2, 3 для установки позиций игроков." : "Press 1, 2, 3 to set Player positions.");
            ImGui::Text(LANGUAGE == "RU" ? "Нажмите Shift+1, Shift+2, Shift+3 для установки позиций лайков." : "Press Shift+1, Shift+2, Shift+3 to set Like positions.");
            ImGui::Text(LANGUAGE == "RU" ? "Нажмите 5 для установки начала шкалы (1/10)" : "Press 5 to set Gauge Start (1/10)");
            ImGui::Text(LANGUAGE == "RU" ? "Нажмите 6 для установки конца шкалы (10/10)" : "Press 6 to set Gauge End (10/10)");
            ImGui::Text(LANGUAGE == "RU" ? "Нажмите 7 для установки мужской полосы стамины" : "Press 7 to set Male Stamina Bar");
            ImGui::Text(LANGUAGE == "RU" ? "Нажмите 8 для установки женской стрелки стамины" : "Press 8 to set Female Stamina Arrow");
            ImGui::Text(LANGUAGE == "RU" ? "Нажмите 9 для установки кнопки сердца (L)" : "Press 9 to set Heart Button (L)");
            ImGui::Text(LANGUAGE == "RU" ? "Нажмите 0 для установки мужской стрелки стамины" : "Press 0 to set Male Stamina Arrow");
            ImGui::Text(LANGUAGE == "RU" ? "Нажмите ESC для выхода из калибровки." : "Press ESC to exit calibration.");
            ImGui::End();
        }
        g_ShowDiagnostics = true;
    }
    else
    {
        // MAIN MENU
        if (g_ShowMainMenu)
        {
            ImGuiWindowFlags mmFlags = (g_PinMainMenu ? (ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse) : 0) | ImGuiWindowFlags_AlwaysAutoResize;
            if (ImGui::Begin(LANGUAGE == "RU" ? "Главное Меню" : "Main Menu", &g_ShowMainMenu, mmFlags))
            {
                HandlePinning(g_PinMainMenu);
                if (HoverTextButton(bot_enabled ? (LANGUAGE == "RU" ? "ПАУЗА" : "PAUSE") : (LANGUAGE == "RU" ? "СТАРТ" : "START"), true, ImVec2(150, 30)))
                {
                    bot_enabled = !bot_enabled;
                    AddLog(bot_enabled ? "Bot started" : "Bot paused");
                }
                
                ImGui::Separator();
                if (HoverTextButton(LANGUAGE == "RU" ? "Диагностика" : "Diagnostics", !g_ShowDiagnostics, ImVec2(150, 30))) g_ShowDiagnostics = !g_ShowDiagnostics;
                ImGui::Spacing();
                if (HoverTextButton(LANGUAGE == "RU" ? "Панель Управления" : "Control Panel", !g_ShowControlPanel, ImVec2(150, 30))) g_ShowControlPanel = !g_ShowControlPanel;
                ImGui::Spacing();
                if (HoverTextButton(LANGUAGE == "RU" ? "Настройки" : "Settings", !g_ShowSettings, ImVec2(150, 30))) g_ShowSettings = !g_ShowSettings;
                ImGui::Spacing();
                ImGui::Separator();
                if (HoverTextButton(LANGUAGE == "RU" ? "ВЫХОД" : "EXIT", true, ImVec2(150, 30), true))
                    g_bRunning = false; // Exit app
            }
            ImGui::End();
        }
        
        // CONTROL PANEL
        if (g_ShowControlPanel)
        {
            ImGuiWindowFlags panelFlags = (g_PinControlPanel ? (ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse) : 0) | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize;
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(5, 5));
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(5, 5));
            if (ImGui::Begin(LANGUAGE == "RU" ? "Панель Управления##Ctrl" : "Control Panel", &g_ShowControlPanel, panelFlags))
            {
                HandlePinning(g_PinControlPanel);
                // J + L as one button
                if (HoverTextButton(" J+L ", auto_j)) 
                {
                    auto_j = !auto_j;
                    auto_l = auto_j;
                    AddLog(auto_j ? "J+L enabled" : "J+L disabled");
                }
                ImGui::SameLine();
                
                // LIKES
                if (HoverTextButton(" LIKES ", auto_likes)) 
                {
                    auto_likes = !auto_likes;
                    AddLog(auto_likes ? "LIKES enabled" : "LIKES disabled");
                }
                ImGui::SameLine();
                
                // Players
                std::string p_str = std::to_string(num_players);
                if (HoverTextButton(p_str.c_str(), true))
                {
                    num_players++;
                    if (num_players > 3) num_players = 1;
                    AddLog("Players: " + std::to_string(num_players));
                }
                ImGui::SameLine();
                
                // Mode
                if (HoverTextButton(mode_immediate ? (LANGUAGE == "RU" ? " МГНОВ " : " IMMED ") : (LANGUAGE == "RU" ? " НАКОП " : " ACCUM "), true))
                {
                    mode_immediate = !mode_immediate;
                    AddLog(mode_immediate ? "Mode: Immediate" : "Mode: Accum");
                }
            }
            ImGui::End();
            ImGui::PopStyleVar(2);
        }
        
    } // End of else (!g_CalibrationMode)

    if (g_ShowSettings)
    {
        ImGuiWindowFlags sFlags = (g_PinSettings ? (ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse) : 0) | ImGuiWindowFlags_AlwaysAutoResize;
        if (ImGui::Begin(LANGUAGE == "RU" ? "Настройки" : "Settings", &g_ShowSettings, sFlags))
        {
            HandlePinning(g_PinSettings);
            ImGui::Text(LANGUAGE == "RU" ? "Язык / Language:" : "Language / Язык:");
            int langIdx = (LANGUAGE == "RU") ? 1 : 0;
            const char* items[] = { "EN", "RU" };
            
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            bool langChanged = ImGui::Combo("##Lang", &langIdx, items, 2);
            ImGui::PopStyleColor();
            
            if (langChanged)
            {
                LANGUAGE = (langIdx == 1) ? "RU" : "EN";
                SaveConfig();
            }
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Text(LANGUAGE == "RU" ? "Настройки производительности" : "Performance");
            
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            bool pollChanged = ImGui::SliderInt("##PollRate", &VISION_POLL_RATE_MS, 10, 500, "%d ms");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::Text(LANGUAGE == "RU" ? "Частота проверки зрения" : "Vision Poll Rate");
            if (pollChanged) SaveConfig();
            
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
            bool speedChanged = ImGui::SliderFloat("##ClickSpeed", &CLICK_SPEED_HZ, 0.1f, 10.0f, "%.1f Hz");
            ImGui::PopStyleColor();
            ImGui::SameLine();
            ImGui::Text(LANGUAGE == "RU" ? "Скорость кликов J и L" : "Click Speed J & L");
            if (speedChanged) SaveConfig();
            
            ImGui::Spacing();
            ImGui::Separator();
            
            if (HoverTextButton(LANGUAGE == "RU" ? " НАЧАТЬ КАЛИБРОВКУ " : " START CALIBRATION ", true))
            {
                g_CalibrationMode = true;
                AddLog(LANGUAGE == "RU" ? "Режим калибровки запущен." : "Calibration mode started.");
            }

            if (HoverTextButton(LANGUAGE == "RU" ? " ПРИВЯЗКА КЛАВИШ " : " KEYBINDINGS ", !g_ShowKeybindings))
            {
                g_ShowKeybindings = !g_ShowKeybindings;
            }
        }
        ImGui::End();
    }
    
    // DIAGNOSTICS
    if (g_ShowDiagnostics)
    {
        ImGuiWindowFlags dFlags = (g_PinDiagnostics ? (ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse) : 0);
        if (ImGui::Begin(LANGUAGE == "RU" ? "Диагностика" : "Diagnostics", &g_ShowDiagnostics, dFlags))
        {
            HandlePinning(g_PinDiagnostics);
            ImGui::TextColored(ImVec4(0,1,1,1), LANGUAGE == "RU" ? "Системные логи" : "System Logs");
            ImGui::BeginChild("LogRegion", ImVec2(0, 150), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
            for (const auto& log : g_Logs)
            {
                ImGui::TextWrapped("%s", log.c_str());
            }
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);
            ImGui::EndChild();
            
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0,1,1,1), LANGUAGE == "RU" ? "Статус бота" : "Bot Status");
            ImGui::Text(LANGUAGE == "RU" ? "Спам J: %s" : "Spamming J: %s", g_bSpammingJ ? (LANGUAGE == "RU" ? "ДА" : "YES") : (LANGUAGE == "RU" ? "НЕТ" : "NO"));
            ImGui::Text(LANGUAGE == "RU" ? "Спам L: %s" : "Spamming L: %s", g_bSpammingL ? (LANGUAGE == "RU" ? "ДА" : "YES") : (LANGUAGE == "RU" ? "НЕТ" : "NO"));
            ImGui::Text(LANGUAGE == "RU" ? "Цвет мужской полосы: %s" : "Male Bar Color: %s", g_MaleBarColor.c_str());
            ImGui::Text(LANGUAGE == "RU" ? "Статус авто-лайков: %s" : "Auto-Likes Status: %s", g_LikesStatus.c_str());

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0,1,1,1), LANGUAGE == "RU" ? "Сенсоры зрения" : "Vision Sensors");
            ImGui::Text(LANGUAGE == "RU" ? "Мужская стрелка желтая: %s" : "Male Arrow Yellow: %s", IsMaleArrowYellow() ? (LANGUAGE == "RU" ? "ДА" : "YES") : (LANGUAGE == "RU" ? "НЕТ" : "NO"));
            ImGui::Text(LANGUAGE == "RU" ? "Женская стрелка желтая: %s" : "Female Arrow Yellow: %s", IsFemaleArrowYellow() ? (LANGUAGE == "RU" ? "ДА" : "YES") : (LANGUAGE == "RU" ? "НЕТ" : "NO"));
            ImGui::Text(LANGUAGE == "RU" ? "Свечение кнопки сердца: %s" : "Heart Button Glow: %s", IsHeartButtonPresent() ? (LANGUAGE == "RU" ? "ДА" : "YES") : (LANGUAGE == "RU" ? "НЕТ" : "NO"));
            ImGui::Text(LANGUAGE == "RU" ? "Начало шкалы (5): %s" : "Gauge First (5): %s", IsGaugeSliceBlue(GAUGE_POS_FIRST) ? (LANGUAGE == "RU" ? "ДА" : "YES") : (LANGUAGE == "RU" ? "НЕТ" : "NO"));
            ImGui::Text(LANGUAGE == "RU" ? "Конец шкалы (6): %s" : "Gauge Last (6): %s", IsGaugeSliceBlue(GAUGE_POS_LAST) ? (LANGUAGE == "RU" ? "ДА" : "YES") : (LANGUAGE == "RU" ? "НЕТ" : "NO"));
        }
        ImGui::End();
    }

    if (g_ShowKeybindings)
    {
        ImGuiWindowFlags kFlags = (g_PinKeybindings ? (ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse) : 0) | ImGuiWindowFlags_AlwaysAutoResize;
        if (ImGui::Begin(LANGUAGE == "RU" ? "ПРИВЯЗКА КЛАВИШ" : "KEYBINDINGS", &g_ShowKeybindings, kFlags))
        {
            HandlePinning(g_PinKeybindings);
            auto DrawKeyBind = [](const char* labelRu, const char* labelEn, KeyBind& bind) {
                if (g_CurrentBinding == &bind) {
                    HoverTextButton(LANGUAGE == "RU" ? "Нажмите комбинацию клавиш..." : "Press key combination...", false);
                } else {
                    std::string label = (LANGUAGE == "RU" ? labelRu : labelEn);
                    std::string btnText = label + " [" + KeyBindToString(bind) + "]";
                    if (HoverTextButton(btnText.c_str(), true)) g_CurrentBinding = &bind;
                }
            };

            ImGui::TextColored(ImVec4(0,1,1,1), LANGUAGE == "RU" ? "Общие привязки" : "General Binds");
            DrawKeyBind("Включить меню", "Menu Toggle", BIND_MENU_TOGGLE);
            DrawKeyBind("Видимость приложения", "App Visibility", BIND_APP_VISIBILITY);
            DrawKeyBind("Спам кнопки J", "Spam J Key", BIND_SPAM_J);
            DrawKeyBind("Спам кнопки L", "Spam L Key", BIND_SPAM_L);
            DrawKeyBind("Закрепить/Открепить окно", "Pin/Unpin Window", BIND_PIN_WINDOW);
            DrawKeyBind("Закрепить/Открепить все окна", "Pin/Unpin All Windows", BIND_PIN_ALL);

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextColored(ImVec4(0,1,1,1), LANGUAGE == "RU" ? "Привязки калибровки" : "Calibration Binds");
            DrawKeyBind("Позиция игрока 1", "Player 1 Pos", BIND_CALIB_P1);
            DrawKeyBind("Позиция игрока 2", "Player 2 Pos", BIND_CALIB_P2);
            DrawKeyBind("Позиция игрока 3", "Player 3 Pos", BIND_CALIB_P3);
            DrawKeyBind("Позиция лайка 1", "Like 1 Pos", BIND_CALIB_L1);
            DrawKeyBind("Позиция лайка 2", "Like 2 Pos", BIND_CALIB_L2);
            DrawKeyBind("Позиция лайка 3", "Like 3 Pos", BIND_CALIB_L3);
            DrawKeyBind("Начало шкалы (1/10)", "Gauge Start (1/10)", BIND_CALIB_GAUGE_START);
            DrawKeyBind("Конец шкалы (10/10)", "Gauge End (10/10)", BIND_CALIB_GAUGE_END);
            DrawKeyBind("Мужская полоса стамины", "Male Stamina Bar", BIND_CALIB_MALE_BAR);
            DrawKeyBind("Женская стрелка стамины", "Female Stamina Arrow", BIND_CALIB_FEMALE_ARROW);
            DrawKeyBind("Кнопка сердца (L)", "Heart Button (L)", BIND_CALIB_HEART);
            DrawKeyBind("Мужская стрелка стамины", "Male Stamina Arrow", BIND_CALIB_MALE_ARROW);
        }
        ImGui::End();
    }
}
