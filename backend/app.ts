import express from "express";
import { createServer } from "http";
import { WebSocketServer, WebSocket, RawData } from "ws";
import path from "path";

const app = express();
const server = createServer(app);
const wss = new WebSocketServer({ server });

const clients: Set<WebSocket> = new Set();
let broadcaster: WebSocket | null = null;

let totalBytesSent = 0;
let totalBytesReceived = 0;
let trafficInterval: NodeJS.Timeout | null = null;

const processMessage = (msg: RawData) => {
    const isBuffer = Buffer.isBuffer(msg);
    const isArray = Array.isArray(msg);
    const size = isBuffer
        ? msg.length
        : isArray
        ? msg.reduce((sum, buf) => sum + buf.length, 0)
        : Buffer.from(msg).length;
    const text = isBuffer
        ? msg.toString()
        : isArray
        ? Buffer.concat(msg).toString()
        : Buffer.from(msg).toString();
    return { size, text };
};

const startTrafficMonitoring = () => {
    if (!trafficInterval) {
        trafficInterval = setInterval(() => {
            const mbSent = (totalBytesSent / (1024 * 1024)).toFixed(2);
            const mbReceived = (totalBytesReceived / (1024 * 1024)).toFixed(2);
            const mbitSentPerSec = (
                (totalBytesSent * 8) /
                (1024 * 1024)
            ).toFixed(2);
            const mbitReceivedPerSec = (
                (totalBytesReceived * 8) /
                (1024 * 1024)
            ).toFixed(2);
            console.log(
                `[TRAFFIC] Sent: ${mbSent} MB (${mbitSentPerSec} Mbit/s), Received: ${mbReceived} MB (${mbitReceivedPerSec} Mbit/s)`
            );
            totalBytesSent = 0;
            totalBytesReceived = 0;
        }, 1000);
    }
};

const stopTrafficMonitoring = () => {
    if (trafficInterval) {
        clearInterval(trafficInterval);
        trafficInterval = null;
    }
};

wss.on("connection", (ws, req) => {
    ws.once("message", (msg: RawData) => {
        const { size, text } = processMessage(msg);
        totalBytesReceived += size;

        if (text === "BACKEND") {
            broadcaster = ws;
            console.log("[+] Broadcaster connected");
            startTrafficMonitoring();

            ws.on("message", (frame: RawData) => {
                const { size } = processMessage(frame);
                totalBytesReceived += size;

                for (const client of clients) {
                    if (client.readyState === WebSocket.OPEN) {
                        client.send(frame);
                        totalBytesSent += size;
                    }
                }
            });

            ws.on("close", () => {
                broadcaster = null;
                stopTrafficMonitoring();
                console.log("[-] Broadcaster disconnected");
            });
        } else if (text === "CLIENT") {
            clients.add(ws);
            console.log("[+] Client connected");

            ws.on("message", (msg: RawData) => {
                const { size } = processMessage(msg);
                totalBytesReceived += size;

                if (broadcaster?.readyState === WebSocket.OPEN) {
                    broadcaster.send(msg);
                }
            });

            ws.on("close", () => {
                clients.delete(ws);
                console.log("[-] Client disconnected");
            });
        } else {
            ws.close();
        }
    });
});

app.get("/", (_, res) => {
    res.sendFile(path.join(__dirname, "../../frontend/index.html"));
});

const PORT = process.env.PORT || 8080;
server.listen(PORT, () => {
    console.log(`[HTTP] Server running on port ${PORT}`);
});
