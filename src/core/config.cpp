#include "core/config.h"
#include <fstream>
#include <iostream>
#include <cmath>
#include <windows.h>
#include "nlohmann/json.hpp"

// Global config variables
int num_players = 3;
bool mode_immediate = true;
bool bot_enabled = false;
bool auto_j = false;
bool auto_l = false;
bool auto_likes = false;
bool auto_detect_coords = true;
float g_WindowScale = 1.0f;

int VISION_POLL_RATE_MS = 50;
int CLICK_SPEED_MS = 1;

int GAUGE_THRESHOLD = 200;
std::string LANGUAGE = "EN"; // Supported: EN, RU

// Reference 1920x1080 manual calibration points
Point2D MANUAL_PLAYER_POSITIONS[4] = { {79, 186}, {75, 302}, {76, 375}, {0, 0} };
Point2D MANUAL_LIKE_POSITIONS[4] = { {73, 241}, {74, 315}, {73, 385}, {0, 0} };

Point2D MANUAL_GAUGE_POS_FIRST = { 1834, 33 };
Point2D MANUAL_GAUGE_POS_LAST = { 1817, 33 };
Point2D MANUAL_MALE_BAR_POS = { 138, 914 };
Point2D MANUAL_MALE_BAR_END_POS = { 353, 917 };
Point2D MANUAL_MALE_ARROW_POS = { 313, 922 };
Point2D MANUAL_FEMALE_ARROW_POS = { 311, 822 };
Point2D MANUAL_HEART_L_POS = { 460, 797 };

// Active coordinates used by bot logic and vision
Point2D PLAYER_POSITIONS[4] = { {79, 186}, {75, 302}, {76, 375}, {0, 0} };
Point2D LIKE_POSITIONS[4] = { {73, 241}, {74, 315}, {73, 385}, {0, 0} };

Point2D GAUGE_POS_FIRST = { 1834, 33 };
Point2D GAUGE_POS_LAST = { 1817, 33 };
Point2D MALE_BAR_POS = { 138, 914 };
Point2D MALE_BAR_END_POS = { 353, 917 };
Point2D MALE_ARROW_POS = { 313, 922 };
Point2D FEMALE_ARROW_POS = { 311, 822 };
Point2D HEART_L_POS = { 460, 797 };

void RestoreManualCoordinates()
{
    for (int i = 0; i < 4; i++) {
        PLAYER_POSITIONS[i] = MANUAL_PLAYER_POSITIONS[i];
        LIKE_POSITIONS[i] = MANUAL_LIKE_POSITIONS[i];
    }
    GAUGE_POS_FIRST = MANUAL_GAUGE_POS_FIRST;
    GAUGE_POS_LAST = MANUAL_GAUGE_POS_LAST;
    MALE_BAR_POS = MANUAL_MALE_BAR_POS;
    MALE_BAR_END_POS = MANUAL_MALE_BAR_END_POS;
    MALE_ARROW_POS = MANUAL_MALE_ARROW_POS;
    FEMALE_ARROW_POS = MANUAL_FEMALE_ARROW_POS;
    HEART_L_POS = MANUAL_HEART_L_POS;
}

