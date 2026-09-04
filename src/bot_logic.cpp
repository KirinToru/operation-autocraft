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
    char cls[256];
    if (GetClassNameA(hwnd, cls, sizeof(cls))) {
        std::string sCls(cls);
        if (sCls == "UnrealWindow") {
            char title[256];
            if (GetWindowTextA(hwnd, title, sizeof(title))) {
                std::string s(title);
                if (s.find("Fallen") != std::string::npos || 
                    s.find("Doll") != std::string::npos || 
                    s.find("Lovecraft") != std::string::npos || 
                    s.find("Paralogue") != std::string::npos) {
                    data->hwnd = hwnd;
                    return FALSE; // Exact match found
                }
            }
            if (data->hwnd == NULL) {
                data->hwnd = hwnd;
            }
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
                    enum LikeState { L_IDLE, L_DISTRIBUTING, L_COOLDOWN };
                    static LikeState like_state = L_IDLE;
                    static auto like_state_time = std::chrono::steady_clock::now();
                    static int current_player = 0;
                    static HWND previous_fg = NULL;
                    
                    if (like_state == L_IDLE)
                    {
                        g_LikesStatus = "IDLE";
                        bool should_start = false;
                        if (mode_immediate) {
                            // Jesli którykolwiek jest niebieski, to znaczy ze mamy min. 1/10
                            if (IsGaugeSliceBlue(GAUGE_POS_FIRST) || IsGaugeSliceBlue(GAUGE_POS_LAST)) should_start = true;
                        } else {
                            // Wymagamy zeby poczatek i koniec byl niebieski, co gwarantuje 10/10 niezaleznie co jest po lewej a co po prawej
                            if (IsGaugeSliceBlue(GAUGE_POS_FIRST) && IsGaugeSliceBlue(GAUGE_POS_LAST)) should_start = true;
                        }
                        
                        if (should_start) {
                            like_state = L_DISTRIBUTING;
                            like_state_time = now;
                            if (GetForegroundWindow() != gameHwnd) {
                                SetForegroundWindow(gameHwnd);
                                std::this_thread::sleep_for(std::chrono::milliseconds(100)); // wait for window to surface
                            }
                            AddLog(LANGUAGE == "RU" ? "[Авто Лайки] Начинаем раздачу!" : "[Auto Likes] Starting distribution!");
                        }
                    }
                    else if (like_state == L_DISTRIBUTING)
                    {
                        g_LikesStatus = "DISTRIBUTING LIKES";
                        
                        // Sprawdzanie czy OBA pola zniknely (koniec rozdawania = 0/10) - calkowicie odporne na to, czy pasek znika od prawej czy od lewej
                        if (!IsGaugeSliceBlue(GAUGE_POS_FIRST) && !IsGaugeSliceBlue(GAUGE_POS_LAST)) {
                            like_state = L_COOLDOWN; // Przechodzimy w Cooldown zamiast od razu do IDLE (zeby animacje w grze mialy czas zgasnac)
                            like_state_time = now;
                            AddLog(LANGUAGE == "RU" ? "[Авто Лайки] Шкала пуста, останавливаем." : "[Auto Likes] Gauge empty, stopping.");
                        }
                        // Limit czasu 25 sekund zeby zdazyl rozdac wszystkie 10+ lajkow
                        else if (std::chrono::duration_cast<std::chrono::seconds>(now - like_state_time).count() >= 25) {
                            like_state = L_COOLDOWN;
                            like_state_time = now;
                            AddLog(LANGUAGE == "RU" ? "[Авто Лайки] Лимит времени (10с). Перерыв." : "[Auto Likes] Time limit (10s). Break.");
                        }
                        else
                        {
                            // Rozdajemy dla JEDNEGO gracza, a potem puszczamy glowna petle zeby zaktualizowac obraz
                            int attempts = 0;
                            while (PLAYER_POSITIONS[current_player].x == 0 && PLAYER_POSITIONS[current_player].y == 0 && attempts < 4) {
                                current_player = (current_player + 1) % num_players;
                                attempts++;
                            }
                            
                            if (attempts < 4) {
                                int px = PLAYER_POSITIONS[current_player].x;
                                int py = PLAYER_POSITIONS[current_player].y;
                                int lx = LIKE_POSITIONS[current_player].x;
                                int ly = LIKE_POSITIONS[current_player].y;
                                
                                auto ClickMouseBackground = [gameHwnd](int x, int y) {
                                    POINT pt = { x, y };
                                    ClientToScreen(gameHwnd, &pt);
                                    
                                    // Move and click
                                    SetCursorPos(pt.x, pt.y);
                                    
                                    INPUT input = { 0 };
                                    input.type = INPUT_MOUSE;
                                    input.mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
                                    SendInput(1, &input, sizeof(INPUT));
                                    
                                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                                    
                                    input.mi.dwFlags = MOUSEEVENTF_LEFTUP;
                                    SendInput(1, &input, sizeof(INPUT));
                                };

                                ClickMouseBackground(px, py);
                                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                                ClickMouseBackground(lx, ly);
                                
                                // Znacznie wolniej, tak jak wczesniej: czekamy az animacja w grze sie zarejestruje
                                std::this_thread::sleep_for(std::chrono::milliseconds(600)); 
                                
                                current_player = (current_player + 1) % num_players;
                            }
                        }
                    }
                    else if (like_state == L_COOLDOWN)
                    {
                        g_LikesStatus = "WAITING (COOLDOWN)";
                        if (std::chrono::duration_cast<std::chrono::seconds>(now - like_state_time).count() >= 10) {
                            like_state = L_IDLE;
                            AddLog(LANGUAGE == "RU" ? "[Авто Лайки] Перерыв закончен." : "[Auto Likes] Break finished.");
                        }
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
