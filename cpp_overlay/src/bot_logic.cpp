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

void SendKey(WORD vk)
{
    INPUT input = {0};
    input.type = INPUT_KEYBOARD;
    
    // Convert Virtual Key to Hardware Scan Code for layout-independence
    UINT scanCode = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
    input.ki.wScan = static_cast<WORD>(scanCode);
    input.ki.dwFlags = KEYEVENTF_SCANCODE;
    
    SendInput(1, &input, sizeof(INPUT));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
    SendInput(1, &input, sizeof(INPUT));
}

bool IsGameActive()
{
    HWND fg = GetForegroundWindow();
    if (!fg) return false;
    char title[256];
    GetWindowTextA(fg, title, sizeof(title));
    std::string sTitle(title);
    return (sTitle.find("FallenDoll") != std::string::npos || 
            sTitle.find("Paralogue") != std::string::npos ||
            fg == g_hwnd);
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
        if (!bot_enabled || !IsGameActive() || g_CalibrationMode)
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
            int click_interval_ms = (int)(1000.0f / (CLICK_SPEED_HZ > 0.1f ? CLICK_SPEED_HZ : 0.1f));

            // Priority: L condition Check
            bool l_condition_met = false;
            if (auto_l)
            {
                l_condition_met = (IsMaleArrowYellow() && IsFemaleArrowYellow()) || IsHeartButtonPresent();
            }

            // AUTO L
            if (auto_l)
            {
                if (l_condition_met)
                {
                    if (!is_spamming_l) {
                        AddLog(LANGUAGE == "RU" ? "Начато нажатие L" : "Started spamming L");
                        is_spamming_l = true;
                    }
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_heart_press).count();
                    if (elapsed >= click_interval_ms)
                    {
                        SendKey(BIND_SPAM_L.vkCode);
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
            }
            else if (is_spamming_l) {
                AddLog(LANGUAGE == "RU" ? "Закончено нажатие L (отключено)" : "Stopped spamming L (disabled)");
                is_spamming_l = false;
            }
            
            // AUTO J (ONLY if L condition is NOT met!)
            if (auto_j && !l_condition_met)
            {
                bool should_press_j = false;
                std::string male_state = g_MaleBarColor;
                
                if (male_state == "FULL_PINK" || male_state == "FULL_GREY")
                {
                    should_press_j = true;
                }
                
                if (should_press_j)
                {
                    if (!is_spamming_j) {
                        AddLog(LANGUAGE == "RU" ? "Начато нажатие J" : "Started spamming J");
                        is_spamming_j = true;
                    }
                    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_j_press).count();
                    if (elapsed >= click_interval_ms)
                    {
                        SendKey(BIND_SPAM_J.vkCode);
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
                    
                    if (elapsed_likes > 10000) // check every 10 seconds
                    {
                        g_LikesStatus = "CHECKING GAUGE";
                        
                        bool should_distribute = false;
                        int likes_to_give = 0;

                        if (mode_immediate) {
                            if (IsGaugeSliceBlue(GAUGE_POS_FIRST)) {
                                should_distribute = true;
                                likes_to_give = 10; // We'll loop but re-check inside
                            }
                        } else {
                            if (IsGaugeSliceBlue(GAUGE_POS_LAST)) {
                                should_distribute = true;
                                likes_to_give = 10;
                            }
                        }

                        if (should_distribute)
                        {
                            g_LikesStatus = "DISTRIBUTING LIKES";
                            AddLog(LANGUAGE == "RU" ? "[Авто Лайки] Раздача лайков!" : "[Auto Likes] Distributing likes!");
                            
                            BlockInput(TRUE);
                            POINT orig_pos;
                            GetCursorPos(&orig_pos);

                            int current_player = 0;
                            
                            for (int like_i = 0; like_i < likes_to_give; ++like_i)
                            {
                                // In immediate mode, we re-verify if the gauge is still blue before each like!
                                if (mode_immediate && like_i > 0)
                                {
                                    CaptureFrame(); // Get latest frame
                                    if (!IsGaugeSliceBlue(GAUGE_POS_FIRST)) {
                                        break; // Gauge is empty, stop distributing!
                                    }
                                }
                                
                                // Find next valid player
                                int attempts = 0;
                                while (PLAYER_POSITIONS[current_player].x == 0 && PLAYER_POSITIONS[current_player].y == 0 && attempts < 4) {
                                    current_player = (current_player + 1) % num_players;
                                    attempts++;
                                }
                                
                                if (attempts >= 4) break; // No valid players

                                int px = PLAYER_POSITIONS[current_player].x;
                                int py = PLAYER_POSITIONS[current_player].y;
                                int lx = LIKE_POSITIONS[current_player].x;
                                int ly = LIKE_POSITIONS[current_player].y;
                                
                                // Open player menu
                                SetCursorPos(px, py);
                                INPUT input = {0};
                                input.type = INPUT_MOUSE;
                                input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
                                SendInput(1, &input, sizeof(INPUT));
                                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                                input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
                                SendInput(1, &input, sizeof(INPUT));

                                std::this_thread::sleep_for(std::chrono::milliseconds(600));

                                // Click like button
                                SetCursorPos(lx, ly);
                                input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
                                SendInput(1, &input, sizeof(INPUT));
                                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                                input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
                                SendInput(1, &input, sizeof(INPUT));
                                
                                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                                
                                current_player = (current_player + 1) % num_players;
                            }
                            
                            SetCursorPos(orig_pos.x, orig_pos.y);
                            BlockInput(FALSE);
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
