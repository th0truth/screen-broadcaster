import express from "express";
import { createServer } from "http";
import { WebSocketServer, WebSocket } from "ws";
import path from "path";

const app = express();
const server = createServer(app);
const wss = new WebSocketServer({ server });

const clients: Set<WebSocket> = new Set();
let broadcaster: WebSocket | null = null;

wss.on("connection", (ws, req) => {
  ws.once("message", (msg) => {
    const text = msg.toString();
    if (text === "BACKEND") {
      broadcaster = ws;
      console.log("[+] Broadcaster connected");

      ws.on("message", (frame) => {
        for (const client of clients) {
          if (client.readyState === WebSocket.OPEN) {
            client.send(frame);
          }
        }
      });

      ws.on("close", () => {
        broadcaster = null;
        console.log("[-] Broadcaster disconnected");
      });
    } else if (text === "CLIENT") {
      clients.add(ws);
      console.log("[+] Client connected");

      ws.on("message", (msg) => {
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