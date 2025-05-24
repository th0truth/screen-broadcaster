#include <thread>
#include <mutex>
#include <deque>
#include <atomic>
#include <fstream>
#include <vector>
#include <chrono>
#include <iostream>
#include <iomanip>

#include "screen.h"
#include "include/turbojpeg.h"

static std::atomic<bool> isRecording = false;
static std::thread recordingThread;
static std::mutex frameMutex;
static std::deque<std::vector<uint8_t>> frameQueue;
static std::ofstream aviFile;
static int recordWidth = 0, recordHeight = 0;
static int frameCount = 0;
static std::streampos riffSizePos, moviSizePos;

#pragma pack(push, 1)
struct MJPEG_BITMAPINFOHEADER  {
    uint32_t biSize = 40;
    int32_t  biWidth;
    int32_t  biHeight;
    uint16_t biPlanes = 1;
    uint16_t biBitCount = 24;
    uint32_t biCompression = 0x47504A4D; // 'MJPG'
    uint32_t biSizeImage = 0;
    int32_t  biXPelsPerMeter = 0;
    int32_t  biYPelsPerMeter = 0;
    uint32_t biClrUsed = 0;
    uint32_t biClrImportant = 0;
};

struct AVISTREAMHEADER {
    char fccType[4] = {'v','i','d','s'};
    char fccHandler[4] = {'M','J','P','G'};
    uint32_t flags = 0;
    uint16_t priority = 0;
    uint16_t language = 0;
    uint32_t initialFrames = 0;
    uint32_t scale = 1;
    uint32_t rate = 5;
    uint32_t start = 0;
    uint32_t length = 0;
    uint32_t suggestedBufferSize = 0;
    uint32_t quality = uint32_t(-1);
    uint32_t sampleSize = 0;
    struct { int16_t left, top, right, bottom; } rcFrame = {0, 0, 0, 0};
};
#pragma pack(pop)

void WriteAviHeader(std::ofstream& out, int width, int height) {
    out.write("RIFF", 4);
    riffSizePos = out.tellp();
    uint32_t dummySize = 0; out.write((char*)&dummySize, 4);
    out.write("AVI ", 4);

    out.write("LIST", 4);
    uint32_t hdrlSize = 4 + (8 + 56) + (12 + 56) + (8 + 40); // 'hdrl' + avih + strh + strf
    out.write((char*)&hdrlSize, 4);
    out.write("hdrl", 4);

    // avih
    out.write("avih", 4);
    uint32_t avihSize = 56; out.write((char*)&avihSize, 4);
    uint32_t microSecPerFrame = 1000000 / 5;
    uint32_t maxBytesPerSec = 10000000;
    uint32_t reserved = 0;
    uint32_t flags = 0x10;
    uint32_t initialFrames = 0;
    uint32_t streams = 1;
    uint32_t bufferSize = 0;
    uint32_t totalFrames = 0;

    out.write((char*)&microSecPerFrame, 4);
    out.write((char*)&maxBytesPerSec, 4);
    out.write((char*)&reserved, 4);
    out.write((char*)&flags, 4);
    out.write((char*)&totalFrames, 4);
    out.write((char*)&initialFrames, 4);
    out.write((char*)&streams, 4);
    out.write((char*)&bufferSize, 4);
    out.write((char*)&width, 4);
    out.write((char*)&height, 4);
    for (int i = 0; i < 4; ++i) out.write((char*)&reserved, 4);

    // strl
    out.write("LIST", 4);
    uint32_t strlSize = 56 + 8 + 40; out.write((char*)&strlSize, 4);
    out.write("strl", 4);

    // strh
    out.write("strh", 4);
    uint32_t strhSize = 56; out.write((char*)&strhSize, 4);
    AVISTREAMHEADER strh;
    strh.length = 0;
    strh.rcFrame.right = width;
    strh.rcFrame.bottom = height;
    out.write((char*)&strh, sizeof(strh));

    // strf
    out.write("strf", 4);
    uint32_t strfSize = 40; out.write((char*)&strfSize, 4);
    MJPEG_BITMAPINFOHEADER  bih;
    bih.biWidth = width;
    bih.biHeight = height;
    bih.biSizeImage = width * height * 3;
    out.write((char*)&bih, sizeof(bih));

    out.write("LIST", 4);
    moviSizePos = out.tellp();
    uint32_t moviSize = 0; out.write((char*)&moviSize, 4);
    out.write("movi", 4);
}