void UpdateAutoCoordinates(HWND gameHwnd)
{
    static int lastW = -1;
    static int lastH = -1;
    static int lastOriginX = -99999;
    static int lastOriginY = -99999;
    static HWND lastHwnd = nullptr;

    int clientW = 1920;
    int clientH = 1080;
    POINT origin = { 0, 0 };

    if (gameHwnd && IsWindow(gameHwnd))
    {
        RECT rc = {};
        if (GetClientRect(gameHwnd, &rc))
        {
            int w = rc.right - rc.left;
            int h = rc.bottom - rc.top;
            if (w > 0 && h > 0)
            {
                clientW = w;
                clientH = h;
                ClientToScreen(gameHwnd, &origin);
            }
        }
    }

    if (clientW == lastW && clientH == lastH && origin.x == lastOriginX && origin.y == lastOriginY && gameHwnd == lastHwnd)
    {
        return;
    }

    lastW = clientW;
    lastH = clientH;
    lastOriginX = origin.x;
    lastOriginY = origin.y;
    lastHwnd = gameHwnd;

    // UE UMG DPI scale is normalized to 1080p vertical resolution
    float scale = (float)clientH / 1080.0f;
    if (scale <= 0.0f) scale = 1.0f;
    g_WindowScale = scale;

    // Player portraits & Like buttons (Top-Left anchor) -> Client coordinates
    PLAYER_POSITIONS[0] = { (int)std::round(79 * scale), (int)std::round(186 * scale) };
    PLAYER_POSITIONS[1] = { (int)std::round(75 * scale), (int)std::round(302 * scale) };
    PLAYER_POSITIONS[2] = { (int)std::round(76 * scale), (int)std::round(375 * scale) };
    PLAYER_POSITIONS[3] = { 0, 0 };

    LIKE_POSITIONS[0] = { (int)std::round(73 * scale), (int)std::round(241 * scale) };
    LIKE_POSITIONS[1] = { (int)std::round(74 * scale), (int)std::round(315 * scale) };
    LIKE_POSITIONS[2] = { (int)std::round(73 * scale), (int)std::round(385 * scale) };
    LIKE_POSITIONS[3] = { 0, 0 };

    // Gauge (Top-Right anchor) -> Screen coordinates
    int g1_x = clientW - (int)std::round((1920 - 1834) * scale);
    int g1_y = (int)std::round(33 * scale);
    GAUGE_POS_FIRST = { origin.x + g1_x, origin.y + g1_y };

    int g2_x = clientW - (int)std::round((1920 - 1817) * scale);
    int g2_y = (int)std::round(33 * scale);
    GAUGE_POS_LAST = { origin.x + g2_x, origin.y + g2_y };

    // Stamina bars, arrows, Heart button (Bottom-Left anchor) -> Screen coordinates
    int fa_x = (int)std::round(311 * scale);
    int fa_y = clientH - (int)std::round((1080 - 822) * scale);
    FEMALE_ARROW_POS = { origin.x + fa_x, origin.y + fa_y };

    int ma_x = (int)std::round(313 * scale);
    int ma_y = clientH - (int)std::round((1080 - 922) * scale);
    MALE_ARROW_POS = { origin.x + ma_x, origin.y + ma_y };

    int mb_x = (int)std::round(138 * scale);
    int mb_y = clientH - (int)std::round((1080 - 914) * scale);
    MALE_BAR_POS = { origin.x + mb_x, origin.y + mb_y };

    int mbe_x = (int)std::round(353 * scale);
    int mbe_y = clientH - (int)std::round((1080 - 917) * scale);
    MALE_BAR_END_POS = { origin.x + mbe_x, origin.y + mbe_y };

    int ht_x = (int)std::round(460 * scale);
    int ht_y = clientH - (int)std::round((1080 - 797) * scale);
    HEART_L_POS = { origin.x + ht_x, origin.y + ht_y };
}

KeyBind BIND_MENU_TOGGLE = { VK_INSERT, false, false, false };
KeyBind BIND_APP_VISIBILITY = { VK_INSERT, true, false, false }; // Shift + Insert
KeyBind BIND_SPAM_J = { 'J', false, false, false };
KeyBind BIND_SPAM_L = { 'L', false, false, false };
KeyBind BIND_PIN_WINDOW = { VK_MBUTTON, false, false, false };
KeyBind BIND_PIN_ALL = { VK_MBUTTON, true, false, false };
KeyBind BIND_START_STOP = { 'P', false, false, false };
KeyBind BIND_TOGGLE_LIKES = { 'K', false, false, false };
KeyBind BIND_TOGGLE_MODE = { VK_OEM_2, false, false, false };

