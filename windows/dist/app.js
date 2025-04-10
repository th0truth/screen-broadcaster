"use strict";
var __importDefault = (this && this.__importDefault) || function (mod) {
    return (mod && mod.__esModule) ? mod : { "default": mod };
};
Object.defineProperty(exports, "__esModule", { value: true });
const ws_1 = __importDefault(require("ws"));
const sharp_1 = __importDefault(require("sharp"));
const ScreenCapture = require("../addon/build/Release/screen_capture.node");
const URL = "ws://localhost:8080";
const ws = new ws_1.default(URL);
let resolution = 1440;
const FPS = 10;
ws.on("open", () => {
    console.log("[+] Connected to backend");
    ws.send("SOURCE");
    setInterval(async () => {
        try {
            const frame = ScreenCapture.capture(resolution);
            const webp = await (0, sharp_1.default)(frame.data, {
                raw: {
                    width: frame.width,
                    height: frame.height,
                    channels: 3,
                },
            })
                .webp({ quality: 100 })
                .toBuffer();
            if (ws.readyState === ws_1.default.OPEN) {
                ws.send(webp);
            }
        }
        catch (err) {
            console.error("[+] Error capturing or sending frame:", err);
        }
    }, 1000 / FPS);
});
ws.on("message", (msg) => {
    try {
        const parsed = JSON.parse(msg.toString());
        if (parsed.resolution) {
            const newRes = parseInt(parsed.resolution);
            resolution = newRes;
        }
    }
    catch (err) {
        console.warn(`[+] Failed to parse message: ${err}`);
    }
});
ws.on("close", () => {
    console.log("[+] Disconnected from backend");
});
//# sourceMappingURL=app.js.map