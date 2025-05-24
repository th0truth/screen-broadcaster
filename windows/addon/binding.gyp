{
  "targets": [
        {
        "target_name": "screen_capture",
        "sources": ["main.cc", "screen_capture.cc", "screen_recorder.cc"],
        "include_dirs": [
            "<!@(node -p \"require('node-addon-api').include\")", "turbojpeg"],
        "dependencies": ["<!(node -p \"require('node-addon-api').gyp\")"],
        "libraries": ["-lgdi32", "-lMfplat", "-lMfreadwrite", "-lMfuuid", "<(module_root_dir)/lib/turbojpeg.lib"],
        "cflags!": ["-fno-exceptions"],
        "cflags_cc!": ["-fno-exceptions"],
        "defines": [ "NAPI_CPP_EXCEPTIONS", "NODE_ADDON_API_ENABLE_CPP_EXCEPTIONS" ],
        "msvs_settings": {
            "VCCLCompilerTool": {
                "ExceptionHandling": 1
            }
        }
        }
    ]
}