void FinalizeAvi() {
    if (!aviFile.is_open()) return;
    std::streampos fileEnd = aviFile.tellp();
    uint32_t moviSize = static_cast<uint32_t>(fileEnd - moviSizePos - 8);
    aviFile.seekp(moviSizePos);
    aviFile.write((char*)&moviSize, 4);

    uint32_t riffSize = static_cast<uint32_t>(fileEnd - riffSizePos - 4);
    aviFile.seekp(riffSizePos);
    aviFile.write((char*)&riffSize, 4);

    aviFile.close();
    std::cout << "[REC] Finalized AVI with " << frameCount << " frames\n";
}

void RecordingLoopAVI(const std::string& filename) {
    aviFile.open(filename, std::ios::binary);
    if (!aviFile.is_open()) return;

    WriteAviHeader(aviFile, recordWidth, recordHeight);

    while (isRecording) {
        std::vector<uint8_t> frame;
        {
            std::lock_guard<std::mutex> lock(frameMutex);
            if (!frameQueue.empty()) {
                frame = std::move(frameQueue.front());
                frameQueue.pop_front();
            }
        }

        if (!frame.empty()) {
            tjhandle jpegCompressor = tjInitCompress();
            unsigned char* jpegBuf = nullptr;
            unsigned long jpegSize = 0;

            int result = tjCompress2(jpegCompressor, frame.data(), recordWidth, 0, recordHeight, TJPF_RGB,
                        &jpegBuf, &jpegSize, TJSAMP_420, 75, TJFLAG_FASTDCT);
            if (result != 0) {
                std::cerr << "[!] Compression error: " << tjGetErrorStr() << "\n";
                tjDestroy(jpegCompressor);
                continue;
            }

            aviFile.write("00dc", 4);
            uint32_t size = static_cast<uint32_t>(jpegSize);
            aviFile.write((char*)&size, 4);
            aviFile.write((char*)jpegBuf, size);

            tjDestroy(jpegCompressor);
            tjFree(jpegBuf);

            frameCount++;
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    FinalizeAvi();
}

void PushFrameToRecording(uint8_t* rgb, int width, int height) {
    if (!isRecording) return;
    std::vector<uint8_t> copy(rgb, rgb + width * height * 3);
    {
        std::lock_guard<std::mutex> lock(frameMutex);
        frameQueue.push_back(std::move(copy));
    }
}

Value PushFrameWrapper(const CallbackInfo& info) {
    Env env = info.Env();
    if (!info[0].IsBuffer() || !info[1].IsNumber() || !info[2].IsNumber()) {
        throw Error::New(env, "Expected (data: Buffer, width: number, height: number)");
    }
    Buffer<uint8_t> buffer = info[0].As<Buffer<uint8_t>>();
    int width = info[1].As<Number>().Int32Value();
    int height = info[2].As<Number>().Int32Value();
    PushFrameToRecording(buffer.Data(), width, height);
    return env.Undefined();
}

Value StartRecording(const CallbackInfo& info) {
    Env env = info.Env();
    if (!info[0].IsString() || !info[1].IsNumber() || !info[2].IsNumber()) {
        throw Error::New(env, "Expected (filename: string, width: number, height: number)");
    }

    std::string filename = info[0].As<String>().Utf8Value();
    recordWidth = info[1].As<Number>().Int32Value();
    recordHeight = info[2].As<Number>().Int32Value();
    frameCount = 0;

    isRecording = true;
    recordingThread = std::thread(RecordingLoopAVI, filename);
    return env.Undefined();
}

Value StopRecording(const CallbackInfo& info) {
    isRecording = false;
    if (recordingThread.joinable()) recordingThread.join();
    return info.Env().Undefined();
}