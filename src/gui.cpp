#include "gui.h"
#include "imgui.h"
#include "imgui_internal.h"
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


bool GameStyleButton(const char* label, bool active, const ImVec2& size_arg = ImVec2(0, 0), bool hoverIsPink = false)
{
    ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
    ImVec2 size = size_arg;
    if (size.x <= 0.0f) size.x = label_size.x + ImGui::GetStyle().FramePadding.x * 2.0f;
    if (size.y <= 0.0f) size.y = label_size.y + ImGui::GetStyle().FramePadding.y * 2.0f;
    
    ImVec2 pos = ImGui::GetCursorScreenPos();
    bool pressed = ImGui::InvisibleButton(label, size);
    bool hovered = ImGui::IsItemHovered();
    bool held = ImGui::IsItemActive();
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 bbMin = pos;
    ImVec2 bbMax = ImVec2(pos.x + size.x, pos.y + size.y);
    
    ImU32 textCol = IM_COL32(255, 255, 255, 255);
    ImU32 bgCol = IM_COL32(20, 20, 20, 150);
    ImU32 borderCol = IM_COL32(65, 184, 213, 50); // Cyan low alpha

    if (hoverIsPink)
    {
        bgCol = IM_COL32(255, 107, 129, 200); // Pink/Red
        borderCol = IM_COL32(255, 107, 129, 255);
    }
    
    if (held)
    {
        bgCol = hoverIsPink ? IM_COL32(200, 80, 100, 255) : IM_COL32(40, 120, 150, 255);
    }
    else if (hovered)
    {
        bgCol = hoverIsPink ? IM_COL32(255, 130, 150, 255) : IM_COL32(65, 184, 213, 80);
        borderCol = hoverIsPink ? IM_COL32(255, 150, 170, 255) : IM_COL32(65, 184, 213, 255);
    }
    else if (active && !hoverIsPink)
    {
        bgCol = IM_COL32(40, 40, 40, 200);
        textCol = IM_COL32(65, 184, 213, 255); // Cyan text if active
    }

    draw_list->AddRectFilled(bbMin, bbMax, bgCol);
    draw_list->AddRect(bbMin, bbMax, borderCol, 0.0f, 0, 1.0f);

    if (active && !hoverIsPink)
    {
        float radius = 3.0f;
        ImVec2 dotPos = ImVec2(bbMax.x - radius - 6.0f, bbMin.y + size.y * 0.5f);
        draw_list->AddCircleFilled(dotPos, radius, IM_COL32(100, 255, 100, 255));
    }

    ImVec2 textPos = ImVec2(bbMin.x + (size.x - label_size.x) * 0.5f, bbMin.y + (size.y - label_size.y) * 0.5f);
    draw_list->AddText(textPos, textCol, label);

    return pressed;
}

#include <map>

bool ActionConfirmButton(const char* label, const ImVec2& size_arg = ImVec2(0,0), ImU32 bgCol = IM_COL32(248, 101, 128, 255))
{
    ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
    ImVec2 size = size_arg;
    if (size.x <= 0.0f) size.x = label_size.x + ImGui::GetStyle().FramePadding.x * 2.0f;
    if (size.y <= 0.0f) size.y = label_size.y + ImGui::GetStyle().FramePadding.y * 2.0f;
    
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGuiID id = ImGui::GetID(label);
    bool pressed = ImGui::InvisibleButton(label, size);
    bool hovered = ImGui::IsItemHovered();
    bool held = ImGui::IsItemActive();
    
    static std::map<ImGuiID, float> anim_map;
    float& t = anim_map[id];
    
    if (hovered || held) {
        t += ImGui::GetIO().DeltaTime * 5.0f;
        if (t > 1.0f) t = 1.0f;
    } else {
        t -= ImGui::GetIO().DeltaTime * 5.0f;
        if (t < 0.0f) t = 0.0f;
    }

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 bbMin = pos;
    ImVec2 bbMax = ImVec2(pos.x + size.x, pos.y + size.y);
    
    ImU32 textCol = IM_COL32(51, 51, 51, 255); // #333333 text

    draw_list->AddRectFilled(bbMin, bbMax, bgCol);
    
    // Fill animation with gray
    if (t > 0.0f)
    {
        ImVec2 fillMax = ImVec2(bbMin.x + size.x * t, bbMax.y);
        draw_list->AddRectFilled(bbMin, fillMax, IM_COL32(200, 200, 200, 255)); // Light gray
    }
    
    // Draw thin white border (0.2px)
    draw_list->AddRect(bbMin, bbMax, IM_COL32(255, 255, 255, 120), 0.0f, 0, 0.2f);
    
    ImVec2 textPos = ImVec2(bbMin.x + (size.x - label_size.x) * 0.5f, bbMin.y + (size.y - label_size.y) * 0.5f);
    draw_list->AddText(textPos, textCol, label);

    return pressed;
}

