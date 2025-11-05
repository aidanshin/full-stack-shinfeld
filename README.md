# VIMMessager

## Overview
VIMMessager is a full-stack messenger application built with a **React + TypeScript frontend** and a **C++ backend**, communicating through a pair of **custom communication protocols**. The system is designed to be flexible and can operate either with or without a central server.

Starting from the transport layer, the backend implements a **custom TCP-like protocol** built on top of **UDP sockets** in C++. On top of this, a **custom WebSocket protocol** enables seamless communication between the frontend UI and the backend. Data flows from the user interface through the WebSocket layer to the backend, where it is encoded and delivered across the network using the custom TCP protocol.

Low-level concurrency mechanisms such as **threads**, **thread-safe queues**, **mutexes**, and **controlled shutdown logic** were used to ensure **performance, reliability, and low-latency message delivery**.

## Architecture

```
[ React + TypeScript Frontend ]
                |
                | (Custom WebSocket Protocol)
                v
          [ C++ Backend ]
                |
                | (Custom TCP Protocol over UDP)
                v
         [ Destination Client / Peer ]
```

## Key Features
- Custom TCP-like protocol implemented on top of UDP
- Custom WebSocket protocol enabling communication between C++ and React
- Thread-safe, asynchronous message passing
- Configurable messaging pipeline with optional central server
- Designed for low latency and efficient data transfer

## Technologies Used
- **Frontend:** React, TypeScript
- **Backend:** C++17
- **Networking:** UDP Sockets, Custom Protocol Serialization
- **Concurrency:** Threads, Mutexes, Condition Variables

## Build & Run Instructions

### Frontend
```
cd frontend/vim-messager
npm install
npm run dev -- --port #
```

### Backend
```
cd backend/VIM-MESSAGE
make run ARGS="tcpIP tcpPort websocketIP websocketPort"
./main tcpIP tcpPort websocketIP websocketPort
```

## Future Work
- Encryption and secure key exchange
- Improved reliability with congestion control
- Server discovery / peer-list synchronization

## License
MIT License


## Architecture
- **Flexible**: P2P or Server
- **Peer-to-Peer (P2P) Design:** Each client can directly connect to others without requiring a central server.


