// TODO implement helper function to convert string to bytes for IP and port 
export type PacketType = 0 | 1 | 2 | 3 | 4;

export default class VIMPacket {
    #type : PacketType = 0;
    #user_id: number = 0;
    #msg_id: number = 0;
    #ip: string = "";
    #port: number = 0;
    #msg_data: string = "";

    static HEADER_SIZE : number = 7;
    static USER_CONFIRMATION_SIZE : number = 11;
    static decoder = new TextDecoder();
    static encoder = new TextEncoder();

    constructor(
        type: PacketType, 
        user_id?: number,
        msg_id?: number,
        ip?: string,
        port?: number,
        msg_data?: string
    ) {
        this.#type = type;
        if(user_id !== undefined) this.#user_id = user_id;
        if(msg_id !== undefined) this.#msg_id = msg_id;
        if(ip !== undefined) this.#ip = ip;
        if(port !== undefined) this.#port = port;
        if(msg_data !== undefined) this.#msg_data = msg_data;
    }

    get type(): PacketType {return this.#type;}
    get userId(): number {return this.#user_id;}
    get msgId(): number {return this.#msg_id;}
    get ip(): string {return this.#ip;}
    get port(): number {return this.#port;}
    get msg_data(): string {return this.#msg_data;}

    private static  ipToBytes(ip: string): Uint8Array | undefined{
        const vals = ip.split('.').map(p => parseInt(p, 10));
        if (vals.length !== 4 || vals.some(p => p < 0 || p > 255)) {
            console.error(`Invalid IP address: ${ip}`);
            return undefined;
        }
        return new Uint8Array(vals);
    }

    private static numberToBytes(val: number, size: number): Uint8Array {
        const buf = new Uint8Array(size);
        for(let i = 0; i < size; ++i) {
            buf[i] = (val >> (8*(size-(1+i)))) & 0xFF;
        }
        return buf;
    }

    static createMsgPacket(type: PacketType, userId: number, msgId: number, data: string) : Uint8Array | undefined {
        if(userId > 0xFFFFFFFF || userId <= 0) return undefined;
        if(msgId > 0xFFFF || msgId <= 0) return undefined; 

        const msgBytes = this.encoder.encode(data);
        const packet = new Uint8Array(this.HEADER_SIZE + msgBytes.length);

        packet[0] = type;
        packet.set(this.numberToBytes(userId, 4), 1);
        packet.set(this.numberToBytes(msgId, 2), 5);
        packet.set(msgBytes, this.HEADER_SIZE);

        return packet;
    }
    
    static createUserPacket(type: PacketType, ip: string, port: number) : Uint8Array | undefined {
        if(port > 0xFFFF || port < 0) return undefined;

        const ipBytes = this.ipToBytes(ip);
        if(!ipBytes) return undefined;

        const packet = new Uint8Array(this.HEADER_SIZE);
        const portBytes = this.numberToBytes(port, 2);

        packet[0] = type;
        packet.set(ipBytes, 1);
        packet.set(portBytes, 5);

        return packet;
    }

    static decodePacket(payload: Uint8Array): VIMPacket | undefined {
        const type = payload[0] as PacketType;
        switch(type) {
            case 1: case 2: return VIMPacket.decodeMsgPacket(payload);
            case 3: return VIMPacket.decodeUserCreation(payload);
            case 4: return VIMPacket.decodeUserConfirmation(payload)
            default: return undefined;
        }
    }

    private static decodeMsgPacket(payload: Uint8Array): VIMPacket | undefined {
        if (payload.length <= this.HEADER_SIZE) return undefined;
        const type = payload[0] as PacketType;
        const userId = (payload[1] << 24) | (payload[2] << 16) | (payload[3] << 8) | payload[4];
        const msgId = (payload[5] << 8) | payload[6];
        const buf = payload.subarray(7);
        const msgString = this.decoder.decode(buf);

        return new VIMPacket(type, userId, msgId, undefined, undefined, msgString);
    }

    private static decodeUserCreation(payload: Uint8Array): VIMPacket | undefined {
        if(payload.length != this.HEADER_SIZE) return undefined;
        const type = payload[0] as PacketType;
        const ipStr = `${payload[1]}.${payload[2]}.${payload[3]}.${payload[4]}`; 
        const port =  (payload[5] << 8) | payload[6];
        return new VIMPacket(type, undefined, undefined, ipStr, port, undefined);
    }

    private static decodeUserConfirmation(payload: Uint8Array): VIMPacket | undefined {
        if(payload.length != this.USER_CONFIRMATION_SIZE) return undefined;
        const type = payload[0] as PacketType;
        const ipStr = `${payload[1]}.${payload[2]}.${payload[3]}.${payload[4]}`; 
        const port =  (payload[5] << 8) | payload[6];
        const userId = (payload[7] << 24) | (payload[8] << 16) | (payload[9] << 8) | payload[10];
        return new VIMPacket(type, userId, undefined, ipStr, port, undefined);
    }

}