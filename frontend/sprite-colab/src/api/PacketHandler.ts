import VIMPacket from "./VIMPacketApi";

export function handlePacket(data: Uint8Array) {
    const packet = VIMPacket.decodePacket(data);

    if (!packet) return;
    console.log(packet);
    
    switch (packet.type){
        case 1:
            console.log(packet.msg_data);
            break;
        case 2: 
            console.log(packet.msg_data);
            break;
        case 3: 
            console.log(packet.ip, packet.port);
            break;
        case 4:
            console.log(packet.ip, packet.port, packet.userId);
            break;
        default:
            console.log("Unkown packet type");
    }
}