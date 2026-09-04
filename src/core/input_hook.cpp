#include "core/input_hook.h"
#include <windows.h>
#include <iostream>
#include <string>
#include "ui/gui.h"
#include "core/config.h"

extern KeyBind* g_CurrentBinding;

HHOOK g_hKeyboardHook = nullptr;
HHOOK g_hMouseHook = nullptr;


bool CheckBind(const KeyBind& bind, int vkCode) {
    if (bind.vkCode != vkCode) return false;
    bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
    bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
    bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
    return bind.shift == shift && bind.ctrl == ctrl && bind.alt == alt;
}

LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0)
    {
        extern bool IsGameActive();
        bool gameActive = IsGameActive();

        KBDLLHOOKSTRUCT* pKey = (KBDLLHOOKSTRUCT*)lParam;
        bool isKeyDown = (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN);
        
        if (isKeyDown)
        {
            if (g_CurrentBinding != nullptr)
            {
                int vk = pKey->vkCode;
                // Ignore modifier keys as the main key
                if (vk != VK_SHIFT && vk != VK_LSHIFT && vk != VK_RSHIFT &&
                    vk != VK_CONTROL && vk != VK_LCONTROL && vk != VK_RCONTROL &&
                    vk != VK_MENU && vk != VK_LMENU && vk != VK_RMENU &&
                    vk != VK_CAPITAL && vk != VK_LWIN && vk != VK_RWIN)
                {
                    g_CurrentBinding->vkCode = vk;
                    g_CurrentBinding->shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
                    g_CurrentBinding->ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
                    g_CurrentBinding->alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
                    
                    g_CurrentBinding = nullptr;
                    SaveConfig();
                    return 1;
                }
            }

            if (gameActive)
            {
                if (CheckBind(BIND_APP_VISIBILITY, pKey->vkCode))
                {
                    g_AppVisible = !g_AppVisible;
                    return 1;
                }

                if (CheckBind(BIND_MENU_TOGGLE, pKey->vkCode))
                {
                    g_ShowMainMenu = !g_ShowMainMenu;
                    return 1;
                }
                
                if (CheckBind(BIND_TOGGLE_LIKES, pKey->vkCode))
                {
                    auto_likes = !auto_likes;
                    AddLog(LANGUAGE == "RU" ? (auto_likes ? "Авто-лайки ВКЛ" : "Авто-лайки ВЫКЛ") : (auto_likes ? "Auto Likes ON" : "Auto Likes OFF"));
                    return 1;
                }

                if (CheckBind(BIND_TOGGLE_MODE, pKey->vkCode))
                {
                    mode_immediate = !mode_immediate;
                    AddLog(LANGUAGE == "RU" ? (mode_immediate ? "Режим клика: ОДИНАКОВЫЙ" : "Режим клика: НАКОПЛЕННЫЙ") : (mode_immediate ? "Click Mode: IMMEDIATE" : "Click Mode: ACCUMULATED"));
                    return 1;
                }
                
                if (CheckBind(BIND_START_STOP, pKey->vkCode))
                {
                    bot_enabled = !bot_enabled;
                    AddLog(bot_enabled ? "Bot started" : "Bot paused");
                    return 1;
                }
            }
            
            // Calibration Mode Handling
            if (g_CalibrationMode && gameActive)
            {
                if (pKey->vkCode == VK_ESCAPE)
                {
                    g_CalibrationMode = false;
                    return 1;
                }
                
                if (CheckBind(BIND_CALIB_P1, pKey->vkCode) || CheckBind(BIND_CALIB_P2, pKey->vkCode) || CheckBind(BIND_CALIB_P3, pKey->vkCode) ||
                    CheckBind(BIND_CALIB_L1, pKey->vkCode) || CheckBind(BIND_CALIB_L2, pKey->vkCode) || CheckBind(BIND_CALIB_L3, pKey->vkCode))
                {
                    int index = 0;
                    bool isLike = false;
                    
                    if (CheckBind(BIND_CALIB_P1, pKey->vkCode)) { index = 0; isLike = false; }
                    else if (CheckBind(BIND_CALIB_P2, pKey->vkCode)) { index = 1; isLike = false; }
                    else if (CheckBind(BIND_CALIB_P3, pKey->vkCode)) { index = 2; isLike = false; }
                    else if (CheckBind(BIND_CALIB_L1, pKey->vkCode)) { index = 0; isLike = true; }
                    else if (CheckBind(BIND_CALIB_L2, pKey->vkCode)) { index = 1; isLike = true; }
                    else if (CheckBind(BIND_CALIB_L3, pKey->vkCode)) { index = 2; isLike = true; }

                    POINT pt;
                    GetCursorPos(&pt);
                    if (isLike)
                    {
                        LIKE_POSITIONS[index].x = pt.x;
                        LIKE_POSITIONS[index].y = pt.y;
                        AddLog(LANGUAGE == "RU" ? 
                            "Лайк " + std::to_string(index+1) + " сохранен: " + std::to_string(pt.x) + "," + std::to_string(pt.y) :
                            "Like " + std::to_string(index+1) + " saved at: " + std::to_string(pt.x) + "," + std::to_string(pt.y));
                    }
                    else
                    {
                        PLAYER_POSITIONS[index].x = pt.x;
                        PLAYER_POSITIONS[index].y = pt.y;
                        AddLog(LANGUAGE == "RU" ? 
                            "Игрок " + std::to_string(index+1) + " сохранен: " + std::to_string(pt.x) + "," + std::to_string(pt.y) :
                            "Player " + std::to_string(index+1) + " saved at: " + std::to_string(pt.x) + "," + std::to_string(pt.y));
                    }
                    SaveConfig();
                    return 1;
                }
                
                if (CheckBind(BIND_CALIB_GAUGE_START, pKey->vkCode) || CheckBind(BIND_CALIB_GAUGE_END, pKey->vkCode) || 
                    CheckBind(BIND_CALIB_MALE_BAR, pKey->vkCode) || CheckBind(BIND_CALIB_FEMALE_ARROW, pKey->vkCode) || 
                    CheckBind(BIND_CALIB_HEART, pKey->vkCode) || CheckBind(BIND_CALIB_MALE_ARROW, pKey->vkCode) ||
                    CheckBind(BIND_CALIB_MEND, pKey->vkCode))
                {
                    POINT pt;
                    GetCursorPos(&pt);
                    if (CheckBind(BIND_CALIB_GAUGE_START, pKey->vkCode)) {
                        GAUGE_POS_FIRST.x = pt.x; GAUGE_POS_FIRST.y = pt.y;
                        AddLog(LANGUAGE == "RU" ? "Шкала ИДЕИ start сохранена." : "IDEA Gauge start saved.");
                    } else if (CheckBind(BIND_CALIB_GAUGE_END, pKey->vkCode)) {
                        GAUGE_POS_LAST.x = pt.x; GAUGE_POS_LAST.y = pt.y;
                        AddLog(LANGUAGE == "RU" ? "Шкала ИДЕИ end сохранена." : "IDEA Gauge end saved.");
                    } else if (CheckBind(BIND_CALIB_MALE_BAR, pKey->vkCode)) {
                        MALE_BAR_POS.x = pt.x; MALE_BAR_POS.y = pt.y;
                        AddLog(LANGUAGE == "RU" ? "Мужская шкала выносливости сохранена." : "Male Bar saved.");
                    } else if (CheckBind(BIND_CALIB_FEMALE_ARROW, pKey->vkCode)) {
                        FEMALE_ARROW_POS.x = pt.x; FEMALE_ARROW_POS.y = pt.y;
                        AddLog(LANGUAGE == "RU" ? "Женская стрелка выносливости сохранена." : "Female Arrow saved.");
                    } else if (CheckBind(BIND_CALIB_HEART, pKey->vkCode)) {
                        HEART_L_POS.x = pt.x; HEART_L_POS.y = pt.y;
                        AddLog(LANGUAGE == "RU" ? "Кнопка Сердце (L) сохранена." : "Heart Button (L) saved.");
                    } else if (CheckBind(BIND_CALIB_MALE_ARROW, pKey->vkCode)) {
                        MALE_ARROW_POS.x = pt.x; MALE_ARROW_POS.y = pt.y;
                        AddLog(LANGUAGE == "RU" ? "Мужские стрелки выносливости сохранены." : "Male Arrow saved.");
                    } else if (CheckBind(BIND_CALIB_MEND, pKey->vkCode)) {
                        MALE_BAR_END_POS.x = pt.x; MALE_BAR_END_POS.y = pt.y;
                        AddLog(LANGUAGE == "RU" ? "Мужская шкала (конец) сохранена." : "Male Bar (End) saved.");
                    }
                    SaveConfig();
                    return 1;
                }
            }
        }
    }
    return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
}

LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0)
    {
        bool isMouseDown = (wParam == WM_LBUTTONDOWN || wParam == WM_RBUTTONDOWN || wParam == WM_MBUTTONDOWN || wParam == WM_XBUTTONDOWN);
        if (isMouseDown && g_CurrentBinding != nullptr)
        {
            int vk = 0;
            if (wParam == WM_LBUTTONDOWN) vk = VK_LBUTTON;
            else if (wParam == WM_RBUTTONDOWN) vk = VK_RBUTTON;
            else if (wParam == WM_MBUTTONDOWN) vk = VK_MBUTTON;
            else if (wParam == WM_XBUTTONDOWN) {
                MSLLHOOKSTRUCT* pMouse = (MSLLHOOKSTRUCT*)lParam;
                if (HIWORD(pMouse->mouseData) == 1) vk = VK_XBUTTON1;
                else if (HIWORD(pMouse->mouseData) == 2) vk = VK_XBUTTON2;
            }
            
            if (vk != 0)
            {
                g_CurrentBinding->vkCode = vk;
                g_CurrentBinding->shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
                g_CurrentBinding->ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
                g_CurrentBinding->alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
                
                g_CurrentBinding = nullptr;
                SaveConfig();
                return 1; // Swallow the click so it doesn't click on the UI underneath
            }
        }
    }
    return CallNextHookEx(g_hMouseHook, nCode, wParam, lParam);
}

#include <thread>
#include <atomic>

std::thread g_HookThread;
std::atomic<bool> g_bHooksRunning{false};
DWORD g_HookThreadId = 0;

void HookThreadProc()
{
    g_HookThreadId = GetCurrentThreadId();
    g_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(nullptr), 0);
    g_hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, GetModuleHandle(nullptr), 0);

    MSG msg;
    while (g_bHooksRunning && GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (g_hKeyboardHook) UnhookWindowsHookEx(g_hKeyboardHook);
    if (g_hMouseHook) UnhookWindowsHookEx(g_hMouseHook);
    g_hKeyboardHook = nullptr;
    g_hMouseHook = nullptr;
}

void InstallHooks()
{
    g_bHooksRunning = true;
    g_HookThread = std::thread(HookThreadProc);
}

void RemoveHooks()
{
    g_bHooksRunning = false;
    if (g_HookThreadId != 0)
    {
        PostThreadMessage(g_HookThreadId, WM_QUIT, 0, 0);
    }
    if (g_HookThread.joinable())
    {
        g_HookThread.join();
    }
}
