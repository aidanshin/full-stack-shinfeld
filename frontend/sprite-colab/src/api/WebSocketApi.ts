export default class WebSocketApi {
    private socket: WebSocket | null = null;

    private url: string;

    constructor(url: string) {
        this.url = url;
    }
    
    connect(onMessage?: (msg: string) => void) {
        this.socket = new WebSocket(this.url);

        this.socket.onopen = () => {
            console.log("WebSocket Connected:", this.url);
        };

        this.socket.onmessage = (event) => {
            console.log("Message from server:", event.data);
            if (onMessage) onMessage(event.data);
        };

        this.socket.onerror = (error) => {
            console.log("WebSocket error:", error);
        };

        this.socket.onclose = () => {
            console.log("WebSocket closed");
        };
    }

    send(message: string) {
        if (this.socket && this.socket.readyState == WebSocket.OPEN) {
            console.log("Sending Message:", message);
            this.socket.send(message);
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