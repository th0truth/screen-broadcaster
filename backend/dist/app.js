"use strict";
Object.defineProperty(exports, "__esModule", { value: true });
const ws_1 = require("ws");
const wss = new ws_1.WebSocketServer({ port: 8080 });
const clients = new Set();
let screenSource = null;
wss.on("connection", (ws) => {
    ws.once("message", (message) => {
        const msg = message.toString();
        if (msg === "SOURCE") {
            console.log("[+] Backend connected");
            screenSource = ws;
            ws.on("message", (frame) => {
                for (const client of clients) {
                    if (client.readyState === ws_1.WebSocket.OPEN) {
                        client.send(frame);
                    }
                }
            });
            ws.on("close", () => {
                console.log("[+] Backend disconnected");
                screenSource = null;
            });
        }
        else if (msg === "CLIENT") {
            clients.add(ws);
            console.log("[+] Client connected");
            ws.on("close", () => {
                clients.delete(ws);
                console.log("[+] Client disconnected");
            });
        }
        else {
            console.log("Unknown connection type:", msg);
            ws.close();
        }
    });
});
