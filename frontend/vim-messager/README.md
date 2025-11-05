# VIMMessage Frontend (React + TypeScript + Vite)

This is the **frontend** for the VIMMessage application, built with **React**, **TypeScript**, and **Vite**. It connects to the backend server (C++ implementation) and allows real-time, low-latency messaging between clients.

---

## Features

- **Specify backend server IP and PORT** for local testing.
  - Allows testing two clients simultaneously without changing the frontend code.
- **Connect to specific clients** via IP and PORT.
  - Connection details are sent to the backend, which handles peer-to-peer communication.
- **Real-time messaging** with peers.
- **WebSocketAPI**
  - Handles sending and receiving packets to/from the backend.
- **VIMPacketAPI**
  - Encodes and decodes messages with 1:1 correspondence to the backend C++ packet structure.
- **PacketHandler**
  - Manages newly received messages and triggers appropriate frontend updates.

---

## Workflow

1. User specifies the **backend IP and PORT** to connect.
2. User specifies the **target client** to connect to (IP + PORT).
3. Messages are **encoded via VIMPacketAPI**, sent through WebSocketAPI, and decoded by the backend.
4. Incoming messages are processed by **PacketHandler** and displayed in the UI in real-time.

---

## TODO

- Implement **VIM movements for keyboard commands**.
- Enable **multiple user communication** for full multi-peer messaging.
- Improve **UI/UX** for message display and connection management.

---

## Notes

- Initial focus was on **establishing functional communication between two clients** as a prototype.
- Backend integration is tightly coupled with the **custom C++ TCP/WebSocket protocol**.