bool MenuSelectButton(const char* label, bool active, const ImVec2& size_arg = ImVec2(0, 0))
{
    ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
    ImVec2 size = size_arg;
    if (size.x <= 0.0f) size.x = label_size.x + ImGui::GetStyle().FramePadding.x * 2.0f;
    if (size.y <= 0.0f) size.y = label_size.y + ImGui::GetStyle().FramePadding.y * 2.0f;
    
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGuiID id = ImGui::GetID(label);
    bool pressed = ImGui::InvisibleButton(label, size);
    bool hovered = ImGui::IsItemHovered();
    
    static std::map<ImGuiID, float> anim_map;
    float& t = anim_map[id];
    
    if (hovered) {
        t += ImGui::GetIO().DeltaTime * 5.0f;
        if (t > 1.0f) t = 1.0f;
    } else {
        t -= ImGui::GetIO().DeltaTime * 5.0f;
        if (t < 0.0f) t = 0.0f;
    }

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 bbMin = pos;
    ImVec2 bbMax = ImVec2(pos.x + size.x, pos.y + size.y);
    
    ImU32 textCol = IM_COL32(255, 255, 255, 255);
    ImU32 bgCol = IM_COL32(26, 37, 44, 255); // Dark background #1a252c
    ImU32 borderCol = IM_COL32(65, 184, 213, 100); // Faint cyan border

    if (active)
    {
        bgCol = IM_COL32(255, 255, 255, 255); // White background
        textCol = IM_COL32(40, 40, 40, 255); // Dark grey text
        borderCol = IM_COL32(255, 255, 255, 255); // White border
    }

    // Draw base background
    draw_list->AddRectFilled(bbMin, bbMax, bgCol);
    
    // Draw fill animation if not active
    if (!active && t > 0.0f)
    {
        ImVec2 fillMax = ImVec2(bbMin.x + size.x * t, bbMax.y);
        draw_list->AddRectFilled(bbMin, fillMax, IM_COL32(255, 255, 255, 255));
        
        // If animation is filling, text should smoothly transition to dark where filled. 
        // For simplicity in immediate mode, we just switch text color if t > 0.5
        if (t > 0.5f) textCol = IM_COL32(51, 51, 51, 255);
    }

    // Draw faint cyan border
    draw_list->AddRect(bbMin, bbMax, borderCol, 0.0f, 0, 1.0f);
    
    // Draw thin white border (0.2px)
    draw_list->AddRect(bbMin, bbMax, IM_COL32(255, 255, 255, 120), 0.0f, 0, 0.2f);

    // Circle Indicator
    float radius = 3.0f;
    ImVec2 dotPos = ImVec2(bbMax.x - radius - 15.0f, bbMin.y + size.y * 0.5f);
    if (active)
        draw_list->AddCircleFilled(dotPos, radius, IM_COL32(100, 255, 100, 255)); // Green
    else
        draw_list->AddCircleFilled(dotPos, radius, IM_COL32(255, 100, 100, 255)); // Red

    // Draw text (left aligned)
    ImVec2 textPos = ImVec2(bbMin.x + 15.0f, bbMin.y + (size.y - label_size.y) * 0.5f);
    draw_list->AddText(textPos, textCol, label);

    return pressed;
}