KeyBind BIND_CALIB_P1 = { '1', false, false, false };
KeyBind BIND_CALIB_P2 = { '2', false, false, false };
KeyBind BIND_CALIB_P3 = { '3', false, false, false };
KeyBind BIND_CALIB_L1 = { '1', true, false, false };
KeyBind BIND_CALIB_L2 = { '2', true, false, false };
KeyBind BIND_CALIB_L3 = { '3', true, false, false };
KeyBind BIND_CALIB_GAUGE_START = { '4', false, false, false };
KeyBind BIND_CALIB_GAUGE_END = { '4', true, false, false };
KeyBind BIND_CALIB_MALE_BAR = { '5', false, false, false };
KeyBind BIND_CALIB_MEND = { '5', true, false, false };
KeyBind BIND_CALIB_MALE_ARROW = { '6', false, false, false };
KeyBind BIND_CALIB_FEMALE_ARROW = { '7', false, false, false };
KeyBind BIND_CALIB_HEART = { '8', false, false, false };

// Diagnostics State
bool g_bSpammingJ = false;
bool g_bSpammingL = false;
std::string g_MaleBarColor = "UNKNOWN";
std::string g_LikesStatus = "IDLE";


void LoadKeyBind(const nlohmann::json& j, const std::string& key, KeyBind& bind) {
    if (j.contains(key)) {
        bind.vkCode = j[key].value("vkCode", bind.vkCode);
        bind.shift = j[key].value("shift", bind.shift);
        bind.ctrl = j[key].value("ctrl", bind.ctrl);
        bind.alt = j[key].value("alt", bind.alt);
    }
}

void SaveKeyBind(nlohmann::json& j, const std::string& key, const KeyBind& bind) {
    j[key] = { {"vkCode", bind.vkCode}, {"shift", bind.shift}, {"ctrl", bind.ctrl}, {"alt", bind.alt} };
}

