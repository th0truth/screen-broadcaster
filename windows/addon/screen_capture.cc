#include <napi.h>
#include <windows.h>
#include <wingdi.h>
#include <cstdint>
#include <immintrin.h>

using namespace Napi;

// capture second monitor

Value ScreenCapture(const CallbackInfo& info) {
    Env env = info.Env();
    
    // get screen resolution
    HDC ScreenDC = GetDC(NULL);
    if (!ScreenDC) {
        Error::New(env, "GetDC failed.").ThrowAsJavaScriptException();
        return env.Null();
    }

    int sWidth = GetDeviceCaps(ScreenDC, HORZRES);
    int sHeight = GetDeviceCaps(ScreenDC, VERTRES);
    int tWidth = sWidth;
    int tHeight = sHeight;
    if (info.Length() > 0 && info[0].IsNumber()){
        int resolution = info[0].As<Number>().Int32Value();
        switch (resolution)
        {
        case 1440: tWidth = 2560; tHeight = 1440; break;
        case 1080: tWidth = 1920; tHeight = 1080; break;
        case 720: tWidth = 1280; tHeight = 720; break;
        default:
            Error::New(env, "Invalid resolution format");
            break;
        }

        float aspectRation  = static_cast<float>(sWidth) / sHeight;
        tHeight = static_cast<int>(tWidth / aspectRation);
    };

    // create compatible DC and bitmap
    HDC MemoryDC = CreateCompatibleDC(ScreenDC);
    HBITMAP Bitmap = CreateCompatibleBitmap(ScreenDC, sWidth, sHeight); 
    SelectObject(MemoryDC, Bitmap);
    BitBlt(MemoryDC, 0, 0, sWidth, sHeight, ScreenDC, 0, 0, SRCCOPY);

    // create scaled bitmap
    HDC ScaleDC = CreateCompatibleDC(ScreenDC);
    HBITMAP ScaledBitmap = CreateCompatibleBitmap(ScreenDC, tWidth, tHeight);
    SelectObject(ScaleDC, ScaledBitmap);
    SetStretchBltMode(ScaleDC, HALFTONE);
    StretchBlt(ScaleDC, 0, 0, tWidth, tHeight,
               MemoryDC, 0, 0, sWidth, sHeight, SRCCOPY);
    
    //  prepare bitmap info (24-bit BGR)
    BITMAPINFOHEADER bi = {0};
    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = tWidth;
    bi.biHeight = -tHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 24;
    bi.biCompression = BI_RGB;    
    
    // calc buffer size
    size_t bufferSize = tWidth * tHeight * 3; 
    
    std::uint8_t* pixelData = static_cast<std::uint8_t*>(_aligned_malloc(bufferSize, 32));
    // std::uint8_t pixelData(bufferSize);
    if (!pixelData) {
        Error::New(env, "Memory allocation failed").ThrowAsJavaScriptException();
        return env.Null();
    }
    
    // get the pixel data in BGR format
    if (!GetDIBits(ScaleDC, ScaledBitmap, 0, tHeight, pixelData, (BITMAPINFO*)&bi, DIB_RGB_COLORS)) {
        _mm_free(pixelData);
        DeleteObject(ScaledBitmap);
        DeleteDC(MemoryDC);
        DeleteDC(ScaleDC);
        ReleaseDC(NULL, ScreenDC);
        Error::New(env, "GetDIBits failed.").ThrowAsJavaScriptException();       
    }
    
    const __m128i shuffleMask = _mm_set_epi8(
        15, 14, 13, 12, 9, 10, 11, 6, 7, 8, 3, 4, 5, 0, 1, 2
    );
    
    // BGR to RGB conversion using SSSE3 instructions
    size_t i = 0;    
    size_t sseLimit = (bufferSize >= 16) ? (bufferSize - 15) : 0;
    for (; i < sseLimit; i += 12) {
        __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pixelData + i));
        __m128i shuffled = _mm_shuffle_epi8(chunk, shuffleMask);
        alignas(16) uint8_t temp[16];
        _mm_store_si128(reinterpret_cast<__m128i*>(temp), shuffled);
        memcpy(pixelData + i, temp, 12);
    }
    for (; i + 2 < bufferSize; i += 3) {
        std::swap(pixelData[i], pixelData[i + 2]);
    }

    // cleanup GDI objects
    DeleteObject(ScaledBitmap);
    DeleteDC(ScaleDC);
    DeleteObject(Bitmap);
    DeleteDC(MemoryDC);
    ReleaseDC(NULL, ScreenDC);

    // create a Node.js Buffer from the pixel data
    Buffer<uint8_t> buffer = Buffer<uint8_t>::Copy(env, pixelData, bufferSize);
    _aligned_free(pixelData);
    
    // return an object with width, height, and pixel data
    Object result = Object::New(env);
    result.Set("width", Number::New(env, tWidth));
    result.Set("height", Number::New(env, tHeight));
    result.Set("data", buffer);
    return result;
};

Object Init(Env env, Object exports) {
    exports.Set("capture", Function::New(env, ScreenCapture));
    return exports;
}

NODE_API_MODULE(screen_capture, Init)