bool MenuSelectButtonWithBind(const char* label, const char* bindStr, bool active, const ImVec2& size_arg = ImVec2(0, 0))
{
    ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
    ImVec2 size = size_arg;
    if (size.x <= 0.0f) size.x = label_size.x + ImGui::GetStyle().FramePadding.x * 2.0f;
    if (size.y <= 0.0f) size.y = label_size.y + ImGui::GetStyle().FramePadding.y * 2.0f;
    
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGuiID id = ImGui::GetID(label);
    bool pressed = ImGui::InvisibleButton(label, size);
    bool hovered = ImGui::IsItemHovered();
    
    static std::map<ImGuiID, float> anim_map;
    float& t = anim_map[id];
    
    if (hovered) {
        t += ImGui::GetIO().DeltaTime * 5.0f;
        if (t > 1.0f) t = 1.0f;
    } else {
        t -= ImGui::GetIO().DeltaTime * 5.0f;
        if (t < 0.0f) t = 0.0f;
    }

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 bbMin = pos;
    ImVec2 bbMax = ImVec2(pos.x + size.x, pos.y + size.y);
    
    ImU32 textCol = IM_COL32(255, 255, 255, 255);
    ImU32 bgCol = IM_COL32(26, 37, 44, 255); // Dark background #1a252c
    ImU32 borderCol = IM_COL32(65, 184, 213, 100); // Faint cyan border

    if (active)
    {
        bgCol = IM_COL32(255, 255, 255, 255); // White background
        textCol = IM_COL32(40, 40, 40, 255); // Dark grey text
        borderCol = IM_COL32(255, 255, 255, 255); // White border
    }

    // Draw base background
    draw_list->AddRectFilled(bbMin, bbMax, bgCol);
    
    // Draw fill animation if not active
    if (!active && t > 0.0f)
    {
        ImVec2 fillMax = ImVec2(bbMin.x + size.x * t, bbMax.y);
        draw_list->AddRectFilled(bbMin, fillMax, IM_COL32(255, 255, 255, 255));
        
        if (t > 0.5f) textCol = IM_COL32(51, 51, 51, 255);
    }

    // Draw faint cyan border
    draw_list->AddRect(bbMin, bbMax, borderCol, 0.0f, 0, 1.0f);
    
    // Draw thin white border (0.2px)
    draw_list->AddRect(bbMin, bbMax, IM_COL32(255, 255, 255, 120), 0.0f, 0, 0.2f);

    // Circle Indicator
    float radius = 3.0f;
    ImVec2 dotPos = ImVec2(bbMax.x - radius - 15.0f, bbMin.y + size.y * 0.5f);
    if (active)
        draw_list->AddCircleFilled(dotPos, radius, IM_COL32(100, 255, 100, 255)); // Green
    else
        draw_list->AddCircleFilled(dotPos, radius, IM_COL32(255, 100, 100, 255)); // Red

    // Draw Text (left aligned, fixed position)
    ImVec2 textPos = ImVec2(bbMin.x + 15.0f, bbMin.y + (size.y - label_size.y) * 0.5f);
    draw_list->AddText(textPos, textCol, label);

    // Draw Bind Text (to the right of the main text, in grey)
    if (bindStr && bindStr[0] != '\0')
    {
        ImVec2 bindPos = ImVec2(textPos.x + 80.0f, textPos.y); // Fixed offset for alignment
        ImU32 bindCol = IM_COL32(120, 120, 120, 255); // Grey
        draw_list->AddText(bindPos, bindCol, bindStr);
    }

    return pressed;
}

#include <math.h>

void DrawGameStyleWindowBorder(ImU32 color = IM_COL32(106, 113, 113, 255))
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 p_min = ImGui::GetWindowPos();
    ImVec2 p_max = ImVec2(p_min.x + ImGui::GetWindowSize().x, p_min.y + ImGui::GetWindowSize().y);
    
    draw_list->PushClipRect(p_min, p_max, false);

    // Draw dotted background pattern below title bar
    float startY = p_min.y + 40.0f;
    float h = p_max.y - startY;
    if (h > 0)
    {
        for (float y = startY + 6.0f; y < p_max.y; y += 6.0f) {
            float t = (y - startY) / h; // purely linear fade
            
            int alpha = (int)(t * 255.0f); // linear alpha scaling
            if (alpha <= 2) continue;
            
            for (float x = p_min.x + 6.0f; x < p_max.x; x += 6.0f) {
                ImVec2 pos(floorf(x), floorf(y));
                draw_list->AddRectFilled(pos, ImVec2(pos.x + 1.0f, pos.y + 1.0f), IM_COL32(70, 74, 74, alpha));
            }
        }
    }
    
    // Draw 1.0 pixel border
    draw_list->AddRect(p_min, ImVec2(p_max.x - 1.0f, p_max.y - 1.0f), color, 0.0f, 0, 1.0f);
    
    draw_list->PopClipRect();
}

bool DrawCalibrationTitleBar(const char* title)
{
    ImVec2 p_min = ImGui::GetWindowPos(); // Top-left of window
    ImVec2 p_max = ImVec2(p_min.x + ImGui::GetWindowSize().x, p_min.y + 40.0f);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Draw red background
    draw_list->AddRectFilled(p_min, p_max, IM_COL32(248, 101, 128, 255));
    
    // Draw diagonal lines (animated)
    draw_list->PushClipRect(p_min, p_max, true);
    ImU32 lineCol = IM_COL32(255, 255, 255, 100);
    float offset = fmodf((float)ImGui::GetTime() * 30.0f, 12.0f); 
    for (float x = p_min.x - 80.0f + offset; x < p_max.x; x += 12.0f)
    {
        draw_list->AddLine(ImVec2(x, p_max.y), ImVec2(x + 80.0f, p_min.y), lineCol, 1.0f); 
    }
    draw_list->PopClipRect();
    
    // Draw Text
    ImU32 textCol = IM_COL32(255, 255, 255, 255); // White text
    ImFont* font = ImGui::GetFont();
    float fontSize = ImGui::GetFontSize() * 1.5f; 
    draw_list->AddText(font, fontSize, ImVec2(p_min.x + 15.0f, p_min.y + (40.0f - fontSize)*0.5f), textCol, title);
    
    bool closed = false;
    // Check click manually to avoid AutoResize feedback loop when padding is > 0
    ImVec2 close_min = ImVec2(p_max.x - 40.0f, p_min.y);
    ImVec2 close_max = ImVec2(p_max.x, p_min.y + 40.0f);
    if (ImGui::IsMouseHoveringRect(close_min, close_max) && ImGui::IsMouseClicked(0)) {
        closed = true;
    }
    
    ImU32 xCol = IM_COL32(255, 255, 255, 255);
    ImVec2 cCenter = ImVec2(p_max.x - 20.0f, p_min.y + 20.0f);
    draw_list->AddLine(ImVec2(cCenter.x - 8.0f, cCenter.y - 8.0f), ImVec2(cCenter.x + 8.0f, cCenter.y + 8.0f), xCol, 1.5f);
    draw_list->AddLine(ImVec2(cCenter.x + 8.0f, cCenter.y - 8.0f), ImVec2(cCenter.x - 8.0f, cCenter.y + 8.0f), xCol, 1.5f);
    
    // Restore Cursor below title bar
    ImGui::SetCursorScreenPos(p_min);
    ImGui::Dummy(ImVec2(ImGui::GetWindowSize().x, 40.0f)); 
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 10.0f);
    
    return closed;
}

