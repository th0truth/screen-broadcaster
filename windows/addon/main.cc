#include "screen.h"

// Exported functions
Value ScreenCapture(const CallbackInfo& info);
Value StartRecording(const CallbackInfo& info);
Value StopRecording(const CallbackInfo& info);
Value PushFrameWrapper(const CallbackInfo& info);

Object Init(Env env, Object exports) {
    exports.Set("capture", Function::New(env, ScreenCapture));
    exports.Set("startRecording", Function::New(env, StartRecording));
    exports.Set("stopRecording", Function::New(env, StopRecording));
    exports.Set("pushFrameToRecording", Function::New(env, PushFrameWrapper));
    return exports;
}

NODE_API_MODULE(screen_capture, Init)