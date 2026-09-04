#include "ui/gui.h"
#include "imgui.h"
#include "imgui_internal.h"
#include <windows.h>
#include <string>
#include <atomic>

#include <vector>
#include <mutex>
#include <map>
#include "core/config.h"
#include "bot/vision.h"

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

struct LogEntry {
    std::string text;
    std::string timeStr;
};
std::vector<LogEntry> g_Logs;
std::mutex g_LogMutex;

#include <ctime>
#include <iomanip>
#include <sstream>

void AddLog(const std::string& message)
{
    auto t = std::time(nullptr);
    std::tm tm;
    localtime_s(&tm, &t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%H:%M:%S");

    std::lock_guard<std::mutex> lock(g_LogMutex);
    g_Logs.push_back({message, oss.str()});
    if (g_Logs.size() > 20) g_Logs.erase(g_Logs.begin());
}

std::string TranslateMaleBarColor(const std::string& color) {
    if (LANGUAGE == "RU") {
        if (color == "PINK") return "РОЗОВЫЙ";
        if (color == "PARTIAL_PINK") return "ЧАСТИЧНО РОЗОВЫЙ";
        if (color == "EMPTY") return "ПУСТО";
        if (color == "UNKNOWN") return "НЕИЗВЕСТНО";
    }
    return color;
}

std::string TranslateLikesStatus(const std::string& status) {
    if (LANGUAGE == "RU") {
        if (status == "IDLE") return "ОЖИДАНИЕ";
        if (status == "WAITING") return "ОЖИДАНИЕ КД";
        if (status == "DISTRIBUTING LIKES") return "РАЗДАЧА ЛАЙКОВ";
    }
    return status;
}


#include "ui/gui_widgets.h"
void ApplyTheme()
{
    ImGuiStyle& style = ImGui::GetStyle();
    style.Colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(12.0f/255.0f, 24.0f/255.0f, 29.0f/255.0f, 0.85f); // Translucent dark slate/navy
    style.Colors[ImGuiCol_TitleBg] = ImVec4(12.0f/255.0f, 24.0f/255.0f, 29.0f/255.0f, 0.85f);
    style.Colors[ImGuiCol_TitleBgActive] = ImVec4(12.0f/255.0f, 24.0f/255.0f, 29.0f/255.0f, 0.85f);
    
    style.WindowBorderSize = 0.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 0.0f;
    
    style.Colors[ImGuiCol_Button] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    
    style.WindowRounding = 0.0f;
    style.FrameRounding = 0.0f;
    style.ScrollbarRounding = 4.0f;
    style.ScrollbarSize = 12.0f;
    
    style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.04f, 0.06f, 0.08f, 0.5f);
    style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.35f, 0.40f, 0.45f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.45f, 0.50f, 0.55f, 1.0f);
    style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.55f, 0.60f, 0.65f, 1.0f);
}