bool DrawCustomTitleBar(const char* title, bool& isPinned)
{
    ImVec2 p_min = ImGui::GetCursorScreenPos();
    ImVec2 p_max = ImVec2(p_min.x + ImGui::GetWindowSize().x, p_min.y + 40.0f);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    draw_list->PushClipRect(p_min, p_max, false);
    
    // Draw white background
    draw_list->AddRectFilled(p_min, p_max, IM_COL32(240, 240, 240, 255));
    
    // Draw diagonal lines (animated)
    draw_list->PushClipRect(p_min, p_max, true); // Prevent lines from bleeding below the title bar
    ImU32 lineCol = IM_COL32(200, 200, 200, 200); // slightly softer alpha
    float offset = fmodf((float)ImGui::GetTime() * 30.0f, 12.0f); 
    for (float x = p_min.x - 80.0f + offset; x < p_max.x; x += 12.0f) // slightly less dense
    {
        draw_list->AddLine(ImVec2(x, p_max.y), ImVec2(x + 80.0f, p_min.y), lineCol, 1.0f); 
    }
    draw_list->PopClipRect();
    
    // Pin indicator circle (left side, dark theme)
    float circleRadius = 3.0f;
    ImVec2 circlePos = ImVec2(p_min.x + 12.0f, p_min.y + 20.0f);
    
    // Make circle interactive
    ImVec2 hit_min = ImVec2(circlePos.x - 12.0f, circlePos.y - 12.0f);
    ImGui::SetCursorScreenPos(hit_min);
    if (ImGui::InvisibleButton("##pincircle", ImVec2(24.0f, 24.0f))) {
        isPinned = !isPinned;
    }
    
    if (isPinned)
        draw_list->AddCircleFilled(circlePos, circleRadius, IM_COL32(65, 184, 213, 255)); // Cyan
    else
        draw_list->AddCircleFilled(circlePos, circleRadius, IM_COL32(150, 150, 150, 255)); // Grey
    
    // Draw Text
    ImU32 textCol = IM_COL32(51, 51, 51, 255);
    draw_list->AddText(ImVec2(p_min.x + 25.0f, p_min.y + 10.0f), textCol, title);
    
    bool closed = false;
    // Check click manually to avoid AutoResize feedback loop
    ImVec2 close_min = ImVec2(p_max.x - 40.0f, p_min.y);
    ImVec2 close_max = ImVec2(p_max.x, p_min.y + 40.0f);
    if (ImGui::IsMouseHoveringRect(close_min, close_max) && ImGui::IsMouseClicked(0)) {
        closed = true;
    }
    
    ImVec2 cCenter = ImVec2(p_max.x - 20.0f, p_min.y + 20.0f);
    draw_list->AddLine(ImVec2(cCenter.x - 8.0f, cCenter.y - 8.0f), ImVec2(cCenter.x + 8.0f, cCenter.y + 8.0f), textCol, 1.5f);
    draw_list->AddLine(ImVec2(cCenter.x + 8.0f, cCenter.y - 8.0f), ImVec2(cCenter.x - 8.0f, cCenter.y + 8.0f), textCol, 1.5f);
    
    draw_list->PopClipRect();

    // Dummy item to reserve vertical space and break AutoResize feedback loop (using fixed width)
    ImGui::SetCursorScreenPos(p_min);
    ImGui::Dummy(ImVec2(260.0f, 40.0f)); 
    
    return closed;
}

// -------------------------------------------------------------------------------------------------
// CUSTOM WIDGETS FOR AUXILIARY WINDOWS (Settings, Control Panel, etc.)
// -------------------------------------------------------------------------------------------------

