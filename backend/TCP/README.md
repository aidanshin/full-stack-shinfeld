# Custom TCP Protocol (C++)

This module implements a **custom TCP-like reliable transport protocol** built on top of **UDP sockets**. The goal of this implementation is to reproduce the core functionality of TCP at the transport layer, while retaining the flexibility to support both **peer-to-peer** networking and **centralized server forwarding**.

This means:
- **Clients can directly connect to each other**, forming distributed networks.
- Or, clients can **connect only to a central server**, which will route messages to other peers.
- Any client can behave as a **server**, depending on the chosen topology.

---

## Key Features

| Feature | Description |
|--------|-------------|
| 3-Way Handshake | Custom SYN, SYN-ACK, ACK negotiation to establish reliable connections |
| 4-Way Teardown | Proper FIN, ACK connection close procedure |
| RTT Measurement | Round-trip time measurement for dynamic packet retransmission timing |
| Reliable Delivery | Lost packets are detected and retransmitted automatically |
| Packet Reordering | Out-of-order segments are stored and reassembled before delivery |
| Checksum Validation | Packet integrity is verified before processing |
| Sliding Window Support | Manages multiple in-flight packets for efficiency |
| Bitwise Header Control | Flags, sequence numbers, and length fields encoded exactly like TCP |

---

## Packet Format

This protocol uses **the same packet framing structure as TCP**, including:

| Field | Size | Description |
|------|------|-------------|
| Source Port | 16 bits | Sender port |
| Destination Port | 16 bits | Receiver port |
| Sequence Number | 32 bits | Byte index of first payload byte |
| Acknowledgment Number | 32 bits | Next expected byte |
| Flags (SYN, ACK, FIN, etc.) | variable bitfield | Controls connection state |
| Window Size | 16 bits | Sliding window capacity |
| Checksum | 16 bits | Data integrity check |
| Payload | variable | Application data |

Internally, these fields are encoded/decoded using **bit manipulation and masking**, ensuring full compatibility with standard TCP semantics.

---

## Reliability Mechanics

### RTT + Retransmission
- RTT is tracked continuously using timestamped packets.
- A smoothed RTO (Retransmission Timeout) is calculated.
- If an ACK is **not** received before the RTO expires → the segment is retransmitted.

### Out-of-Order Packet Handling
- Incoming packets are inserted into a **reordering buffer**.
- Delivery to the application occurs **only when sequence order is restored**, ensuring correctness.

---

## Connection Flexibility

### **Peer-to-Peer Mode**
Clients directly dial and maintain sessions with each other.

### **Central Forwarding Mode**
Clients connect only to a central server:
The server **does not inspect or modify message payloads** — it simply forwards transport frames.

This makes the system useful for:
- Messaging applications
- Game server relay networks
- Lightweight distributed services

---

## Threading & Concurrency

This implementation uses:
- Worker threads for send/receive loops
- Thread-safe queues for data passing
- Mutexes and condition variables for synchronization
- Graceful shutdown logic to prevent deadlocks

This ensures **low latency**, efficient routing, and stable connections.

---

## Status

| Component | Status |
|---------|--------|
| Connection Establishment | ✅ Complete |
| Reliability & Retransmission | ✅ Complete |
| Reordering + Buffers | ✅ Complete |
| Packet Encoding/Checksum | ✅ Complete |
| Server Forwarding Support | ✅ Complete |
| Congestion Control | 🚧 Optional / Future Improvement |
| TLS/Encryption | 🚧 Planned |

---