void UpdateClickThroughState()
{
    // Make clickable ONLY if the mouse is hovering an ImGui window.
    bool shouldBeClickable = ImGui::GetIO().WantCaptureMouse;
    bool wantTextInput = ImGui::GetIO().WantTextInput;
    
    LONG_PTR exStyle = GetWindowLongPtr(g_hwnd, GWL_EXSTYLE);
    LONG_PTR newStyle = exStyle;
    
    if (shouldBeClickable)
        newStyle &= ~WS_EX_TRANSPARENT;
    else
        newStyle |= WS_EX_TRANSPARENT;
        
    if (wantTextInput)
        newStyle &= ~WS_EX_NOACTIVATE;
    else
        newStyle |= WS_EX_NOACTIVATE;
        
    if (newStyle != exStyle)
    {
        SetWindowLongPtr(g_hwnd, GWL_EXSTYLE, newStyle);
        SetWindowPos(g_hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
    }
    
    static bool wasTextInput = false;
    if (wantTextInput && !wasTextInput)
    {
        // Force focus when text input starts
        SetForegroundWindow(g_hwnd);
    }
    wasTextInput = wantTextInput;
}

bool IsGameActive(); // Defined in bot_logic.cpp
HWND GetGameWindow(); // Defined in bot_logic.cpp

void ForceTopMost()
{
    static HWND s_lastGameHwnd = NULL;
    HWND game_hwnd = GetGameWindow();
    if (game_hwnd && game_hwnd != s_lastGameHwnd)
    {
        SetWindowLongPtr(g_hwnd, GWLP_HWNDPARENT, (LONG_PTR)game_hwnd);
        s_lastGameHwnd = game_hwnd;
    }
}

void RenderGUI()
{
    if (!g_AppVisible)
    {
        // Must update click-through even when invisible, so it falls back to transparent
        LONG_PTR exStyle = GetWindowLongPtr(g_hwnd, GWL_EXSTYLE);
        if (!(exStyle & WS_EX_TRANSPARENT)) {
            SetWindowLongPtr(g_hwnd, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT);
            SetWindowPos(g_hwnd, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED | SWP_NOACTIVATE);
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

    auto CheckImGuiBind = [&](const KeyBind& bind) -> bool {
        if (bind.vkCode == 0) return false;
        
        bool shiftHeld = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        bool ctrlHeld = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
        bool altHeld = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
        
        if (!bind.shift && shiftHeld) return false;
        if (!bind.ctrl && ctrlHeld) return false;
        if (!bind.alt && altHeld) return false;
        
        if (bind.shift && !shiftHeld) return false;
        if (bind.ctrl && !ctrlHeld) return false;
        if (bind.alt && !altHeld) return false;
        
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
        ImGui::SetNextWindowSizeConstraints(ImVec2(500.0f, 0.0f), ImVec2(700.0f, FLT_MAX));
        ImGui::SetNextWindowBgAlpha(0.9f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        if (ImGui::Begin("CalibrationText", nullptr, calibFlags))
        {
            if (DrawCalibrationTitleBar(LANGUAGE == "RU" ? "РЕЖИМ КАЛИБРОВКИ" : "CALIBRATION MODE"))
            {
                g_CalibrationMode = false;
            }
            DrawGameStyleWindowBorder(IM_COL32(248, 101, 128, 255)); // Red border
            
            // Calibration items with status circles and fill animation
            static std::map<int, float> calibAnimMap; // animation timers per item index
            static std::map<int, bool> calibPrevState; // previous configured state per item
            
            auto DrawCalibItem = [&](int idx, const char* textRu, const char* textEn, bool isConfigured) {
                // Detect newly configured items and trigger animation
                bool wasConfigured = calibPrevState.count(idx) ? calibPrevState[idx] : isConfigured;
                if (isConfigured && !wasConfigured) {
                    calibAnimMap[idx] = 1.0f; // trigger flash
                }
                calibPrevState[idx] = isConfigured;
                
                // Decay animation
                float& animT = calibAnimMap[idx];
                if (animT > 0.0f) {
                    animT -= ImGui::GetIO().DeltaTime * 1.5f;
                    if (animT < 0.0f) animT = 0.0f;
                }
                
                const char* text = LANGUAGE == "RU" ? textRu : textEn;
                float rowHeight = 22.0f;
                float rowWidth = ImGui::GetWindowSize().x - 30.0f; // 15px padding each side
                ImVec2 rowPos = ImGui::GetCursorScreenPos();
                ImDrawList* dl = ImGui::GetWindowDrawList();
                
                // Draw fill animation (white sweep from left to right)
                if (animT > 0.0f) {
                    float fillWidth = rowWidth * (1.0f - animT); // expands from left
                    dl->AddRectFilled(ImVec2(rowPos.x, rowPos.y), 
                                      ImVec2(rowPos.x + fillWidth, rowPos.y + rowHeight), 
                                      IM_COL32(255, 255, 255, (int)(animT * 100.0f)));
                }
                
                // Status circle (right side)
                float circleR = 3.0f;
                ImVec2 circleP = ImVec2(rowPos.x + rowWidth - circleR - 5.0f, rowPos.y + rowHeight * 0.5f);
                if (isConfigured)
                    dl->AddCircleFilled(circleP, circleR, IM_COL32(100, 255, 100, 255)); // Green
                else
                    dl->AddCircleFilled(circleP, circleR, IM_COL32(255, 100, 100, 255)); // Red
                
                ImGui::SetCursorPosX(15.0f);
                ImGui::Text("%s", text);
            };
            
            bool p1Set = (PLAYER_POSITIONS[0].x != 0 || PLAYER_POSITIONS[0].y != 0);
            bool p2Set = (PLAYER_POSITIONS[1].x != 0 || PLAYER_POSITIONS[1].y != 0);
            bool p3Set = (PLAYER_POSITIONS[2].x != 0 || PLAYER_POSITIONS[2].y != 0);
            bool l1Set = (LIKE_POSITIONS[0].x != 0 || LIKE_POSITIONS[0].y != 0);
            bool l2Set = (LIKE_POSITIONS[1].x != 0 || LIKE_POSITIONS[1].y != 0);
            bool l3Set = (LIKE_POSITIONS[2].x != 0 || LIKE_POSITIONS[2].y != 0);
            bool gsSet = (GAUGE_POS_FIRST.x != 0 || GAUGE_POS_FIRST.y != 0);
            bool geSet = (GAUGE_POS_LAST.x != 0 || GAUGE_POS_LAST.y != 0);
            bool mbSet = (MALE_BAR_POS.x != 0 || MALE_BAR_POS.y != 0);
            bool maSet = (MALE_ARROW_POS.x != 0 || MALE_ARROW_POS.y != 0);
            bool faSet = (FEMALE_ARROW_POS.x != 0 || FEMALE_ARROW_POS.y != 0);
            bool htSet = (HEART_L_POS.x != 0 || HEART_L_POS.y != 0);
            
            DrawCalibItem(0,  (std::string("[") + KeyBindToString(BIND_CALIB_P1) + "] Позиция игрока 1").c_str(),        (std::string("[") + KeyBindToString(BIND_CALIB_P1) + "] Player 1 Position").c_str(),      p1Set);
            DrawCalibItem(1,  (std::string("[") + KeyBindToString(BIND_CALIB_P2) + "] Позиция игрока 2").c_str(),        (std::string("[") + KeyBindToString(BIND_CALIB_P2) + "] Player 2 Position").c_str(),      p2Set);
            DrawCalibItem(2,  (std::string("[") + KeyBindToString(BIND_CALIB_P3) + "] Позиция игрока 3").c_str(),        (std::string("[") + KeyBindToString(BIND_CALIB_P3) + "] Player 3 Position").c_str(),      p3Set);
            DrawCalibItem(3,  (std::string("[") + KeyBindToString(BIND_CALIB_L1) + "] Позиция лайка 1").c_str(),   (std::string("[") + KeyBindToString(BIND_CALIB_L1) + "] Like 1 Position").c_str(),  l1Set);
            DrawCalibItem(4,  (std::string("[") + KeyBindToString(BIND_CALIB_L2) + "] Позиция лайка 2").c_str(),   (std::string("[") + KeyBindToString(BIND_CALIB_L2) + "] Like 2 Position").c_str(),  l2Set);
            DrawCalibItem(5,  (std::string("[") + KeyBindToString(BIND_CALIB_L3) + "] Позиция лайка 3").c_str(),   (std::string("[") + KeyBindToString(BIND_CALIB_L3) + "] Like 3 Position").c_str(),  l3Set);
            DrawCalibItem(6,  (std::string("[") + KeyBindToString(BIND_CALIB_GAUGE_START) + "] Начало шкалы ИДЕИ").c_str(),      (std::string("[") + KeyBindToString(BIND_CALIB_GAUGE_START) + "] IDEA Start").c_str(),       gsSet);
            DrawCalibItem(7,  (std::string("[") + KeyBindToString(BIND_CALIB_GAUGE_END) + "] Конец шкалы ИДЕИ").c_str(),       (std::string("[") + KeyBindToString(BIND_CALIB_GAUGE_END) + "] IDEA Stop").c_str(),        geSet);
            DrawCalibItem(8,  (std::string("[") + KeyBindToString(BIND_CALIB_MALE_BAR) + "] Муж. полоса стамины (начало)").c_str(),  (std::string("[") + KeyBindToString(BIND_CALIB_MALE_BAR) + "] Male Stamina Bar (Start)").c_str(),       mbSet);
            DrawCalibItem(9,  (std::string("[") + KeyBindToString(BIND_CALIB_MEND) + "] Муж. полоса стамины (конец)").c_str(), (std::string("[") + KeyBindToString(BIND_CALIB_MEND) + "] Male Stamina Bar (End)").c_str(),  (MALE_BAR_END_POS.x != 0 || MALE_BAR_END_POS.y != 0));
            DrawCalibItem(10,  (std::string("[") + KeyBindToString(BIND_CALIB_MALE_ARROW) + "] Мужская стрелка стамины").c_str(), (std::string("[") + KeyBindToString(BIND_CALIB_MALE_ARROW) + "] Male Stamina Arrow").c_str(),     maSet);
            DrawCalibItem(11, (std::string("[") + KeyBindToString(BIND_CALIB_FEMALE_ARROW) + "] Женская стрелка стамины").c_str(), (std::string("[") + KeyBindToString(BIND_CALIB_FEMALE_ARROW) + "] Female Stamina Arrow").c_str(),   faSet);
            DrawCalibItem(12, (std::string("[") + KeyBindToString(BIND_CALIB_HEART) + "] Кнопка сердца").c_str(),           (std::string("[") + KeyBindToString(BIND_CALIB_HEART) + "] Heart Button").c_str(),           htSet);
            
            ImGui::SetCursorPosX(15.0f);
            ImGui::Text(LANGUAGE == "RU" ? "Нажмите ESC для выхода из калибровки." : "Press ESC to exit calibration.");
            
            ImGui::Dummy(ImVec2(10.0f, 10.0f));
            
            float btnWidth = 230.0f;
            float btnHeight = 30.0f;
            float spacing = 20.0f;
            float totalWidth = btnWidth * 2.0f + spacing;
            float windowWidth = ImGui::GetWindowSize().x;
            ImGui::SetCursorPosX((windowWidth - totalWidth) * 0.5f);
            
            // Match the border color: IM_COL32(248, 101, 128, 255)
            if (ActionConfirmButton(LANGUAGE == "RU" ? "СБРОС" : "RESET", ImVec2(btnWidth, btnHeight), IM_COL32(248, 101, 128, 255)))
            {
                for (int i=0; i<3; ++i) { PLAYER_POSITIONS[i] = {0,0}; LIKE_POSITIONS[i] = {0,0}; }
                GAUGE_POS_FIRST = {0, 0}; GAUGE_POS_LAST = {0, 0};
                MALE_BAR_POS = {0, 0}; MALE_BAR_END_POS = {0, 0};
                MALE_ARROW_POS = {0, 0}; FEMALE_ARROW_POS = {0, 0};
                HEART_L_POS = {0, 0};
                SaveConfig();
            }
            
            ImGui::SameLine(0.0f, spacing);
            
            if (ActionConfirmButton(LANGUAGE == "RU" ? "ПОДТВЕРДИТЬ" : "CONFIRM", ImVec2(btnWidth, btnHeight), IM_COL32(240, 175, 20, 255)))
            {
                g_CalibrationMode = false;
            }
            
            ImGui::Dummy(ImVec2(10.0f, 10.0f));
            
            ImGui::End();
        }
        ImGui::PopStyleVar();
        g_ShowDiagnostics = true;
    }
    else
    {
        // MAIN MENU
        if (g_ShowMainMenu)
        {
            ImGuiWindowFlags mmFlags = (g_PinMainMenu ? (ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse) : 0) | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar;
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0)); // No gap at top
            if (ImGui::Begin("MainMenuWindow", &g_ShowMainMenu, mmFlags))
            {
                if (DrawCustomTitleBar(LANGUAGE == "RU" ? "ГЛАВНОЕ МЕНЮ" : "MAIN MENU", g_PinMainMenu))
                    g_ShowMainMenu = false; // Close window if X is clicked
                    
                DrawGameStyleWindowBorder(); // Draw over the title bar
                
                HandlePinning(g_PinMainMenu);
                ImGui::Dummy(ImVec2(0, 5.0f)); // add space before first button
                ImGui::SetCursorPosX(20.0f);
                std::string bindStr = "[" + KeyBindToString(BIND_START_STOP) + "]";
                if (MenuSelectButtonWithBind(bot_enabled ? (LANGUAGE == "RU" ? "ПАУЗА" : "PAUSE") : (LANGUAGE == "RU" ? "СТАРТ" : "START"), bindStr.c_str(), bot_enabled, ImVec2(220, 40)))
                {
                    bot_enabled = !bot_enabled;
                    AddLog(bot_enabled ? "Bot started" : "Bot paused");
                }
                
                ImGui::Spacing();
                ImGui::SetCursorPosX(20.0f);
                if (MenuSelectButton(LANGUAGE == "RU" ? "ДИАГНОСТИКА" : "DIAGNOSTICS", g_ShowDiagnostics, ImVec2(220, 40))) g_ShowDiagnostics = !g_ShowDiagnostics;
                ImGui::Spacing();
                ImGui::SetCursorPosX(20.0f);
                if (MenuSelectButton(LANGUAGE == "RU" ? "ПАНЕЛЬ УПРАВЛЕНИЯ" : "CONTROL PANEL", g_ShowControlPanel, ImVec2(220, 40))) g_ShowControlPanel = !g_ShowControlPanel;
                ImGui::Spacing();
                ImGui::SetCursorPosX(20.0f);
                if (MenuSelectButton(LANGUAGE == "RU" ? "НАСТРОЙКИ" : "SETTINGS", g_ShowSettings, ImVec2(220, 40))) g_ShowSettings = !g_ShowSettings;
                ImGui::Spacing();
                ImGui::SetCursorPosX(20.0f);
                if (ActionConfirmButton(LANGUAGE == "RU" ? "ВЫХОД" : "EXIT", ImVec2(220, 40)))
                    g_bRunning = false; // Exit app
                
                ImGui::Dummy(ImVec2(0, 15.0f)); // larger gap at the bottom
            }
            ImGui::End();
            ImGui::PopStyleVar();
        }
        
        // CONTROL PANEL
        if (g_ShowControlPanel)
        {
            ImGuiWindowFlags panelFlags = (g_PinControlPanel ? (ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse) : 0) | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize;
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10)); // Adjusted padding
            if (ImGui::Begin("ControlPanelWindow", &g_ShowControlPanel, panelFlags))
            {
                if (DrawSubWindowTitleBar(LANGUAGE == "RU" ? "ПАНЕЛЬ УПРАВЛЕНИЯ" : "CONTROL PANEL", g_PinControlPanel)) g_ShowControlPanel = false;
                DrawGameStyleWindowBorder();
                HandlePinning(g_PinControlPanel);
                
                ImGui::Dummy(ImVec2(320.0f, 10.0f)); // Fixed width
                
                if (CustomCheckbox(LANGUAGE == "RU" ? "Авто J+L" : "Auto J+L", &auto_j))
                {
                    auto_l = auto_j;
                    AddLog(auto_j ? "J+L enabled" : "J+L disabled");
                }
                
                if (CustomCheckbox(LANGUAGE == "RU" ? "Авто Лайки" : "Auto Likes", &auto_likes))
                {
                    AddLog(auto_likes ? "LIKES enabled" : "LIKES disabled");
                }
                
                ImGui::Spacing();
                
                int pIdx = num_players - 1;
                std::vector<std::string> pList = {LANGUAGE == "RU" ? "1 Игрок" : "1 Player", LANGUAGE == "RU" ? "2 Игрока" : "2 Players", LANGUAGE == "RU" ? "3 Игрока" : "3 Players"};
                if (CustomSelector(LANGUAGE == "RU" ? "Количество игроков" : "Num Players", &pIdx, pList, 150.0f))
                {
                    num_players = pIdx + 1;
                    AddLog(LANGUAGE == "RU" ? ("Игроков: " + std::to_string(num_players)) : ("Players: " + std::to_string(num_players)));
                }
                
                ImGui::Spacing();
                
                int mIdx = mode_immediate ? 0 : 1;
                std::vector<std::string> mList = {LANGUAGE == "RU" ? "МГНОВЕННЫЙ" : "IMMEDIATE", LANGUAGE == "RU" ? "НАКОПИТЕЛЬНЫЙ" : "ACCUMULATED"};
                if (CustomSelector(LANGUAGE == "RU" ? "Режим кликов" : "Click Mode", &mIdx, mList, 180.0f))
                {
                    mode_immediate = (mIdx == 0);
                    AddLog(mode_immediate ? (LANGUAGE == "RU" ? "Режим: Мгновенный" : "Mode: Immediate") : (LANGUAGE == "RU" ? "Режим: Накопительный" : "Mode: Accumulated"));
                }
                
                ImGui::Dummy(ImVec2(0, 10.0f));
            }
            ImGui::End();
            ImGui::PopStyleVar();
        }
        
    } // End of else (!g_CalibrationMode)

    if (g_ShowSettings)
    {
        ImGuiWindowFlags sFlags = (g_PinSettings ? (ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse) : 0) | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
        if (ImGui::Begin("SettingsWindow", &g_ShowSettings, sFlags))
        {
            if (DrawSubWindowTitleBar(LANGUAGE == "RU" ? "НАСТРОЙКИ" : "SETTINGS", g_PinSettings)) g_ShowSettings = false;
            DrawGameStyleWindowBorder();
            HandlePinning(g_PinSettings);
            
            ImGui::Dummy(ImVec2(350.0f, 10.0f)); 
            
            int langIdx = (LANGUAGE == "RU") ? 1 : 0;
            std::vector<std::string> langs = {"English", "Русский"};
            if (CustomSelector(LANGUAGE == "RU" ? "Язык" : "Language", &langIdx, langs))
            {
                LANGUAGE = (langIdx == 1) ? "RU" : "EN";
                SaveConfig();
            }
            ImGui::Spacing();
            bool pollChanged = CustomSliderInt(LANGUAGE == "RU" ? "Частота зрения" : "Vision Poll Rate", &VISION_POLL_RATE_MS, 10, 500, "%d ms");
            if (pollChanged) SaveConfig();
            
            ImGui::Spacing();
            bool speedChanged = CustomSliderInt(LANGUAGE == "RU" ? "Скорость клика" : "Click Speed", &CLICK_SPEED_MS, 1, 3000, "%d ms");
            if (speedChanged) SaveConfig();
            
            ImGui::Dummy(ImVec2(0, 15.0f));
            
            if (CyanActionButton(LANGUAGE == "RU" ? "НАЧАТЬ КАЛИБРОВКУ" : "START CALIBRATION", ImVec2(350, 30)))
            {
                g_CalibrationMode = true;
                AddLog(LANGUAGE == "RU" ? "Режим калибровки запущен." : "Calibration mode started.");
            }
            
            ImGui::Spacing();
            
            if (CyanActionButton(LANGUAGE == "RU" ? "ПРИВЯЗКА КЛАВИШ" : "KEYBINDINGS", ImVec2(350, 30)))
            {
                g_ShowKeybindings = !g_ShowKeybindings;
            }
            ImGui::Dummy(ImVec2(0, 10.0f));
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }
    
    // DIAGNOSTICS

    if (g_ShowDiagnostics)
    {
        ImGuiWindowFlags dFlags = (g_PinDiagnostics ? (ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse) : 0) | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        if (ImGui::Begin("DiagnosticsWindow", &g_ShowDiagnostics, dFlags))
        {
            if (DrawSubWindowTitleBar(LANGUAGE == "RU" ? "\u0414\u0418\u0410\u0413\u041d\u041e\u0421\u0422\u0418\u041a\u0410" : "DIAGNOSTICS", g_PinDiagnostics)) g_ShowDiagnostics = false;
            DrawGameStyleWindowBorder();
            HandlePinning(g_PinDiagnostics);
            ImGui::SetCursorPosX(10.0f);
            ImGui::Dummy(ImVec2(360.0f, 5.0f));
            
            ImGui::SetCursorPosX(10.0f);
            ImGui::TextColored(ImVec4(0.43f, 0.77f, 0.91f, 1.0f), LANGUAGE == "RU" ? "\u0421\u0438\u0441\u0442\u0435\u043c\u043d\u044b\u0435 \u043b\u043e\u0433\u0438" : "System Logs");
            ImGui::SetCursorPosX(10.0f);
            ImGui::PushStyleColor(ImGuiCol_ChildBg, IM_COL32(8, 8, 10, 240));
            ImGui::BeginChild("LogRegion", ImVec2(350, 150), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
            
            ImGui::SetWindowFontScale(0.9f); // Make text a bit smaller
            ImGui::Dummy(ImVec2(0, 5.0f)); // Gap at top
            
            // Draw animated isometric cube pattern INSIDE log region only
            {
                ImDrawList* dl = ImGui::GetWindowDrawList();
                ImVec2 cp = ImGui::GetWindowPos();
                ImVec2 cs = ImGui::GetWindowSize();
                ImVec2 cMax(cp.x + cs.x, cp.y + cs.y);
                dl->PushClipRect(cp, cMax, true);
                
                ImU32 lineCol = IM_COL32(22, 21, 22, 255);
                float cell = 25.0f;
                float slope = 0.57735f; // tan(30 deg)
                
                // To repeat the isometric pattern seamlessly, the animation cycle must be cell * 2.0f
                float anim = fmodf((float)ImGui::GetTime() * 30.0f, cell * 2.0f);
                float yStep = cell * slope * 2.0f;
                float reach = (cMax.x - cp.x + cell * 4.0f) * slope;
                
                // 1) Vertical lines - animated moving right, shifted 3px right
                for (float x = cp.x - cell * 2.0f + anim + 3.0f; x < cMax.x + cell * 2.0f; x += cell)
                    dl->AddLine(ImVec2(x, cp.y), ImVec2(x, cMax.y), lineCol, 0.5f);
                
                // 2) Lines going down-right - animated in sync with verticals
                for (float y0 = cp.y - reach - yStep; y0 < cMax.y + yStep; y0 += yStep)
                {
                    float x1 = cp.x - cell * 2.0f;
                    float x2 = cMax.x + cell * 2.0f;
                    float yOff = -anim * slope; // shift down-right lines to move RIGHT with verticals
                    dl->AddLine(ImVec2(x1, y0 + (x1 - cp.x) * slope + yOff),
                                ImVec2(x2, y0 + (x2 - cp.x) * slope + yOff), lineCol, 0.5f);
                }
                
                // 3) Lines going up-right - animated in sync with verticals
                for (float y0 = cp.y - cell - yStep; y0 < cMax.y + reach + yStep; y0 += yStep)
                {
                    float x1 = cp.x - cell * 2.0f;
                    float x2 = cMax.x + cell * 2.0f;
                    float yOff = anim * slope; // shift up-right lines to move RIGHT with verticals
                    dl->AddLine(ImVec2(x1, y0 - (x1 - cp.x) * slope + yOff),
                                ImVec2(x2, y0 - (x2 - cp.x) * slope + yOff), lineCol, 0.5f);
                }
                
                dl->PopClipRect();
            }
            
            for (const auto& log : g_Logs)
            {
                float text_width = ImGui::GetWindowWidth() - 85.0f;
                ImGui::SetCursorPosX(10.0f);
                ImVec2 p_min = ImGui::GetCursorScreenPos(); // Get absolute position before text
                
                ImGui::PushTextWrapPos(10.0f + text_width);
                ImGui::TextWrapped("%s", log.text.c_str());
                ImGui::PopTextWrapPos();
                
                // Draw timestamp on the right side using DrawList to avoid layout issues
                ImDrawList* dl = ImGui::GetWindowDrawList();
                dl->AddText(ImVec2(p_min.x + text_width + 5.0f, p_min.y), IM_COL32(100, 100, 100, 255), log.timeStr.c_str());
            }
            
            ImGui::Dummy(ImVec2(0, 5.0f)); // Gap at bottom
            
            if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                ImGui::SetScrollHereY(1.0f);
                
            ImGui::SetWindowFontScale(1.0f); // Restore scale
            ImGui::EndChild();
            ImGui::PopStyleColor();
            
            ImGui::SetCursorPosX(10.0f);
            ImGui::Spacing();
            ImGui::SetCursorPosX(10.0f);
            ImGui::TextColored(ImVec4(0.43f, 0.77f, 0.91f, 1.0f), LANGUAGE == "RU" ? "\u0421\u0442\u0430\u0442\u0443\u0441 \u0431\u043e\u0442\u0430" : "Bot Status");
            ImGui::SetCursorPosX(10.0f);
            ImGui::Text(LANGUAGE == "RU" ? "\u0421\u043f\u0430\u043c J: %s" : "Spamming J: %s", g_bSpammingJ ? (LANGUAGE == "RU" ? "\u0414\u0410" : "YES") : (LANGUAGE == "RU" ? "\u041d\u0415\u0422" : "NO"));
            ImGui::SetCursorPosX(10.0f);
            ImGui::Text(LANGUAGE == "RU" ? "\u0421\u043f\u0430\u043c L: %s" : "Spamming L: %s", g_bSpammingL ? (LANGUAGE == "RU" ? "\u0414\u0410" : "YES") : (LANGUAGE == "RU" ? "\u041d\u0415\u0422" : "NO"));
            ImGui::SetCursorPosX(10.0f);
            ImGui::Text(LANGUAGE == "RU" ? "\u0426\u0432\u0435\u0442 \u043c\u0443\u0436\u0441\u043a\u043e\u0439 \u043f\u043e\u043b\u043e\u0441\u044b: %s" : "Male Bar Color: %s", TranslateMaleBarColor(g_MaleBarColor).c_str());
            ImGui::SetCursorPosX(10.0f);
            ImGui::Text(LANGUAGE == "RU" ? "\u0421\u0442\u0430\u0442\u0443\u0441 \u0430\u0432\u0442\u043e-\u043b\u0430\u0439\u043a\u043e\u0432: %s" : "Auto-Likes Status: %s", TranslateLikesStatus(g_LikesStatus).c_str());

            ImGui::SetCursorPosX(10.0f);
            ImGui::Spacing();
            ImGui::SetCursorPosX(10.0f);
            ImGui::TextColored(ImVec4(0.43f, 0.77f, 0.91f, 1.0f), LANGUAGE == "RU" ? "\u0421\u0435\u043d\u0441\u043e\u0440\u044b \u0437\u0440\u0435\u043d\u0438\u044f" : "Vision Sensors");
            ImGui::SetCursorPosX(10.0f);
            ImGui::Text(LANGUAGE == "RU" ? "\u041c\u0443\u0436\u0441\u043a\u0430\u044f \u0441\u0442\u0440\u0435\u043b\u043a\u0430 \u0436\u0435\u043b\u0442\u0430\u044f: %s" : "Male Arrow Yellow: %s", IsMaleArrowYellow() ? (LANGUAGE == "RU" ? "\u0414\u0410" : "YES") : (LANGUAGE == "RU" ? "\u041d\u0415\u0422" : "NO"));
            ImGui::SetCursorPosX(10.0f);
            ImGui::Text(LANGUAGE == "RU" ? "\u0416\u0435\u043d\u0441\u043a\u0430\u044f \u0441\u0442\u0440\u0435\u043b\u043a\u0430 \u0436\u0435\u043b\u0442\u0430\u044f: %s" : "Female Arrow Yellow: %s", IsFemaleArrowYellow() ? (LANGUAGE == "RU" ? "\u0414\u0410" : "YES") : (LANGUAGE == "RU" ? "\u041d\u0415\u0422" : "NO"));
            ImGui::SetCursorPosX(10.0f);
            ImGui::Text(LANGUAGE == "RU" ? "\u0421\u0432\u0435\u0447\u0435\u043d\u0438\u0435 \u043a\u043d\u043e\u043f\u043a\u0438 \u0441\u0435\u0440\u0434\u0446\u0430: %s" : "Heart Button Glow: %s", IsHeartButtonPresent() ? (LANGUAGE == "RU" ? "\u0414\u0410" : "YES") : (LANGUAGE == "RU" ? "\u041d\u0415\u0422" : "NO"));
            ImGui::SetCursorPosX(10.0f);
            ImGui::Text(LANGUAGE == "RU" ? "Начало шкалы ИДЕИ: %s" : "Gauge First: %s", IsGaugeSliceBlue(GAUGE_POS_FIRST) ? (LANGUAGE == "RU" ? "ДА" : "YES") : (LANGUAGE == "RU" ? "НЕТ" : "NO"));
            ImGui::SetCursorPosX(10.0f);
            ImGui::Text(LANGUAGE == "RU" ? "Конец шкалы ИДЕИ: %s" : "Gauge Last: %s", IsGaugeSliceBlue(GAUGE_POS_LAST) ? (LANGUAGE == "RU" ? "ДА" : "YES") : (LANGUAGE == "RU" ? "НЕТ" : "NO"));
            ImGui::Dummy(ImVec2(0, 10.0f));
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }


    if (g_ShowKeybindings)
    {
        ImGuiWindowFlags kFlags = (g_PinKeybindings ? (ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse) : 0) | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar;
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
        if (ImGui::Begin("KeybindingsWindow", &g_ShowKeybindings, kFlags))
        {
            if (DrawSubWindowTitleBar(LANGUAGE == "RU" ? "ПРИВЯЗКА КЛАВИШ" : "KEYBINDINGS", g_PinKeybindings)) g_ShowKeybindings = false;
            DrawGameStyleWindowBorder();
            HandlePinning(g_PinKeybindings);
            ImGui::Dummy(ImVec2(350.0f, 5.0f));
            
            auto DrawKeyBind = [](const char* labelRu, const char* labelEn, KeyBind& bind) {
                if (g_CurrentBinding == &bind) {
                    CyanActionButton(LANGUAGE == "RU" ? "Нажмите комбинацию клавиш..." : "Press key combination...", ImVec2(350, 30));
                } else {
                    std::string label = (LANGUAGE == "RU" ? labelRu : labelEn);
                    std::string btnText = label + " [" + KeyBindToString(bind) + "]";
                    if (CyanActionButton(btnText.c_str(), ImVec2(350, 30))) g_CurrentBinding = &bind;
                }
            };

            ImGui::TextColored(ImVec4(0.43f, 0.77f, 0.91f, 1.0f), LANGUAGE == "RU" ? "Общие привязки" : "General Binds");
            DrawKeyBind("Включить меню", "Menu Toggle", BIND_MENU_TOGGLE);
            DrawKeyBind("Видимость приложения", "App Visibility", BIND_APP_VISIBILITY);
            DrawKeyBind("Старт/Стоп", "Start/Stop", BIND_START_STOP);
            DrawKeyBind("Спам кнопки J", "Spam J Key", BIND_SPAM_J);
            DrawKeyBind("Спам кнопки L", "Spam L Key", BIND_SPAM_L);
            DrawKeyBind("Закрепить/Открепить окно", "Pin/Unpin Window", BIND_PIN_WINDOW);
            DrawKeyBind("Закрепить/Открепить все окна", "Pin/Unpin All Windows", BIND_PIN_ALL);
            DrawKeyBind(LANGUAGE == "RU" ? "Вкл/Выкл Авто-Лайки" : "Turn off/on Likes", "Turn off/on Likes", BIND_TOGGLE_LIKES);
            DrawKeyBind(LANGUAGE == "RU" ? "Режим клика (Immediate/Accumulated)" : "Scroll click mode", "Scroll click mode", BIND_TOGGLE_MODE);

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.43f, 0.77f, 0.91f, 1.0f), LANGUAGE == "RU" ? "Привязки калибровки" : "Calibration Binds");
            DrawKeyBind(LANGUAGE == "RU" ? "Позиция игрока 1" : "Player 1 Pos", "Player 1 Pos", BIND_CALIB_P1);
            DrawKeyBind(LANGUAGE == "RU" ? "Позиция игрока 2" : "Player 2 Pos", "Player 2 Pos", BIND_CALIB_P2);
            DrawKeyBind(LANGUAGE == "RU" ? "Позиция игрока 3" : "Player 3 Pos", "Player 3 Pos", BIND_CALIB_P3);
            DrawKeyBind(LANGUAGE == "RU" ? "Позиция лайка 1" : "Like 1 Pos", "Like 1 Pos", BIND_CALIB_L1);
            DrawKeyBind(LANGUAGE == "RU" ? "Позиция лайка 2" : "Like 2 Pos", "Like 2 Pos", BIND_CALIB_L2);
            DrawKeyBind(LANGUAGE == "RU" ? "Позиция лайка 3" : "Like 3 Pos", "Like 3 Pos", BIND_CALIB_L3);
            DrawKeyBind(LANGUAGE == "RU" ? "Начало шкалы ИДЕИ" : "IDEA Gauge Start", "IDEA Gauge Start", BIND_CALIB_GAUGE_START);
            DrawKeyBind(LANGUAGE == "RU" ? "Конец шкалы ИДЕИ" : "IDEA Gauge Stop", "IDEA Gauge Stop", BIND_CALIB_GAUGE_END);
            DrawKeyBind(LANGUAGE == "RU" ? "Мужская полоса стамины (начало)" : "Male Stamina Bar (Start)", "Male Stamina Bar (Start)", BIND_CALIB_MALE_BAR);
            DrawKeyBind(LANGUAGE == "RU" ? "Мужская полоса стамины (конец)" : "Male Stamina Bar (End)", "Male Stamina Bar (End)", BIND_CALIB_MEND);
            DrawKeyBind(LANGUAGE == "RU" ? "Мужская стрелка стамины" : "Male Stamina Arrow", "Male Stamina Arrow", BIND_CALIB_MALE_ARROW);
            DrawKeyBind(LANGUAGE == "RU" ? "Женская стрелка стамины" : "Female Stamina Arrow", "Female Stamina Arrow", BIND_CALIB_FEMALE_ARROW);
            DrawKeyBind(LANGUAGE == "RU" ? "Кнопка сердца" : "Heart Button", "Heart Button", BIND_CALIB_HEART);
            
            ImGui::Spacing();
            if (CyanActionButton(LANGUAGE == "RU" ? "СБРОС КЛАВИШ" : "RESET KEYBINDS", ImVec2(350, 30))) {
                SetDefaultBinds();
                SaveConfig();
                AddLog(LANGUAGE == "RU" ? "Клавиши сброшены." : "Keybinds reset.");
            }
            
            ImGui::Dummy(ImVec2(0, 10.0f));
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }
}
