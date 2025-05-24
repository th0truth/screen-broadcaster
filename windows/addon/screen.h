#pragma once

#include <napi.h>
#include <windows.h>
#include <wingdi.h>
#include <cstdint>
#include <immintrin.h>  
#include <vector>
#include <memory>

using namespace Napi;

struct MonitorInfo {
    HMONITOR hMonitor;
    RECT rect;
};

// RAII wrappers for GDI resources
struct GDIDeleter {
    void operator()(HBITMAP h) { if (h) DeleteObject(h); }
    void operator()(HDC h) { if (h) DeleteDC(h); }
};

using UniqueHDC = std::unique_ptr<std::remove_pointer<HDC>::type, GDIDeleter>;
using UniqueHBITMAP = std::unique_ptr<std::remove_pointer<HBITMAP>::type, GDIDeleter>;

// Screen DC wrapper
struct ScreenDC {
    HDC dc;
    ScreenDC() : dc(GetDC(nullptr)) {}
    ~ScreenDC() { if (dc) ReleaseDC(nullptr, dc); }
    operator HDC() const { return dc; }
};

// Monitor enumeration
extern std::vector<MonitorInfo> monitors;
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC, LPRECT rect, LPARAM);

void InitMonitors();
    
constexpr int RESOLUTIONS[][2] = {
    {1280, 720},
    {1920, 1080},
    {2560, 1440}
};

// BGR to RGB conversion with SSSE3 for better performance
void ConvertBGRtoRGB(uint8_t* data, size_t size);

// Aligned memory allocator
struct AlignedFree {
    void operator()(uint8_t* p) { _mm_free(p); }
};
using AlignedBuffer = std::unique_ptr<uint8_t[], AlignedFree>;