bool DrawSubWindowTitleBar(const char* title, bool& isPinned)
{
    ImVec2 win_pos = ImGui::GetWindowPos();
    float win_width = ImGui::GetWindowSize().x;
    ImVec2 p_min = win_pos;
    ImVec2 p_max = ImVec2(win_pos.x + win_width, win_pos.y + 40.0f);
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    draw_list->PushClipRect(p_min, p_max, false);

    // Draw solid dark background for title bar with 80% transparency
    draw_list->AddRectFilled(p_min, p_max, IM_COL32(18, 25, 30, 204));
    
    // Pin indicator circle (left side)
    float circleRadius = 3.0f;
    ImVec2 circlePos = ImVec2(win_pos.x + 15.0f, win_pos.y + 20.0f);
    
    // Make circle interactive
    ImVec2 hit_min = ImVec2(circlePos.x - 12.0f, circlePos.y - 12.0f);
    ImGui::SetCursorScreenPos(hit_min);
    if (ImGui::InvisibleButton("##pincircle", ImVec2(24.0f, 24.0f))) {
        isPinned = !isPinned;
    }
    
    if (isPinned)
        draw_list->AddCircleFilled(circlePos, circleRadius, IM_COL32(110, 196, 232, 255)); // Cyan/blue
    else
        draw_list->AddCircleFilled(circlePos, circleRadius, IM_COL32(150, 150, 150, 255)); // Grey
    
    // Title Text (Cyan) - shifted right to make room for circle
    ImU32 textCol = IM_COL32(110, 196, 232, 255); // Cyan
    draw_list->AddText(ImVec2(win_pos.x + win_width*0.5f - ImGui::CalcTextSize(title).x * 0.5f, win_pos.y + 12.0f), textCol, title);
    
    bool closed = false;
    // Check click manually to avoid AutoResize feedback loop when padding is > 0
    ImVec2 close_min = ImVec2(win_pos.x + win_width - 40.0f, win_pos.y);
    ImVec2 close_max = ImVec2(win_pos.x + win_width, win_pos.y + 40.0f);
    if (ImGui::IsMouseHoveringRect(close_min, close_max) && ImGui::IsMouseClicked(0)) {
        closed = true;
    }
    
    // Draw X
    ImU32 xCol = IM_COL32(200, 200, 200, 255);
    ImVec2 cCenter = ImVec2(win_pos.x + win_width - 20.0f, win_pos.y + 20.0f);
    draw_list->AddLine(ImVec2(cCenter.x - 8.0f, cCenter.y - 8.0f), ImVec2(cCenter.x + 8.0f, cCenter.y + 8.0f), xCol, 1.5f);
    draw_list->AddLine(ImVec2(cCenter.x + 8.0f, cCenter.y - 8.0f), ImVec2(cCenter.x - 8.0f, cCenter.y + 8.0f), xCol, 1.5f);
    
    draw_list->PopClipRect();

    // Resume layout properly below the title bar, respecting window padding
    ImGui::SetCursorPosY(40.0f - ImGui::GetStyle().WindowPadding.y); 
    
    return closed;
}

bool CyanActionButton(const char* label, const ImVec2& size_arg)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    
    ImVec2 pos = window->DC.CursorPos;
    ImVec2 size = ImGui::CalcItemSize(size_arg, ImGui::CalcTextSize(label).x + style.FramePadding.x * 2.0f, style.FramePadding.y * 2.0f);
    
    const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
    ImGui::ItemSize(size, style.FramePadding.y);
    if (!ImGui::ItemAdd(bb, id)) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held);

    static std::map<ImGuiID, float> anim_map;
    float& t = anim_map[id];
    
    if (hovered || held) {
        t += ImGui::GetIO().DeltaTime * 5.0f;
        if (t > 1.0f) t = 1.0f;
    } else {
        t -= ImGui::GetIO().DeltaTime * 5.0f;
        if (t < 0.0f) t = 0.0f;
    }

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImU32 bgCol = IM_COL32(110, 196, 232, 255); // Cyan background
    ImU32 textCol = IM_COL32(10, 10, 10, 255); // Dark text
    
    draw_list->AddRectFilled(bb.Min, bb.Max, bgCol);
    
    if (t > 0.0f)
    {
        ImVec2 fillMax = ImVec2(bb.Min.x + size.x * t, bb.Max.y);
        draw_list->AddRectFilled(bb.Min, fillMax, IM_COL32(255, 255, 255, 255)); // White fill
    }
    
    // Draw thin white border (0.2px)
    draw_list->AddRect(bb.Min, bb.Max, IM_COL32(255, 255, 255, 120), 0.0f, 0, 0.2f);
    
    ImVec2 label_size = ImGui::CalcTextSize(label);
    ImVec2 textPos = ImVec2(bb.Min.x + (size.x - label_size.x) * 0.5f, bb.Min.y + (size.y - label_size.y) * 0.5f);
    draw_list->AddText(textPos, textCol, label);

    return pressed;
}

