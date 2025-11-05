# VIM-MESSAGE (C++)

**VIM-MESSAGE** is a C++ module that implements a **custom messaging protocol**, built on top of custom **TCP** and **WebSocket** layers. It enables **seamless message flow** between peers (P2P) or between clients and servers. The module supports real-time messaging, user creation, and message forwarding, making it suitable for distributed applications.

---

## Features

- Starts a **WebSocket server** to accept connections from UI clients.
- Maintains **threads to monitor messages** from both UI and TCP sources.
- Automatically forwards messages to the appropriate next location (peer or server).
- Supports **custom packet types** for messages, user creation, and connection confirmations.

---

## Packet Types & Structure

### 1. Message Packets
**SEND MSG / RECEIVE MSG**  
- **Type:** 1 byte (`1 = SEND MSG`, `2 = RECEIVE MSG`)  
- **UserID:** 4 bytes  
- **MsgID:** 2 bytes  
- **MSG Payload:** variable (WebSocket payload size minus 7 bytes for headers)  
- **Total Size:** 7 + payload size  

### 2. New User Creation Request
- **Type:** 1 byte (`3`)  
- **IP:** 4 bytes  
- **Port:** 2 bytes  
- **Total Size:** 7 bytes  

### 3. User Connection Confirmation
- **Type:** 1 byte (`4`)  
- **IP:** 4 bytes  
- **Port:** 2 bytes  
- **UserID:** 4 bytes  
- **Total Size:** 11 bytes  

---

## Workflow

1. **Start WebSocket server**
2. **Accept UI connections**
3. Start **threaded loops** to:
   - Check for messages from the UI
   - Check for messages from TCP connections
   - Forward messages to the next peer or server

This ensures **real-time message propagation** and allows the system to scale to multiple peers or centralized servers.

---

## TODO

- Implement **payload size in header** for proper packet parsing.
- Create an **automatic connection between frontend and backend**, with support for specifying backend locations during local testing.
- Implement **Message encryption** for secure communication.

