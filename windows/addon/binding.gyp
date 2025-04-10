{
  "targets": [
        {
            "target_name": "screen_capture",
            "sources": ["screen_capture.cc"],
            "include_dirs": ["<!@(node -p \"require('node-addon-api').include\")"],
            "libraries": ["-lgdi32"],
            "dependencies": ["<!(node -p \"require('node-addon-api').gyp\")"],
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