bool CustomSelector(const char* label, int* current_item, const std::vector<std::string>& items, float width = 200.0f)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImVec2 pos = window->DC.CursorPos;
    float rowHeight = 30.0f;
    float totalWidth = 330.0f; // Fixed content width
    
    // Draw Label on the left
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", label);
    ImGui::SameLine();
    
    // Position the selector on the right
    ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMin().x + totalWidth - width);
    
    pos = window->DC.CursorPos;
    ImRect bb(pos, ImVec2(pos.x + width, pos.y + rowHeight));
    
    ImGui::ItemSize(bb.GetSize());
    if (!ImGui::ItemAdd(bb, window->GetID(label))) return false;
    
    bool changed = false;
    
    // Left Arrow
    ImGui::SetCursorScreenPos(pos);
    std::string leftIDStr = std::string("##L_") + label;
    bool lClicked = ImGui::InvisibleButton(leftIDStr.c_str(), ImVec2(30.0f, rowHeight));
    bool lHov = ImGui::IsItemHovered();
    if (lClicked && *current_item > 0)
    {
        (*current_item)--;
        changed = true;
    }
    
    // Right Arrow
    ImGui::SetCursorScreenPos(ImVec2(pos.x + width - 30.0f, pos.y));
    std::string rightIDStr = std::string("##R_") + label;
    bool rClicked = ImGui::InvisibleButton(rightIDStr.c_str(), ImVec2(30.0f, rowHeight));
    bool rHov = ImGui::IsItemHovered();
    if (rClicked && *current_item < (int)items.size() - 1)
    {
        (*current_item)++;
        changed = true;
    }
    
    // Restore Cursor
    ImGui::SetCursorScreenPos(ImVec2(pos.x, pos.y + rowHeight));
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImU32 arrColL = (*current_item > 0) ? IM_COL32(110, 196, 232, lHov ? 255 : 200) : IM_COL32(80, 80, 80, 255);
    ImU32 arrColR = (*current_item < (int)items.size() - 1) ? IM_COL32(110, 196, 232, rHov ? 255 : 200) : IM_COL32(80, 80, 80, 255);
    
    // Draw Left Arrow
    float h = rowHeight * 0.5f;
    ImVec2 lC(pos.x + 15.0f, pos.y + h);
    draw_list->AddTriangleFilled(ImVec2(lC.x - 6, lC.y), ImVec2(lC.x + 6, lC.y - 6), ImVec2(lC.x + 6, lC.y + 6), arrColL);
    
    // Draw Right Arrow
    ImVec2 rC(pos.x + width - 15.0f, pos.y + h);
    draw_list->AddTriangleFilled(ImVec2(rC.x + 6, rC.y), ImVec2(rC.x - 6, rC.y - 6), ImVec2(rC.x - 6, rC.y + 6), arrColR);
    
    // Draw Text
    if (*current_item >= 0 && *current_item < (int)items.size())
    {
        const std::string& text = items[*current_item];
        ImVec2 tSize = ImGui::CalcTextSize(text.c_str());
        draw_list->AddText(ImVec2(pos.x + width*0.5f - tSize.x*0.5f, pos.y + h - tSize.y*0.5f), IM_COL32(255,255,255,255), text.c_str());
    }
    
    return changed;
}

bool CustomCheckbox(const char* label, bool* v)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    float totalWidth = 330.0f;
    ImVec2 pos = window->DC.CursorPos;
    
    ImGui::AlignTextToFramePadding();
    ImGui::Text("%s", label);
    ImGui::SameLine();
    
    float boxSize = 20.0f;
    ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMin().x + totalWidth - boxSize);
    pos = window->DC.CursorPos;
    
    ImRect bb(pos, ImVec2(pos.x + boxSize, pos.y + boxSize));
    ImGui::ItemSize(bb.GetSize());
    if (!ImGui::ItemAdd(bb, window->GetID(label))) return false;

    bool hovered, held;
    bool pressed = ImGui::ButtonBehavior(bb, window->GetID(label), &hovered, &held);
    if (pressed) *v = !(*v);

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->AddRect(bb.Min, bb.Max, IM_COL32(110, 196, 232, 100), 0.0f, 0, 1.0f);
    
    if (*v)
    {
        // Draw Checkmark
        float pad = 4.0f;
        draw_list->AddLine(ImVec2(bb.Min.x + pad, bb.Min.y + boxSize*0.5f), ImVec2(bb.Min.x + boxSize*0.4f, bb.Max.y - pad), IM_COL32(255,255,255,255), 2.0f);
        draw_list->AddLine(ImVec2(bb.Min.x + boxSize*0.4f, bb.Max.y - pad), ImVec2(bb.Max.x - pad, bb.Min.y + pad), IM_COL32(255,255,255,255), 2.0f);
    }
    
    return pressed;
}

