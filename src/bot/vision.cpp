#include "bot/vision.h"
#include "core/config.h"
#include <d3d11.h>
#include <dxgi1_2.h>
#include <iostream>

// Globals for DXGI Duplication
ID3D11Device* v_pd3dDevice = nullptr;
ID3D11DeviceContext* v_pd3dDeviceContext = nullptr;
IDXGIOutputDuplication* v_pDuplication = nullptr;
ID3D11Texture2D* v_pStagingTexture = nullptr;
int v_ScreenWidth = 1920;
int v_ScreenHeight = 1080;

bool InitVision()
{
    // Basic setup for D3D11 device
    D3D_FEATURE_LEVEL featureLevel;
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &v_pd3dDevice, &featureLevel, &v_pd3dDeviceContext);
    if (FAILED(hr)) return false;

    IDXGIDevice* dxgiDevice = nullptr;
    hr = v_pd3dDevice->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
    if (FAILED(hr)) return false;

    IDXGIAdapter* dxgiAdapter = nullptr;
    hr = dxgiDevice->GetAdapter(&dxgiAdapter);
    dxgiDevice->Release();
    if (FAILED(hr)) return false;

    IDXGIOutput* dxgiOutput = nullptr;
    hr = dxgiAdapter->EnumOutputs(0, &dxgiOutput);
    dxgiAdapter->Release();
    if (FAILED(hr)) return false;

    IDXGIOutput1* dxgiOutput1 = nullptr;
    hr = dxgiOutput->QueryInterface(__uuidof(IDXGIOutput1), (void**)&dxgiOutput1);
    dxgiOutput->Release();
    if (FAILED(hr)) return false;

    hr = dxgiOutput1->DuplicateOutput(v_pd3dDevice, &v_pDuplication);
    dxgiOutput1->Release();
    if (FAILED(hr)) return false;

    // Create a staging texture to copy the screen data to CPU
    DXGI_OUTDUPL_DESC dupDesc;
    v_pDuplication->GetDesc(&dupDesc);
    v_ScreenWidth = (int)dupDesc.ModeDesc.Width;
    v_ScreenHeight = (int)dupDesc.ModeDesc.Height;

    D3D11_TEXTURE2D_DESC texDesc;
    texDesc.Width = dupDesc.ModeDesc.Width;
    texDesc.Height = dupDesc.ModeDesc.Height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = dupDesc.ModeDesc.Format;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_STAGING;
    texDesc.BindFlags = 0;
    texDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    texDesc.MiscFlags = 0;

    hr = v_pd3dDevice->CreateTexture2D(&texDesc, nullptr, &v_pStagingTexture);
    if (FAILED(hr)) return false;

    return true;
}

void CleanupVision()
{
    if (v_pStagingTexture) { v_pStagingTexture->Release(); v_pStagingTexture = nullptr; }
    if (v_pDuplication) { v_pDuplication->Release(); v_pDuplication = nullptr; }
    if (v_pd3dDeviceContext) { v_pd3dDeviceContext->Release(); v_pd3dDeviceContext = nullptr; }
    if (v_pd3dDevice) { v_pd3dDevice->Release(); v_pd3dDevice = nullptr; }
}

D3D11_MAPPED_SUBRESOURCE mappedScreen;

bool CaptureFrame()
{
    if (!v_pDuplication) return false;

    IDXGIResource* desktopResource = nullptr;
    DXGI_OUTDUPL_FRAME_INFO frameInfo;
    HRESULT hr = v_pDuplication->AcquireNextFrame(0, &frameInfo, &desktopResource);
    
    if (hr == DXGI_ERROR_WAIT_TIMEOUT) return true; // Not an error, just no new frame
    if (FAILED(hr)) return false;

    ID3D11Texture2D* desktopTexture = nullptr;
    hr = desktopResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&desktopTexture);
    desktopResource->Release();
    if (FAILED(hr))
    {
        v_pDuplication->ReleaseFrame();
        return false;
    }

    // Copy to CPU readable staging texture
    v_pd3dDeviceContext->CopyResource(v_pStagingTexture, desktopTexture);
    desktopTexture->Release();
    v_pDuplication->ReleaseFrame();

    // Unmap previous if any
    v_pd3dDeviceContext->Unmap(v_pStagingTexture, 0);

    // Map new
    hr = v_pd3dDeviceContext->Map(v_pStagingTexture, 0, D3D11_MAP_READ, 0, &mappedScreen);
    if (FAILED(hr)) return false;

    return true;
}

PixelColor GetPixel(int x, int y)
{
    if (!mappedScreen.pData) return {0,0,0,0};
    if (x < 0 || y < 0 || x >= v_ScreenWidth || y >= v_ScreenHeight) return {0,0,0,0};
    // Format is typically BGRA
    unsigned char* pBytes = (unsigned char*)mappedScreen.pData;
    int offset = y * mappedScreen.RowPitch + x * 4;
    return { pBytes[offset+2], pBytes[offset+1], pBytes[offset], pBytes[offset+3] };
}