void LoadConfig()
{
    try {
        std::ifstream file("config.json");
        if (file.is_open()) {
            nlohmann::json j;
            file >> j;
            
            if (j.contains("auto_detect_coords")) auto_detect_coords = j["auto_detect_coords"];
            if (j.contains("LANGUAGE")) LANGUAGE = j["LANGUAGE"];
            if (j.contains("num_players")) num_players = j["num_players"];
            if (j.contains("mode_immediate")) mode_immediate = j["mode_immediate"];
            if (j.contains("bot_enabled")) bot_enabled = j["bot_enabled"];
            if (j.contains("auto_j")) auto_j = j["auto_j"];
            if (j.contains("auto_l")) auto_l = j["auto_l"];
            if (j.contains("auto_likes")) auto_likes = j["auto_likes"];
            if (j.contains("VISION_POLL_RATE_MS")) VISION_POLL_RATE_MS = j["VISION_POLL_RATE_MS"];
            if (j.contains("CLICK_SPEED_MS")) { CLICK_SPEED_MS = j["CLICK_SPEED_MS"]; }
            
            // Bounds checking
            if (VISION_POLL_RATE_MS < 1) VISION_POLL_RATE_MS = 1;
            if (VISION_POLL_RATE_MS > 3000) VISION_POLL_RATE_MS = 3000;
            if (CLICK_SPEED_MS < 1) CLICK_SPEED_MS = 1;
            if (CLICK_SPEED_MS > 3000) CLICK_SPEED_MS = 3000;

            if (j.contains("GAUGE_THRESHOLD")) GAUGE_THRESHOLD = j["GAUGE_THRESHOLD"];
            
            if (j.contains("players")) {
                for (size_t i = 0; i < j["players"].size() && i < 4; i++) {
                    MANUAL_PLAYER_POSITIONS[i].x = j["players"][i]["x"];
                    MANUAL_PLAYER_POSITIONS[i].y = j["players"][i]["y"];
                }
            }
            if (j.contains("likes")) {
                for (size_t i = 0; i < j["likes"].size() && i < 4; i++) {
                    MANUAL_LIKE_POSITIONS[i].x = j["likes"][i]["x"];
                    MANUAL_LIKE_POSITIONS[i].y = j["likes"][i]["y"];
                }
            }
            if (j.contains("gauge_first")) { MANUAL_GAUGE_POS_FIRST.x = j["gauge_first"]["x"]; MANUAL_GAUGE_POS_FIRST.y = j["gauge_first"]["y"]; }
            if (j.contains("gauge_last")) { MANUAL_GAUGE_POS_LAST.x = j["gauge_last"]["x"]; MANUAL_GAUGE_POS_LAST.y = j["gauge_last"]["y"]; }
            if (j.contains("male_bar")) { MANUAL_MALE_BAR_POS.x = j["male_bar"]["x"]; MANUAL_MALE_BAR_POS.y = j["male_bar"]["y"]; }
            if (j.contains("male_bar_end")) { MANUAL_MALE_BAR_END_POS.x = j["male_bar_end"]["x"]; MANUAL_MALE_BAR_END_POS.y = j["male_bar_end"]["y"]; }
            if (j.contains("male_arrow")) { MANUAL_MALE_ARROW_POS.x = j["male_arrow"]["x"]; MANUAL_MALE_ARROW_POS.y = j["male_arrow"]["y"]; }
            if (j.contains("female_arrow")) { MANUAL_FEMALE_ARROW_POS.x = j["female_arrow"]["x"]; MANUAL_FEMALE_ARROW_POS.y = j["female_arrow"]["y"]; }
            if (j.contains("heart_l")) { MANUAL_HEART_L_POS.x = j["heart_l"]["x"]; MANUAL_HEART_L_POS.y = j["heart_l"]["y"]; }
            
            LoadKeyBind(j, "bind_menu_toggle", BIND_MENU_TOGGLE);
            LoadKeyBind(j, "bind_app_visibility", BIND_APP_VISIBILITY);
            LoadKeyBind(j, "bind_start_stop", BIND_START_STOP);
            LoadKeyBind(j, "bind_spam_j", BIND_SPAM_J);
            LoadKeyBind(j, "bind_spam_l", BIND_SPAM_L);
            LoadKeyBind(j, "bind_toggle_likes", BIND_TOGGLE_LIKES);
            LoadKeyBind(j, "bind_toggle_mode", BIND_TOGGLE_MODE);
            LoadKeyBind(j, "bind_pin_window", BIND_PIN_WINDOW);
            LoadKeyBind(j, "bind_pin_all", BIND_PIN_ALL);
            LoadKeyBind(j, "bind_calib_p1", BIND_CALIB_P1);
            LoadKeyBind(j, "bind_calib_p2", BIND_CALIB_P2);
            LoadKeyBind(j, "bind_calib_p3", BIND_CALIB_P3);
            LoadKeyBind(j, "bind_calib_l1", BIND_CALIB_L1);
            LoadKeyBind(j, "bind_calib_l2", BIND_CALIB_L2);
            LoadKeyBind(j, "bind_calib_l3", BIND_CALIB_L3);
            LoadKeyBind(j, "bind_calib_gauge_start", BIND_CALIB_GAUGE_START);
            LoadKeyBind(j, "bind_calib_gauge_end", BIND_CALIB_GAUGE_END);
            LoadKeyBind(j, "bind_calib_male_bar", BIND_CALIB_MALE_BAR);
            LoadKeyBind(j, "bind_calib_mend", BIND_CALIB_MEND);
            LoadKeyBind(j, "bind_calib_female_arrow", BIND_CALIB_FEMALE_ARROW);
            LoadKeyBind(j, "bind_calib_heart", BIND_CALIB_HEART);
            LoadKeyBind(j, "bind_calib_male_arrow", BIND_CALIB_MALE_ARROW);
        }
    } catch (...) {
    }

    if (!auto_detect_coords) {
        RestoreManualCoordinates();
    }
}

