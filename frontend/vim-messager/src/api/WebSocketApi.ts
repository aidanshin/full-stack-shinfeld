import { handlePacket } from "./PacketHandler";
import type VIMPacket from "./VIMPacketApi";

type User = {
    id: number,
    hasNewMessage: boolean
}

export default class WebSocketApi {
    private socket: WebSocket | null = null;

    private url: string;

    constructor(url: string) {
        this.url = url;
    }
    
    connect(onPacket?: (packet: VIMPacket) => void, addUser?: (user: User) => void) {
        this.socket = new WebSocket(this.url);
        this.socket.binaryType = "arraybuffer";

        this.socket.onopen = () => {
            console.log("WebSocket Connected:", this.url);
        };

        this.socket.onmessage = (event) => {
            if(event.data instanceof ArrayBuffer) {
                const data = new Uint8Array(event.data);
                handlePacket(data, onPacket, addUser);
            } else if (typeof event.data === "string") {
                console.log("Message from server:", event.data);
            }
        };

        this.socket.onerror = (error) => {
            console.log("WebSocket error:", error);
        };

        this.socket.onclose = () => {
            console.log("WebSocket closed");
        };
    }

    send(packet: Uint8Array) {
        if (this.socket && this.socket.readyState == WebSocket.OPEN) {
            console.log("Sending packet", packet);
            this.socket.send(packet);
        } else {
            console.warn("WebSocket not connected.");
        }
    }

    disconnect() {
        if(this.socket) {
            this.socket.close();
            this.socket = null;
        }
    }
}