bool is_prompt_light(int x, int y)
{
    // No offset needed if we use absolute screen coordinates (unlike python which used fixed bbox)
    PixelColor c = GetPixel(x, y);
    return (c.r >= 180 && c.g >= 180 && c.b >= 180 && 
            std::abs((int)c.r - (int)c.g) <= 45 && std::abs((int)c.r - (int)c.b) <= 45);
}

bool area_has_heart_prompt()
{
    struct Point { int x, y; };
    Point points[] = {
        {430, 778}, {445, 772}, {460, 772}, {475, 778},
        {414, 798}, {476, 798}, {414, 832}, {476, 832},
        {430, 852}, {445, 858}, {460, 858}, {475, 852}
    };
    int hits = 0;
    for (const auto& p : points) {
        if (is_prompt_light(p.x, p.y)) hits++;
    }
    return hits >= 4;
}

bool area_has_start_prompt()
{
    struct Point { int x, y; };
    Point points[5];
    if (MALE_BAR_END_POS.x != 0 || MALE_BAR_END_POS.y != 0) {
        points[0] = { MALE_BAR_END_POS.x + 40, MALE_BAR_END_POS.y - 2 };
        points[1] = { MALE_BAR_END_POS.x + 43, MALE_BAR_END_POS.y - 2 };
        points[2] = { MALE_BAR_END_POS.x + 67, MALE_BAR_END_POS.y - 30 };
        points[3] = { MALE_BAR_END_POS.x + 94, MALE_BAR_END_POS.y - 2 };
        points[4] = { MALE_BAR_END_POS.x + 67, MALE_BAR_END_POS.y + 26 };
    } else {
        points[0] = { 393, 915 };
        points[1] = { 396, 915 };
        points[2] = { 420, 887 };
        points[3] = { 447, 915 };
        points[4] = { 420, 943 };
    }
    int hits = 0;
    for (const auto& p : points) {
        if (is_prompt_light(p.x, p.y)) hits++;
    }
    return hits >= 4;
}

const char* GetMaleBarState()
{
    if (MALE_BAR_POS.x == 0 && MALE_BAR_POS.y == 0) return "UNKNOWN";

    int pink_count = 0;
    int grey_count = 0;
    int total_end = 0;
    int pink_at_end = 0;

    // 1. Check for Pink at the end point (if calibrated)
    if (MALE_BAR_END_POS.x != 0 || MALE_BAR_END_POS.y != 0) {
        int e_radius = max(6, (int)std::round(10 * g_WindowScale));
        int e_start_x = max(0, MALE_BAR_END_POS.x - e_radius);
        int e_end_x = min(v_ScreenWidth - 1, MALE_BAR_END_POS.x + e_radius);
        int e_y = MALE_BAR_END_POS.y;
        for (int x = e_start_x; x <= e_end_x; ++x) {
            PixelColor c = GetPixel(x, e_y);
            total_end++;
            if (c.r >= 145 && c.b >= 75 && ((int)c.r - (int)c.g) >= 35 && ((int)c.b - (int)c.g) >= 5) {
                pink_at_end++;
            }
        }
    }

    // 2. Check area along male bar
    int start_x = 0;
    int end_x = 0;
    if (MALE_BAR_END_POS.x != 0 && MALE_BAR_END_POS.x > MALE_BAR_POS.x) {
        start_x = max(0, MALE_BAR_POS.x);
        end_x = min(v_ScreenWidth - 1, MALE_BAR_END_POS.x);
    } else {
        int span = max(100, (int)std::round(150 * g_WindowScale));
        start_x = max(0, MALE_BAR_POS.x - span);
        end_x = min(v_ScreenWidth - 1, MALE_BAR_POS.x + span);
    }
    int y = MALE_BAR_POS.y;

    for (int x = start_x; x <= end_x; ++x) {
        PixelColor c = GetPixel(x, y);
        // Pink check
        if (c.r >= 145 && c.b >= 75 && ((int)c.r - (int)c.g) >= 35 && ((int)c.b - (int)c.g) >= 5) {
            pink_count++;
        }
        // Grey check (lenient)
        else if (c.r >= 15 && c.r <= 200 && 
            std::abs((int)c.r - (int)c.g) <= 25 && 
            std::abs((int)c.g - (int)c.b) <= 25 && 
            std::abs((int)c.r - (int)c.b) <= 25) {
            grey_count++;
        }
    }

    if (total_end > 0) {
        int req_pink = max(2, (int)std::round(3 * g_WindowScale));
        if (pink_at_end >= req_pink) return "FULL_PINK";
    } else {
        // Fallback if END POS is not calibrated (using old logic)
        if (pink_count > 0 && grey_count == 0) return "FULL_PINK"; 
    }

    if (pink_count > 0) return "PARTIAL_PINK";
    
    // If no pink at all, but there is some grey/dark background
    if (grey_count > 0) return "FULL_GREY";

    return "UNKNOWN";
}