bool CustomSliderFloat(const char* label, float* v, float v_min, float v_max, const char* format = "%.1f")
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImVec2 pos = window->DC.CursorPos;
    float rowHeight = 40.0f;
    float width = 330.0f; // Fixed content width
    
    ImRect bb(pos, ImVec2(pos.x + width, pos.y + rowHeight));
    ImGuiID id = window->GetID(label);
    
    bool& is_editing = *ImGui::GetStateStorage()->GetBoolRef(id + 1);
    
    ImRect interactBB = bb;
    if (is_editing)
        interactBB.Max.x = pos.x + width - 65.0f;
        
    ImGui::ItemSize(bb.GetSize());
    if (!ImGui::ItemAdd(bb, id)) return false;
    
    bool hovered, held;
    ImGui::ButtonBehavior(interactBB, id, &hovered, &held);
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    // Draw Label and Value
    char valBuf[64];
    snprintf(valBuf, sizeof(valBuf), format, *v);
    draw_list->AddText(pos, IM_COL32(255,255,255,255), label);
    ImVec2 vSize = ImGui::CalcTextSize(valBuf);
    ImVec2 valPos(pos.x + width - vSize.x, pos.y);
    
    ImRect valBB(valPos, ImVec2(valPos.x + vSize.x, valPos.y + vSize.y));
    valBB.Expand(ImVec2(5.0f, 5.0f));

    if (hovered && ImGui::IsMouseClicked(0) && valBB.Contains(ImGui::GetMousePos()))
    {
        is_editing = true;
        held = false;
        ImGui::ClearActiveID();
    }
    
    bool changed = false;
    if (!is_editing)
    {
        draw_list->AddText(valPos, IM_COL32(255,255,255,255), valBuf);
    }
    else
    {
        ImGui::SetCursorScreenPos(ImVec2(pos.x + width - 60.0f, pos.y - 2.0f));
        ImGui::SetNextItemWidth(60.0f);
        
        if (!ImGui::IsAnyItemActive() && ImGui::IsWindowFocused())
            ImGui::SetKeyboardFocusHere(0);
            
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(30,40,50,255));
        ImGui::PushID(id + 1);
        
        if (ImGui::InputFloat("##v", v, 0.0f, 0.0f, format, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            is_editing = false;
            if (*v < v_min) *v = v_min;
            if (*v > v_max) *v = v_max;
            changed = true;
        }
        
        ImGui::PopID();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        
        if (ImGui::IsItemDeactivated())
        {
            is_editing = false;
            if (*v < v_min) *v = v_min;
            if (*v > v_max) *v = v_max;
            changed = true;
        }
    }
    
    // Track and Thumb logic
    float trackY = pos.y + 25.0f;
    float trackH = 4.0f;
    ImRect trackBB(ImVec2(pos.x, trackY), ImVec2(pos.x + width, trackY + trackH));
    
    ImRect grab_bb;
    if (held && !is_editing) {
        if (ImGui::SliderBehavior(trackBB, id, ImGuiDataType_Float, v, &v_min, &v_max, format, ImGuiSliderFlags_None, &grab_bb))
            changed = true;
    }
    
    // Draw Track Background
    draw_list->AddRectFilled(trackBB.Min, trackBB.Max, IM_COL32(30, 40, 50, 255));
    
    // Draw Track Fill
    float fillT = (*v - v_min) / (v_max - v_min);
    ImRect fillBB(trackBB.Min, ImVec2(trackBB.Min.x + width * fillT, trackBB.Max.y));
    draw_list->AddRectFilled(fillBB.Min, fillBB.Max, IM_COL32(110, 196, 232, 255)); // Cyan fill
    
    // Draw Thumb
    float thumbW = 16.0f;
    float thumbH = 12.0f;
    ImVec2 thumbC(trackBB.Min.x + width * fillT, trackBB.Min.y + trackH*0.5f);
    ImRect thumbBB(ImVec2(thumbC.x - thumbW*0.5f, thumbC.y - thumbH*0.5f), ImVec2(thumbC.x + thumbW*0.5f, thumbC.y + thumbH*0.5f));
    draw_list->AddRectFilled(thumbBB.Min, thumbBB.Max, IM_COL32(140, 215, 245, 255), 3.0f); // Bright cyan thumb
    
    // Draw Ticks on thumb
    float tOff = 3.0f;
    draw_list->AddLine(ImVec2(thumbC.x, thumbBB.Min.y + 2), ImVec2(thumbC.x, thumbBB.Max.y - 2), IM_COL32(80, 150, 180, 255), 1.0f);
    draw_list->AddLine(ImVec2(thumbC.x - tOff, thumbBB.Min.y + 2), ImVec2(thumbC.x - tOff, thumbBB.Max.y - 2), IM_COL32(80, 150, 180, 255), 1.0f);
    draw_list->AddLine(ImVec2(thumbC.x + tOff, thumbBB.Min.y + 2), ImVec2(thumbC.x + tOff, thumbBB.Max.y - 2), IM_COL32(80, 150, 180, 255), 1.0f);
    
    return changed;
}

