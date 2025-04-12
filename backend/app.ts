import express from "express";
import { createServer } from "http";
import { WebSocketServer, WebSocket } from "ws";

const PORT = parseInt(process.env.PORT || "8080");
const app = express();
const server = createServer(app);

const wss = new WebSocketServer({ noServer: true });
const clients: Set<WebSocket> = new Set();
let wSocket: WebSocket | null = null;

wss.on("connection", (ws, req) => {
  ws.once("message", (message) => {
    const msg = message.toString();

    if (msg === "BACKEND") {
      console.log("[+] Backend connected");
      wSocket = ws;

      ws.on("message", (frame) => {
        for (const client of clients) {
          if (client.readyState === WebSocket.OPEN) {
            client.send(frame);
          }
        }
      });

      ws.on("close", () => {
        console.log("[+] Backend disconnected");
        wSocket = null;
      });

    } else if (msg === "CLIENT") {
      clients.add(ws);
      console.log("[+] Client connected");

      ws.on("message", (msg) => {
        if (wSocket?.readyState === WebSocket.OPEN) {
          wSocket.send(msg);
        }
      });

      ws.on("close", () => {
        clients.delete(ws);
        console.log("[+] Client disconnected");
      });

    } else {
      console.log("Unknown connection type:", msg);
      ws.close();
    }
  });
});

server.on("upgrade", (req, socket, head) => {
  if (req.url === "/ws") {
    wss.handleUpgrade(req, socket, head, (ws) => {
      wss.emit("connection", ws, req);
    });
  } else {
    socket.destroy();
  }
});

app.get("/", (_, res) => {
  res.send("Screen broadcast backend is running.");
});

server.listen(PORT, () => {
  console.log(`[HTTP] Listening on port ${PORT}`);
});