bool IsFemaleArrowYellow()
{
    if (FEMALE_ARROW_POS.x == 0 && FEMALE_ARROW_POS.y == 0) return false;

    // Check area around the point for yellow, scaled to window resolution
    int radius = max(8, (int)std::round(10 * g_WindowScale));
    int start_x = max(0, FEMALE_ARROW_POS.x - radius);
    int end_x = min(v_ScreenWidth - 1, FEMALE_ARROW_POS.x + radius);
    int start_y = max(0, FEMALE_ARROW_POS.y - radius);
    int end_y = min(v_ScreenHeight - 1, FEMALE_ARROW_POS.y + radius);

    for (int y = start_y; y <= end_y; ++y) {
        for (int x = start_x; x <= end_x; ++x) {
            PixelColor c = GetPixel(x, y);
            if (c.r >= 170 && c.g >= 135 && c.b <= 115 && 
                ((int)c.r - (int)c.b) >= 85 && ((int)c.g - (int)c.b) >= 65) {
                return true;
            }
        }
    }
    return false;
}

bool IsMaleArrowYellow()
{
    if (MALE_ARROW_POS.x == 0 && MALE_ARROW_POS.y == 0) return false;

    // Check area around the point for yellow, scaled to window resolution
    int radius = max(8, (int)std::round(10 * g_WindowScale));
    int start_x = max(0, MALE_ARROW_POS.x - radius);
    int end_x = min(v_ScreenWidth - 1, MALE_ARROW_POS.x + radius);
    int start_y = max(0, MALE_ARROW_POS.y - radius);
    int end_y = min(v_ScreenHeight - 1, MALE_ARROW_POS.y + radius);

    for (int y = start_y; y <= end_y; ++y) {
        for (int x = start_x; x <= end_x; ++x) {
            PixelColor c = GetPixel(x, y);
            if (c.r >= 170 && c.g >= 135 && c.b <= 115 && 
                ((int)c.r - (int)c.b) >= 85 && ((int)c.g - (int)c.b) >= 65) {
                return true;
            }
        }
    }
    return false;
}

bool IsHeartButtonPresent()
{
    if (HEART_L_POS.x == 0 && HEART_L_POS.y == 0) return area_has_heart_prompt(); // fallback to static

    // Check area for the bright white hexagon border, scaled to window resolution
    int radius = max(10, (int)std::round(15 * g_WindowScale));
    int start_x = max(0, HEART_L_POS.x - radius);
    int end_x = min(v_ScreenWidth - 1, HEART_L_POS.x + radius);
    int start_y = max(0, HEART_L_POS.y - radius);
    int end_y = min(v_ScreenHeight - 1, HEART_L_POS.y + radius);

    for (int y = start_y; y <= end_y; ++y) {
        for (int x = start_x; x <= end_x; ++x) {
            PixelColor c = GetPixel(x, y);
            if (c.r >= 220 && c.g >= 220 && c.b >= 220) {
                return true;
            }
        }
    }
    return false;
}

bool IsGaugeSliceBlue(const Point2D& pos)
{
    if (pos.x == 0 && pos.y == 0) return false;

    // Check area around the point for light blue, scaled to window resolution
    int radius = max(4, (int)std::round(5 * g_WindowScale));
    int start_x = max(0, pos.x - radius);
    int end_x = min(v_ScreenWidth - 1, pos.x + radius);
    int start_y = max(0, pos.y - radius);
    int end_y = min(v_ScreenHeight - 1, pos.y + radius);

    int cyan_hits = 0;
    for (int y = start_y; y <= end_y; ++y) {
        for (int x = start_x; x <= end_x; ++x) {
            PixelColor c = GetPixel(x, y);
            // Stricter cyan/blue check for the gauge
            if (c.b > 180 && c.g > 140 && c.r < 130) {
                cyan_hits++;
            }
        }
    }
    
    int total_pixels = (end_x - start_x + 1) * (end_y - start_y + 1);
    int required_hits = max(4, (int)std::round(total_pixels * 0.12f));
    return cyan_hits >= required_hits;
}

bool IsPinkApproaching(const Point2D& pos)
{
    if (pos.x == 0 && pos.y == 0) return false;

    // Check an area [x-30, x-10] and [y-30, y+30]
    int start_x = max(0, pos.x - 30);
    int end_x = max(0, pos.x - 10);
    int start_y = max(0, pos.y - 30);
    int end_y = pos.y + 30;

    int pink_hits = 0;
    for (int y = start_y; y <= end_y; ++y) {
        for (int x = start_x; x <= end_x; ++x) {
            PixelColor c = GetPixel(x, y);
            if (c.r >= 145 && c.b >= 75 && ((int)c.r - (int)c.g) >= 35 && ((int)c.b - (int)c.g) >= 5) {
                pink_hits++;
                if (pink_hits > 5) return true;
            }
        }
    }
    return false;
}
