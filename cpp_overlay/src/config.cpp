#include "config.h"
#include <fstream>
#include <iostream>
#include <windows.h>
#include "nlohmann/json.hpp"

// Global config variables
int num_players = 3;
bool mode_immediate = true;
bool bot_enabled = false;
bool auto_j = false;
bool auto_l = false;
bool auto_likes = false;

int VISION_POLL_RATE_MS = 50;
float CLICK_SPEED_HZ = 2.0f;

int GAUGE_THRESHOLD = 200;
std::string LANGUAGE = "EN"; // Can be EN, PL

Point2D PLAYER_POSITIONS[4] = { {0,0}, {0,0}, {0,0}, {0,0} };
Point2D LIKE_POSITIONS[4] = { {0,0}, {0,0}, {0,0}, {0,0} };

Point2D GAUGE_POS_FIRST = {0,0};
Point2D GAUGE_POS_LAST = {0,0};
Point2D MALE_BAR_POS = {0,0};
Point2D MALE_ARROW_POS = {0,0};
Point2D FEMALE_BAR_POS = {0,0};
Point2D HEART_L_POS = {0,0};

KeyBind BIND_MENU_TOGGLE = { VK_INSERT, false, false, false };
KeyBind BIND_APP_VISIBILITY = { VK_INSERT, true, false, false }; // Shift + Insert
KeyBind BIND_SPAM_J = { 'J', false, false, false };
KeyBind BIND_SPAM_L = { 'L', false, false, false };
KeyBind BIND_PIN_WINDOW = { VK_MBUTTON, false, false, false };
KeyBind BIND_PIN_ALL = { VK_MBUTTON, true, false, false };

KeyBind BIND_CALIB_P1 = { '1', false, false, false };
KeyBind BIND_CALIB_P2 = { '2', false, false, false };
KeyBind BIND_CALIB_P3 = { '3', false, false, false };
KeyBind BIND_CALIB_L1 = { '1', true, false, false };
KeyBind BIND_CALIB_L2 = { '2', true, false, false };
KeyBind BIND_CALIB_L3 = { '3', true, false, false };
KeyBind BIND_CALIB_GAUGE_START = { '5', false, false, false };
KeyBind BIND_CALIB_GAUGE_END = { '6', false, false, false };
KeyBind BIND_CALIB_MALE_BAR = { '7', false, false, false };
KeyBind BIND_CALIB_FEMALE_ARROW = { '8', false, false, false };
KeyBind BIND_CALIB_HEART = { '9', false, false, false };
KeyBind BIND_CALIB_MALE_ARROW = { '0', false, false, false };

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
            
            if (j.contains("LANGUAGE")) LANGUAGE = j["LANGUAGE"];
            if (j.contains("num_players")) num_players = j["num_players"];
            if (j.contains("mode_immediate")) mode_immediate = j["mode_immediate"];
            if (j.contains("bot_enabled")) bot_enabled = j["bot_enabled"];
            if (j.contains("auto_j")) auto_j = j["auto_j"];
            if (j.contains("auto_l")) auto_l = j["auto_l"];
            if (j.contains("auto_likes")) auto_likes = j["auto_likes"];
            if (j.contains("VISION_POLL_RATE_MS")) VISION_POLL_RATE_MS = j["VISION_POLL_RATE_MS"];
            if (j.contains("CLICK_SPEED_HZ")) CLICK_SPEED_HZ = j["CLICK_SPEED_HZ"];
            if (j.contains("GAUGE_THRESHOLD")) GAUGE_THRESHOLD = j["GAUGE_THRESHOLD"];
            
            if (j.contains("players")) {
                for (size_t i = 0; i < j["players"].size() && i < 4; i++) {
                    PLAYER_POSITIONS[i].x = j["players"][i]["x"];
                    PLAYER_POSITIONS[i].y = j["players"][i]["y"];
                }
            }
            if (j.contains("likes")) {
                for (size_t i = 0; i < j["likes"].size() && i < 4; i++) {
                    LIKE_POSITIONS[i].x = j["likes"][i]["x"];
                    LIKE_POSITIONS[i].y = j["likes"][i]["y"];
                }
            }
            if (j.contains("gauge_first")) { GAUGE_POS_FIRST.x = j["gauge_first"]["x"]; GAUGE_POS_FIRST.y = j["gauge_first"]["y"]; }
            if (j.contains("gauge_last")) { GAUGE_POS_LAST.x = j["gauge_last"]["x"]; GAUGE_POS_LAST.y = j["gauge_last"]["y"]; }
            if (j.contains("male_bar")) { MALE_BAR_POS.x = j["male_bar"]["x"]; MALE_BAR_POS.y = j["male_bar"]["y"]; }
            if (j.contains("male_arrow")) { MALE_ARROW_POS.x = j["male_arrow"]["x"]; MALE_ARROW_POS.y = j["male_arrow"]["y"]; }
            if (j.contains("female_bar")) { FEMALE_BAR_POS.x = j["female_bar"]["x"]; FEMALE_BAR_POS.y = j["female_bar"]["y"]; }
            if (j.contains("heart_l")) { HEART_L_POS.x = j["heart_l"]["x"]; HEART_L_POS.y = j["heart_l"]["y"]; }
            
            LoadKeyBind(j, "bind_menu_toggle", BIND_MENU_TOGGLE);
            LoadKeyBind(j, "bind_app_visibility", BIND_APP_VISIBILITY);
            LoadKeyBind(j, "bind_spam_j", BIND_SPAM_J);
            LoadKeyBind(j, "bind_spam_l", BIND_SPAM_L);
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
            LoadKeyBind(j, "bind_calib_female_arrow", BIND_CALIB_FEMALE_ARROW);
            LoadKeyBind(j, "bind_calib_heart", BIND_CALIB_HEART);
            LoadKeyBind(j, "bind_calib_male_arrow", BIND_CALIB_MALE_ARROW);
        }
    } catch (...) {
    }
}

