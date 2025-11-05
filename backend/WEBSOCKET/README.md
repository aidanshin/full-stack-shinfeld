# WebSocket (C++) Implementation

This module provides a **custom WebSocket implementation in C++**, following the official WebSocket protocol specification:

🔗 RFC 6455: https://www.rfc-editor.org/rfc/rfc6455

## Overview

This implementation establishes a WebSocket connection by performing the required **HTTP Upgrade handshake** over an existing **TCP connection**. After the handshake is complete, the connection transitions into a **full-duplex, persistent communication channel**, enabling efficient, real-time message exchange.

Full control over:

- Connection lifecycle
- Frame parsing & encoding
- Data buffering
- Threading and synchronization
- Message flow handling

This allows the WebSocket to integrate seamlessly with the project’s **custom network transport layer**.

## Key Features

| Feature | Description |
|--------|-------------|
| Custom Handshake | Performs WebSocket HTTP Upgrade with SHA-1 + Base64 accept key generation |
| Full Duplex Communication | Supports simultaneous send/receive using independent worker threads |
| Frame Encoding/Decoding | Implements WebSocket frame format per RFC 6455 (FIN, opcode, masking, payload len, etc.) |
| Binary & Text Message Support | Both payload types are correctly packed and transmitted |
| Thread-Safe Queues | Ensures safe message passing between producer/consumer threads |
| Graceful Shutdown | Handles connection close frames and socket teardown cleanly |

## Architecture

Once the WebSocket handshake is completed:

- **Receiver Thread**
  - Reads incoming frames from the socket
  - Decodes payload and dispatches it to application logic

- **Sender Thread**
  - Monitors an outgoing message queue
  - Encodes data into WebSocket frames
  - Writes frames to the socket

This enables **non-blocking, low-latency** communication without stalling the main application.

