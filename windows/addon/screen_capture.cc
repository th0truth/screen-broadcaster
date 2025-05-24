#include "screen.h"

// Setup monitors 
std::vector<MonitorInfo> monitors;
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC, LPRECT rect, LPARAM) {
    monitors.push_back({hMonitor, *rect});
    return TRUE;
}

void InitMonitors() {
    monitors.clear();
    EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, 0);
}

// BGR to RGB conversion
void ConvertBGRtoRGB(uint8_t* data, size_t size) {
    const __m128i shuffleMask = _mm_set_epi8(
        15, 14, 13, 12, 9, 10, 11, 6, 7, 8, 3, 4, 5, 0, 1, 2
    );
    size_t i = 0;    
    size_t sseLimit = size - (size % 12);
    for (; i + 15 < sseLimit; i += 12) {
        __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(data + i));
        __m128i shuffled = _mm_shuffle_epi8(chunk, shuffleMask);
        _mm_storeu_si128(reinterpret_cast<__m128i*>(data + i), shuffled);
    }
    for (; i + 2 < size; i += 3) {
        std::swap(data[i], data[i + 2]);
    }
}

Value ScreenCapture(const CallbackInfo& info) {
    Env env = info.Env();

    int tWidth = 0, tHeight = 0;
    if (info.Length() > 0 || !info[0].IsNumber()) {
        const int resolution = info[0].As<Number>().Int32Value();
        for (const auto& res : RESOLUTIONS) {
            if (resolution == res[1]) {
                tWidth = res[0];
                tHeight = res[0];
                break;
            }
        }
    } else {
        throw Error::New(env, "Resolution parameter required");
    }

    int monitorIndex = 0;
    if (info.Length() > 1 && info[1].IsNumber()) {
        monitorIndex = info[1].As<Number>().Int32Value();
    }
    InitMonitors();
    if (monitorIndex < 0  || monitorIndex >= static_cast<int>(monitors.size())) {
        throw Error::New(env, "Invalid monitor index");
    }

    const auto& monitor = monitors[monitorIndex];
    const int sWidth = monitor.rect.right - monitor.rect.left;
    const int sHeight = monitor.rect.bottom - monitor.rect.top;
    tHeight = static_cast<int>(tWidth / (static_cast<float>(sWidth) / sHeight));
    
    // GDI resources
    ScreenDC screenDC;
    UniqueHDC memDC(CreateCompatibleDC(screenDC));
    UniqueHBITMAP bitmap(CreateCompatibleBitmap(screenDC, sWidth, sHeight));
    if (!memDC || !bitmap) {
        throw Error::New(env, "Failed to create GDI resources");
    }

    SelectObject(memDC.get(), bitmap.get());
    if (!BitBlt(memDC.get(), 0, 0, sWidth, sHeight, screenDC, 
                monitor.rect.left, monitor.rect.top, SRCCOPY)) {
        throw Error::New(env, "Screen capture failed");
    }

    UniqueHDC scaleDC(CreateCompatibleDC(screenDC));
    UniqueHBITMAP scaledBitmap(CreateCompatibleBitmap(screenDC, tWidth, tHeight));
    if (!scaleDC || !scaledBitmap) {
        throw Error::New(env, "Failed to create scaling resources");
    }

    SelectObject(scaleDC.get(), scaledBitmap.get());
    SetStretchBltMode(scaleDC.get(), COLORONCOLOR);
    if (!StretchBlt(scaleDC.get(), 0, 0, tWidth, tHeight,
                    memDC.get(), 0, 0, sWidth, sHeight, SRCCOPY)) {
        throw Error::New(env, "Image scaling failed");
    }

    // Pixel buffer setup
    BITMAPINFOHEADER bi{sizeof(BITMAPINFOHEADER), tWidth, -tHeight, 1, 24};
    const size_t bufferSize = tWidth * tHeight * 3;
    AlignedBuffer pixelData(static_cast<uint8_t*>(_aligned_malloc(bufferSize, 32)));
    if (!pixelData) {
        throw Error::New(env, "Memory allocation failed");
    }
    
    // Retrieve pixels
    if (!GetDIBits(scaleDC.get(), scaledBitmap.get(), 0, tHeight,
    pixelData.get(), reinterpret_cast<BITMAPINFO*>(&bi), DIB_RGB_COLORS)) {
        throw Error::New(env, "Failed to get pixel data");
    }

    // Convert BGR to RGB
    ConvertBGRtoRGB(pixelData.get(), bufferSize);

    // Create buffer without copy
    auto finalizer = [](Env, uint8_t* data) { _aligned_free(data); };
    Buffer<uint8_t> buffer = Buffer<uint8_t>::New(
        env, pixelData.release(), bufferSize, finalizer);
    
    // return an object
    Object result = Object::New(env);
    result.Set("width", Number::New(env, tWidth));
    result.Set("height", Number::New(env, tHeight));
    result.Set("data", buffer);
    return result;
};