void SaveConfig()
{
    try {
        nlohmann::json j;
        j["bot_enabled"] = bot_enabled;
        j["auto_j"] = auto_j;
        j["auto_l"] = auto_l;
        j["auto_likes"] = auto_likes;
        j["num_players"] = num_players;
        j["mode_immediate"] = mode_immediate;
        j["VISION_POLL_RATE_MS"] = VISION_POLL_RATE_MS;
        j["CLICK_SPEED_HZ"] = CLICK_SPEED_HZ;
        j["GAUGE_THRESHOLD"] = GAUGE_THRESHOLD;
        j["LANGUAGE"] = LANGUAGE;

        nlohmann::json players = nlohmann::json::array();
        for (int i = 0; i < 4; i++) {
            players.push_back({{"x", PLAYER_POSITIONS[i].x}, {"y", PLAYER_POSITIONS[i].y}});
        }
        j["players"] = players;

        nlohmann::json likes = nlohmann::json::array();
        for (int i = 0; i < 4; i++) {
            likes.push_back({{"x", LIKE_POSITIONS[i].x}, {"y", LIKE_POSITIONS[i].y}});
        }
        j["likes"] = likes;

        j["gauge_first"] = {{"x", GAUGE_POS_FIRST.x}, {"y", GAUGE_POS_FIRST.y}};
        j["gauge_last"] = {{"x", GAUGE_POS_LAST.x}, {"y", GAUGE_POS_LAST.y}};
        j["male_bar"] = {{"x", MALE_BAR_POS.x}, {"y", MALE_BAR_POS.y}};
        j["male_arrow"] = {{"x", MALE_ARROW_POS.x}, {"y", MALE_ARROW_POS.y}};
        j["female_bar"] = {{"x", FEMALE_BAR_POS.x}, {"y", FEMALE_BAR_POS.y}};
        j["heart_l"] = {{"x", HEART_L_POS.x}, {"y", HEART_L_POS.y}};
        
        SaveKeyBind(j, "bind_menu_toggle", BIND_MENU_TOGGLE);
        SaveKeyBind(j, "bind_app_visibility", BIND_APP_VISIBILITY);
        SaveKeyBind(j, "bind_spam_j", BIND_SPAM_J);
        SaveKeyBind(j, "bind_spam_l", BIND_SPAM_L);
        SaveKeyBind(j, "bind_pin_window", BIND_PIN_WINDOW);
        SaveKeyBind(j, "bind_pin_all", BIND_PIN_ALL);
        SaveKeyBind(j, "bind_calib_p1", BIND_CALIB_P1);
        SaveKeyBind(j, "bind_calib_p2", BIND_CALIB_P2);
        SaveKeyBind(j, "bind_calib_p3", BIND_CALIB_P3);
        SaveKeyBind(j, "bind_calib_l1", BIND_CALIB_L1);
        SaveKeyBind(j, "bind_calib_l2", BIND_CALIB_L2);
        SaveKeyBind(j, "bind_calib_l3", BIND_CALIB_L3);
        SaveKeyBind(j, "bind_calib_gauge_start", BIND_CALIB_GAUGE_START);
        SaveKeyBind(j, "bind_calib_gauge_end", BIND_CALIB_GAUGE_END);
        SaveKeyBind(j, "bind_calib_male_bar", BIND_CALIB_MALE_BAR);
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
