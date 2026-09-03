#pragma once
#include <vector>
#include <string>

// Struct for coordinates
struct Point2D {
    int x, y;
};

// Global Config State
extern bool bot_enabled;
extern bool auto_j;
extern bool auto_l;
extern bool auto_likes;
extern int num_players;
extern bool mode_immediate;

extern int VISION_POLL_RATE_MS;
extern float CLICK_SPEED_HZ;

extern int GAUGE_THRESHOLD;
extern std::string LANGUAGE;

extern Point2D PLAYER_POSITIONS[4];
extern Point2D LIKE_POSITIONS[4];

extern Point2D GAUGE_POS_FIRST;
extern Point2D GAUGE_POS_LAST;
extern Point2D MALE_BAR_POS;
extern Point2D MALE_ARROW_POS;
extern Point2D FEMALE_BAR_POS;
extern Point2D HEART_L_POS;

struct KeyBind {
    int vkCode;
    bool shift;
    bool ctrl;
    bool alt;

    bool operator==(const KeyBind& other) const {
        return vkCode == other.vkCode && shift == other.shift && ctrl == other.ctrl && alt == other.alt;
    }
};

// Application Binds
extern KeyBind BIND_MENU_TOGGLE;
extern KeyBind BIND_APP_VISIBILITY;
extern KeyBind BIND_SPAM_J;
extern KeyBind BIND_SPAM_L;
extern KeyBind BIND_PIN_WINDOW;
extern KeyBind BIND_PIN_ALL;

// Calibration Binds
extern KeyBind BIND_CALIB_P1;
extern KeyBind BIND_CALIB_P2;
extern KeyBind BIND_CALIB_P3;
extern KeyBind BIND_CALIB_L1;
extern KeyBind BIND_CALIB_L2;
extern KeyBind BIND_CALIB_L3;
extern KeyBind BIND_CALIB_GAUGE_START;
extern KeyBind BIND_CALIB_GAUGE_END;
extern KeyBind BIND_CALIB_MALE_BAR;
extern KeyBind BIND_CALIB_FEMALE_ARROW;
extern KeyBind BIND_CALIB_HEART;
extern KeyBind BIND_CALIB_MALE_ARROW;

std::string VKCodeToString(int vk);
std::string KeyBindToString(const KeyBind& bind);

// Diagnostics State
extern bool g_bSpammingJ;
extern bool g_bSpammingL;
extern std::string g_MaleBarColor;
extern std::string g_LikesStatus;

void AddLog(const std::string& message);

// Initialization / Save
void LoadConfig();
void SaveConfig();
