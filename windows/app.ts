import WebSocket from "ws";
import sharp from "sharp";
import path from "path";

const screen = require("../addon/build/Release/screen_capture.node");
const URL = "ws://localhost:8080";
const ws = new WebSocket(URL);

let interval: NodeJS.Timeout | null = null;
let isRecording = false;
let resolution = 1080;
let monitorIndex = 0;
const FPS = 5;

function getTimestampedFilename(resolution: number) {
    const now = new Date();
    const dateStr = now.toISOString().replace(/[:.]/g, "-");
    return `record_${resolution}p_${dateStr}.avi`;
}

ws.on("open", () => {
    ws.send("BACKEND");
    console.log("[+] Connected to backend");

    interval = setInterval(async () => {
        try {
            const frame = screen.capture(resolution, monitorIndex);

            if (!isRecording) {
                const filename = getTimestampedFilename(resolution);
                screen.startRecording(
                    path.join(__dirname, `../../videos/${filename}`),
                    frame.width,
                    frame.height
                );
                isRecording = true;
                console.log(`[REC] Recording started at ${resolution}p`);
            }

            screen.pushFrameToRecording(frame.data, frame.width, frame.height);

            const webp = await sharp(frame.data, {
                raw: {
                    width: frame.width,
                    height: frame.height,
                    channels: 3,
                },
            })
                .webp({ quality: 85 })
                .toBuffer();

            if (ws.readyState === WebSocket.OPEN) {
                ws.send(webp);
            }
        } catch (err) {
            console.error("[!] Capture error:", err);
        }
    }, 1000 / FPS);
});

ws.on("message", (msg) => {
    try {
        const parsed = JSON.parse(msg.toString());

        if (parsed.resolution) {
            resolution = parseInt(parsed.resolution);
            if (isRecording) {
                screen.stopRecording();
                isRecording = false;
                console.log(
                    "[REC] Recording restarted due to resolution change"
                );
            }
        }

        if (parsed.monitorIndex) {
            monitorIndex = parseInt(parsed.monitorIndex);
            console.log(`[CFG] Monitor index changed to ${monitorIndex}`);
        }
    } catch (err) {}
});

ws.on("close", () => {
    console.log("[+] Disconnected from backend");

    if (interval) {
        clearInterval(interval);
        interval = null;
    }

    if (isRecording) {
        screen.stopRecording();
        isRecording = false;
        console.log("[REC] Recording stopped");
    }
});
