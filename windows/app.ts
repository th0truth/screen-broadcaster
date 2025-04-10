import WebSocket from "ws";
import sharp from "sharp";

const ScreenCapture = require("../addon/build/Release/screen_capture.node");

const URL = "ws://localhost:8080";
const ws = new WebSocket(URL);

let resolution = 1440;
const FPS = 10;

ws.on("open", () => {
    console.log("[+] Connected to backend");
    ws.send("SOURCE");
    setInterval(async () => {
        try {
            const frame = ScreenCapture.capture(resolution);
    
            const webp = await sharp(frame.data, {
                raw: {
                    width: frame.width,
                    height: frame.height,
                    channels: 3,
                },
            })
            .webp({ quality: 100 })
            .toBuffer();
    
            if (ws.readyState === WebSocket.OPEN) {
                ws.send(webp);
            }
        } catch (err) {
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
    } catch (err) {
        console.warn(`[+] Failed to parse message: ${err}`);
    }
});

ws.on("close", () => {
    console.log("[+] Disconnected from backend");
});