void SaveConfig()
{
    try {
        nlohmann::json j;
        j["auto_detect_coords"] = auto_detect_coords;
        j["bot_enabled"] = bot_enabled;
        j["auto_j"] = auto_j;
        j["auto_l"] = auto_l;
        j["auto_likes"] = auto_likes;
        j["num_players"] = num_players;
        j["mode_immediate"] = mode_immediate;
        j["VISION_POLL_RATE_MS"] = VISION_POLL_RATE_MS;
        j["CLICK_SPEED_MS"] = CLICK_SPEED_MS;
        j["GAUGE_THRESHOLD"] = GAUGE_THRESHOLD;
        j["LANGUAGE"] = LANGUAGE;

        nlohmann::json players = nlohmann::json::array();
        for (int i = 0; i < 4; i++) {
            players.push_back({{"x", MANUAL_PLAYER_POSITIONS[i].x}, {"y", MANUAL_PLAYER_POSITIONS[i].y}});
        }
        j["players"] = players;

        nlohmann::json likes = nlohmann::json::array();
        for (int i = 0; i < 4; i++) {
            likes.push_back({{"x", MANUAL_LIKE_POSITIONS[i].x}, {"y", MANUAL_LIKE_POSITIONS[i].y}});
        }
        j["likes"] = likes;

        j["gauge_first"] = {{"x", MANUAL_GAUGE_POS_FIRST.x}, {"y", MANUAL_GAUGE_POS_FIRST.y}};
        j["gauge_last"] = {{"x", MANUAL_GAUGE_POS_LAST.x}, {"y", MANUAL_GAUGE_POS_LAST.y}};
        j["male_bar"] = {{"x", MANUAL_MALE_BAR_POS.x}, {"y", MANUAL_MALE_BAR_POS.y}};
        j["male_bar_end"] = {{"x", MANUAL_MALE_BAR_END_POS.x}, {"y", MANUAL_MALE_BAR_END_POS.y}};
        j["male_arrow"] = {{"x", MANUAL_MALE_ARROW_POS.x}, {"y", MANUAL_MALE_ARROW_POS.y}};
        j["female_arrow"] = {{"x", MANUAL_FEMALE_ARROW_POS.x}, {"y", MANUAL_FEMALE_ARROW_POS.y}};
        j["heart_l"] = {{"x", MANUAL_HEART_L_POS.x}, {"y", MANUAL_HEART_L_POS.y}};
        
        SaveKeyBind(j, "bind_menu_toggle", BIND_MENU_TOGGLE);
        SaveKeyBind(j, "bind_app_visibility", BIND_APP_VISIBILITY);
        SaveKeyBind(j, "bind_spam_j", BIND_SPAM_J);
        SaveKeyBind(j, "bind_spam_l", BIND_SPAM_L);
        SaveKeyBind(j, "bind_pin_window", BIND_PIN_WINDOW);
        SaveKeyBind(j, "bind_pin_all", BIND_PIN_ALL);
        SaveKeyBind(j, "bind_start_stop", BIND_START_STOP);
        SaveKeyBind(j, "bind_toggle_likes", BIND_TOGGLE_LIKES);
        SaveKeyBind(j, "bind_toggle_mode", BIND_TOGGLE_MODE);
        SaveKeyBind(j, "bind_calib_p1", BIND_CALIB_P1);
        SaveKeyBind(j, "bind_calib_p2", BIND_CALIB_P2);
        SaveKeyBind(j, "bind_calib_p3", BIND_CALIB_P3);
        SaveKeyBind(j, "bind_calib_l1", BIND_CALIB_L1);
        SaveKeyBind(j, "bind_calib_l2", BIND_CALIB_L2);
        SaveKeyBind(j, "bind_calib_l3", BIND_CALIB_L3);
        SaveKeyBind(j, "bind_calib_gauge_start", BIND_CALIB_GAUGE_START);
        SaveKeyBind(j, "bind_calib_gauge_end", BIND_CALIB_GAUGE_END);
        SaveKeyBind(j, "bind_calib_male_bar", BIND_CALIB_MALE_BAR);
        SaveKeyBind(j, "bind_calib_mend", BIND_CALIB_MEND);
        SaveKeyBind(j, "bind_calib_female_arrow", BIND_CALIB_FEMALE_ARROW);
        SaveKeyBind(j, "bind_calib_heart", BIND_CALIB_HEART);
        SaveKeyBind(j, "bind_calib_male_arrow", BIND_CALIB_MALE_ARROW);
        
        std::ofstream f("config.json");
        if (f.is_open()) {
            f << j.dump(4);
        }
    } catch (...) {
    }
}

