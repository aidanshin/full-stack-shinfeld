import { handlePacket } from "./PacketHandler";

export default class WebSocketApi {
    private socket: WebSocket | null = null;

    private url: string;

    constructor(url: string) {
        this.url = url;
    }
    
    connect() {
        this.socket = new WebSocket(this.url);
        this.socket.binaryType = "arraybuffer";

        this.socket.onopen = () => {
            console.log("WebSocket Connected:", this.url);
        };

        this.socket.onmessage = (event) => {
            if(event.data instanceof ArrayBuffer) {
                const data = new Uint8Array(event.data);
                handlePacket(data);
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