#include "ui/gui_widgets.h"
#include "imgui_internal.h"
bool GameStyleButton(const char* label, bool active, const ImVec2& size_arg, bool hoverIsPink)
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

bool ActionConfirmButton(const char* label, const ImVec2& size_arg, ImU32 bgCol)
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

bool MenuSelectButton(const char* label, bool active, const ImVec2& size_arg)
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

bool MenuSelectButtonWithBind(const char* label, const char* bindStr, bool active, const ImVec2& size_arg)
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

void DrawGameStyleWindowBorder(ImU32 color)
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

bool CustomSelector(const char* label, int* current_item, const std::vector<std::string>& items, float width)
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

bool CustomSliderFloat(const char* label, float* v, float v_min, float v_max, const char* format)
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
        held;
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
            
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(30,40,50,255));
        ImGui::PushID(id + 1);
        
        if (ImGui::InputFloat("##v", v, 0.0f, 0.0f, format, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            is_editing;
            if (*v < v_min) *v = v_min;
            if (*v > v_max) *v = v_max;
            changed = true;
        }
        
        ImGui::PopID();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        
        if (ImGui::IsItemDeactivated())
        {
            is_editing;
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

bool CustomSliderInt(const char* label, int* v, int v_min, int v_max, const char* format)
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
        held;
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
            
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2());
        ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(30,40,50,255));
        ImGui::PushID(id + 1);
        
        if (ImGui::InputInt("##v", v, 0, 0, ImGuiInputTextFlags_EnterReturnsTrue))
        {
            is_editing;
            if (*v < v_min) *v = v_min;
            if (*v > v_max) *v = v_max;
            changed = true;
        }
        
        ImGui::PopID();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        
        if (ImGui::IsItemDeactivated())
        {
            is_editing;
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

