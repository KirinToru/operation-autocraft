#include <thread>
#include <atomic>
#include <windows.h>
#include <chrono>
#include "input_hook.h"
#include "config.h"
#include "vision.h"
#include "gui.h"

std::thread g_BotThread;
extern std::atomic<bool> g_bRunning;
extern HWND g_hwnd;

bool g_bUpperYellow = false;
bool g_bLowerYellow = false;
std::string g_BarState = "";
int g_GaugePixels = 0;

// Config State variables are now in config.cpp/config.h

struct EnumData {
    HWND hwnd;
};

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    EnumData* data = (EnumData*)lParam;
    char title[256];
    if (GetWindowTextA(hwnd, title, sizeof(title))) {
        std::string s(title);
        if (s.find("Fallen") != std::string::npos || 
            s.find("Doll") != std::string::npos || 
            s.find("Lovecraft") != std::string::npos || 
            s.find("Paralogue") != std::string::npos) {
            data->hwnd = hwnd;
            return FALSE; // Stop enumerating
        }
    }
    char cls[256];
    if (GetClassNameA(hwnd, cls, sizeof(cls))) {
        std::string s(cls);
        if (s == "UnrealWindow") {
            data->hwnd = hwnd;
        }
    }
    return TRUE;
}

HWND GetGameWindow()
{
    EnumData data = { NULL };
    EnumWindows(EnumWindowsProc, (LPARAM)&data);
    return data.hwnd;
}

bool IsGameActive()
{
    HWND fg = GetForegroundWindow();
    if (!fg) return false;
    char title[256];
    GetWindowTextA(fg, title, sizeof(title));
    std::string s(title);
    
    if (s.find("Fallen") != std::string::npos || 
        s.find("Doll") != std::string::npos || 
        s.find("Lovecraft") != std::string::npos || 
        s.find("Paralogue") != std::string::npos) {
        return true;
    }
    char cls[256];
    if (GetClassNameA(fg, cls, sizeof(cls))) {
        std::string sCls(cls);
        if (sCls == "UnrealWindow") {
            return true;
        }
    }
    return (fg == g_hwnd);
}

void SendKey(WORD vk)
{
    HWND hwnd = GetGameWindow();
    if (!hwnd) return;
    
    UINT scanCode = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
    LPARAM lParamDown = 1 | (scanCode << 16);
    LPARAM lParamUp = 1 | (scanCode << 16) | (1 << 30) | (1 << 31);
    
    PostMessage(hwnd, WM_KEYDOWN, vk, lParamDown);
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    PostMessage(hwnd, WM_KEYUP, vk, lParamUp);
}