bool CustomSliderInt(const char* label, int* v, int v_min, int v_max, const char* format = "%d")
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImVec2 pos = window->DC.CursorPos;
    float rowHeight = 40.0f;
    float width = 330.0f;
    
    ImRect bb(pos, ImVec2(pos.x + width, pos.y + rowHeight));
    ImGuiID id = window->GetID(label);
    
    bool& is_editing = *ImGui::GetStateStorage()->GetBoolRef(id + 1);
    
    ImRect interactBB = bb;
    if (is_editing)
        interactBB.Max.x = pos.x + width - 65.0f;
        
    ImGui::ItemSize(bb.GetSize());
    if (!ImGui::ItemAdd(bb, id)) return false;
    
    bool hovered, held;
    ImGui::ButtonBehavior(interactBB, id, &hovered, &held);
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    
    char valBuf[64];
    snprintf(valBuf, sizeof(valBuf), format, *v); // Correctly passing int
    draw_list->AddText(pos, IM_COL32(255,255,255,255), label);
    ImVec2 vSize = ImGui::CalcTextSize(valBuf);
    ImVec2 valPos(pos.x + width - vSize.x, pos.y);
    
    ImRect valBB(valPos, ImVec2(valPos.x + vSize.x, valPos.y + vSize.y));
    valBB.Expand(ImVec2(5.0f, 5.0f));

    if (hovered && ImGui::IsMouseClicked(0) && valBB.Contains(ImGui::GetMousePos()))
    {
        is_editing = true;
        held = false;
        ImGui::ClearActiveID();
    }
    
    bool changed = false;
    if (!is_editing)
    {
        draw_list->AddText(valPos, IM_COL32(255,255,255,255), valBuf);
    }
    else
    {
        ImGui::SetCursorScreenPos(ImVec2(pos.x + width - 60.0f, pos.y - 2.0f));
        ImGui::SetNextItemWidth(60.0f);
        
        if (!ImGui::IsAnyItemActive() && ImGui::IsWindowFocused())
            ImGui::SetKeyboardFocusHere(0);
            
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(30,40,50,255));
        ImGui::PushID(id + 1);
        
        if (ImGui::InputInt("##v", v, 0, 0, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            is_editing = false;
            if (*v < v_min) *v = v_min;
            if (*v > v_max) *v = v_max;
            changed = true;
        }
        
        ImGui::PopID();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        
        if (ImGui::IsItemDeactivated())
        {
            is_editing = false;
            if (*v < v_min) *v = v_min;
            if (*v > v_max) *v = v_max;
            changed = true;
        }
    }
    
    float trackY = pos.y + 25.0f;
    float trackH = 4.0f;
    ImRect trackBB(ImVec2(pos.x, trackY), ImVec2(pos.x + width, trackY + trackH));
    
    ImRect grab_bb;
    if (held && !is_editing) {
        if (ImGui::SliderBehavior(trackBB, id, ImGuiDataType_S32, v, &v_min, &v_max, format, ImGuiSliderFlags_None, &grab_bb))
            changed = true;
    }
    
    draw_list->AddRectFilled(trackBB.Min, trackBB.Max, IM_COL32(30, 40, 50, 255));
    
    float fillT = (float)(*v - v_min) / (float)(v_max - v_min);
    ImRect fillBB(trackBB.Min, ImVec2(trackBB.Min.x + width * fillT, trackBB.Max.y));
    draw_list->AddRectFilled(fillBB.Min, fillBB.Max, IM_COL32(110, 196, 232, 255)); 
    
    float thumbW = 16.0f;
    float thumbH = 12.0f;
    ImVec2 thumbC(trackBB.Min.x + width * fillT, trackBB.Min.y + trackH*0.5f);
    ImRect thumbBB(ImVec2(thumbC.x - thumbW*0.5f, thumbC.y - thumbH*0.5f), ImVec2(thumbC.x + thumbW*0.5f, thumbC.y + thumbH*0.5f));
    draw_list->AddRectFilled(thumbBB.Min, thumbBB.Max, IM_COL32(140, 215, 245, 255), 3.0f);
    
    float tOff = 3.0f;
    draw_list->AddLine(ImVec2(thumbC.x, thumbBB.Min.y + 2), ImVec2(thumbC.x, thumbBB.Max.y - 2), IM_COL32(80, 150, 180, 255), 1.0f);
    draw_list->AddLine(ImVec2(thumbC.x - tOff, thumbBB.Min.y + 2), ImVec2(thumbC.x - tOff, thumbBB.Max.y - 2), IM_COL32(80, 150, 180, 255), 1.0f);
    draw_list->AddLine(ImVec2(thumbC.x + tOff, thumbBB.Min.y + 2), ImVec2(thumbC.x + tOff, thumbBB.Max.y - 2), IM_COL32(80, 150, 180, 255), 1.0f);
    
    return changed;
}

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