std::string VKCodeToString(int vk)
{
    // Handle some common non-character keys explicitly
    switch (vk) {
        case VK_INSERT: return "Insert";
        case VK_HOME: return "Home";
        case VK_END: return "End";
        case VK_DELETE: return "Delete";
        case VK_PRIOR: return "Page Up";
        case VK_NEXT: return "Page Down";
        case VK_F1: return "F1";
        case VK_F2: return "F2";
        case VK_F3: return "F3";
        case VK_F4: return "F4";
        case VK_F5: return "F5";
        case VK_F6: return "F6";
        case VK_F7: return "F7";
        case VK_F8: return "F8";
        case VK_F9: return "F9";
        case VK_F10: return "F10";
        case VK_F11: return "F11";
        case VK_F12: return "F12";
        case VK_LSHIFT: return "Left Shift";
        case VK_RSHIFT: return "Right Shift";
        case VK_SHIFT: return "Shift";
        case VK_LCONTROL: return "Left Ctrl";
        case VK_RCONTROL: return "Right Ctrl";
        case VK_CONTROL: return "Ctrl";
        case VK_LMENU: return "Left Alt";
        case VK_RMENU: return "Right Alt";
        case VK_MENU: return "Alt";
        case VK_SPACE: return "Space";
        case VK_RETURN: return "Enter";
        case VK_TAB: return "Tab";
        case VK_ESCAPE: return "Esc";
        case VK_BACK: return "Backspace";
        case VK_UP: return "Up Arrow";
        case VK_DOWN: return "Down Arrow";
        case VK_LEFT: return "Left Arrow";
        case VK_RIGHT: return "Right Arrow";
    }

    // Handle alphanumeric and mapped keys
    UINT scanCode = MapVirtualKeyA(vk, MAPVK_VK_TO_VSC);
    char name[128] = {0};
    int result = GetKeyNameTextA(scanCode << 16, name, 128);
    if (result > 0) return std::string(name);
    
    // Fallback
    return "[" + std::to_string(vk) + "]";
}

std::string KeyBindToString(const KeyBind& bind)
{
    if (bind.vkCode == 0) return "None";

    std::string res = "";
    if (bind.ctrl) res += "Ctrl+";
    if (bind.alt) res += "Alt+";
    if (bind.shift) res += "Shift+";

    if (bind.vkCode == VK_MBUTTON) res += "Mouse3";
    else if (bind.vkCode == VK_XBUTTON1) res += "Mouse4";
    else if (bind.vkCode == VK_XBUTTON2) res += "Mouse5";
    else res += VKCodeToString(bind.vkCode);

    return res;
}


void SetDefaultBinds()
{
    BIND_MENU_TOGGLE = { VK_INSERT, false, false, false };
    BIND_APP_VISIBILITY = { VK_INSERT, true, false, false }; 
    BIND_SPAM_J = { 'J', false, false, false };
    BIND_SPAM_L = { 'L', false, false, false };
    BIND_PIN_WINDOW = { VK_MBUTTON, false, false, false };
    BIND_PIN_ALL = { VK_MBUTTON, true, false, false };
    BIND_START_STOP = { 'P', false, false, false };
    BIND_TOGGLE_LIKES = { 'K', false, false, false };
    BIND_TOGGLE_MODE = { VK_OEM_2, false, false, false };

    BIND_CALIB_P1 = { '1', false, false, false };
    BIND_CALIB_P2 = { '2', false, false, false };
    BIND_CALIB_P3 = { '3', false, false, false };
    BIND_CALIB_L1 = { '1', true, false, false };
    BIND_CALIB_L2 = { '2', true, false, false };
    BIND_CALIB_L3 = { '3', true, false, false };
    BIND_CALIB_GAUGE_START = { '4', false, false, false };
    BIND_CALIB_GAUGE_END = { '4', true, false, false };
    BIND_CALIB_MALE_BAR = { '5', false, false, false };
    BIND_CALIB_MEND = { '5', true, false, false };
    BIND_CALIB_MALE_ARROW = { '6', false, false, false };
    BIND_CALIB_FEMALE_ARROW = { '7', false, false, false };
    BIND_CALIB_HEART = { '8', false, false, false };
}