void BotLogicLoop()
{
    LoadConfig();
    InitVision();

    auto last_heart_press = std::chrono::steady_clock::now();
    auto last_j_press = std::chrono::steady_clock::now();
    auto last_start_press = std::chrono::steady_clock::now();
    
    bool is_spamming_j = false;
    bool is_spamming_l = false;

    while (g_bRunning)
    {
        HWND gameHwnd = GetGameWindow();
        if (!bot_enabled || !gameHwnd || g_CalibrationMode)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            continue;
        }

        if (CaptureFrame())
        {
            auto now = std::chrono::steady_clock::now();
            
            // Diagnostics
            g_bSpammingJ = is_spamming_j;
            g_bSpammingL = is_spamming_l;
            g_MaleBarColor = GetMaleBarState();

            // Calculate click interval
            int click_interval_ms = CLICK_SPEED_MS;

            // AHK Signal #1 (L Priority)
            bool heartVisible = IsHeartButtonPresent();
            bool yellowMarks = IsMaleArrowYellow() && IsFemaleArrowYellow();
            bool should_press_l = (heartVisible || yellowMarks) && auto_l;

            if (should_press_l)
            {

                is_spamming_j = false; // L overrides J
                if (!is_spamming_l) {
                    AddLog(LANGUAGE == "RU" ? "Начато нажатие L" : "Started spamming L");
                    is_spamming_l = true;
                }
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_heart_press).count();
                if (elapsed >= click_interval_ms)
                {
                    SendKey((WORD)BIND_SPAM_L.vkCode);
                    last_heart_press = now;
                }
            }
            else
            {
                if (is_spamming_l) {
                    AddLog(LANGUAGE == "RU" ? "Закончено нажатие L" : "Stopped spamming L");
                    is_spamming_l = false;
                }
            }
            
            // AHK Signal #2 (J)
            if (auto_j && !should_press_l)
            {
                bool should_press_j = false;
                int j_interval = click_interval_ms;
                
                std::string male_state = g_MaleBarColor;
                
                if (male_state == "FULL_PINK") {
                    should_press_j = true;
                    j_interval = click_interval_ms; // FullBarJInterval
                }
                else if (male_state == "FULL_GREY" || male_state == "UNKNOWN") {
                    should_press_j = true;
                    j_interval = click_interval_ms; // EmptyBarJInterval
                }
                else if (male_state == "PARTIAL_PINK") {
                    // Do nothing, wait for it to reach the end
                    should_press_j = false;
                }
                else if (area_has_start_prompt()) {
                    should_press_j = true;
                    j_interval = 90; // StartPressInterval
                }
                
                if (should_press_j)
                {
                    if (!is_spamming_j) {
                        AddLog(LANGUAGE == "RU" ? "Начато нажатие J" : "Started spamming J");
                        is_spamming_j = true;
                    }
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_j_press).count();
                    if (elapsed >= j_interval)
                    {
                        SendKey((WORD)BIND_SPAM_J.vkCode);
                        last_j_press = now;
                    }
                }
                else
                {
                    if (is_spamming_j) {
                        AddLog(LANGUAGE == "RU" ? "Закончено нажатие J" : "Stopped spamming J");
                        is_spamming_j = false;
                    }
                }
            }
            else if (is_spamming_j) {
                AddLog(LANGUAGE == "RU" ? "Закончено нажатие J" : "Stopped spamming J");
                is_spamming_j = false;
            }
            
            // AUTO LIKES
            if (auto_likes)
            {
                bool is_calibrated = false;
                for (int i = 0; i < num_players; ++i) {
                    if (PLAYER_POSITIONS[i].x != 0 || PLAYER_POSITIONS[i].y != 0) {
                        is_calibrated = true;
                        break;
                    }
                }
                
                if (!is_calibrated || (GAUGE_POS_FIRST.x == 0 && GAUGE_POS_FIRST.y == 0) || (GAUGE_POS_LAST.x == 0 && GAUGE_POS_LAST.y == 0))
                {
                    if (g_LikesStatus != "NEEDS CALIBRATION") {
                        AddLog(LANGUAGE == "RU" ? "Откалибруйте игроков и шкалу лайков!" : "Calibrate players and gauge first!");
                        g_LikesStatus = "NEEDS CALIBRATION";
                    }
                }
                else
                {
                    static auto last_likes_run = std::chrono::steady_clock::now();
                    auto elapsed_likes = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_likes_run).count();
                    
                    if (elapsed_likes > 500) // check every 500ms (2Hz)
                    {
                        g_LikesStatus = "CHECKING IDEA";
                        
                        bool should_distribute = false;

                        if (mode_immediate) {
                            if (IsGaugeSliceBlue(GAUGE_POS_FIRST)) {
                                should_distribute = true;
                            }
                        } else {
                            if (IsGaugeSliceBlue(GAUGE_POS_LAST)) {
                                should_distribute = true;
                            }
                        }

                        if (should_distribute)
                        {
                            g_LikesStatus = "DISTRIBUTING LIKES";
                            AddLog(LANGUAGE == "RU" ? "[Авто Лайки] Раздача лайков!" : "[Auto Likes] Distributing likes!");
                            
                            // UE4 ignores background mouse clicks, so we must move the physical cursor

                            static int current_player = 0;
                            int likes_to_give = mode_immediate ? 1 : num_players;
                            
                            for (int i = 0; i < likes_to_give; ++i)
                            {
                                // Find next valid player
                                int attempts = 0;
                                while (PLAYER_POSITIONS[current_player].x == 0 && PLAYER_POSITIONS[current_player].y == 0 && attempts < 4) {
                                    current_player = (current_player + 1) % num_players;
                                    attempts++;
                                }
                                
                                if (attempts >= 4) {
                                    AddLog("[Debug] No valid players found!");
                                    break; 
                                }

                                int px = PLAYER_POSITIONS[current_player].x;
                                int py = PLAYER_POSITIONS[current_player].y;
                                int lx = LIKE_POSITIONS[current_player].x;
                                int ly = LIKE_POSITIONS[current_player].y;
                                
                                AddLog(LANGUAGE == "RU" ? ("[Дебаг] Лайк игроку " + std::to_string(current_player + 1)) : ("[Debug] Liking player " + std::to_string(current_player + 1)));
                                
                                auto MoveMouseAbsolute = [](int x, int y) {
                                    INPUT input = {0};
                                    input.type = INPUT_MOUSE;
                                    input.mi.dx = (x * 65535) / GetSystemMetrics(SM_CXSCREEN);
                                    input.mi.dy = (y * 65535) / GetSystemMetrics(SM_CYSCREEN);
                                    input.mi.dwFlags = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
                                    SendInput(1, &input, sizeof(INPUT));
                                };

                                // Open player menu (Physical click)
                                MoveMouseAbsolute(px, py);
                                INPUT input = {0};
                                input.type = INPUT_MOUSE;
                                input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
                                SendInput(1, &input, sizeof(INPUT));
                                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                                input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
                                SendInput(1, &input, sizeof(INPUT));

                                std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Increased for reliability

                                // Click like button (Physical click)
                                MoveMouseAbsolute(lx, ly);
                                input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
                                SendInput(1, &input, sizeof(INPUT));
                                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                                input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
                                SendInput(1, &input, sizeof(INPUT));
                                
                                std::this_thread::sleep_for(std::chrono::milliseconds(400)); // Wait before next action
                                
                                current_player = (current_player + 1) % num_players;
                            }
                            
                        }
                        
                        last_likes_run = std::chrono::steady_clock::now();
                        g_LikesStatus = "WAITING";
                    }
                }
            }
            else {
                g_LikesStatus = "IDLE";
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(VISION_POLL_RATE_MS));
    }

    CleanupVision();
    SaveConfig();
}

void InitBotLogic()
{
    g_BotThread = std::thread(BotLogicLoop);
}

void CleanupBotLogic()
{
    g_bRunning = false;
    if (g_BotThread.joinable())
    {
        g_BotThread.join();
    }
}
