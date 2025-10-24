import VIMPacket from "./VIMPacketApi";

type User = {
    id: number,
    hasNewMessage: boolean
}

export function handlePacket(data: Uint8Array, onMessage?: (packet: VIMPacket) => void, addUser?: (user: User) => void) {
    const packet = VIMPacket.decodePacket(data);

    if (!packet) return;
    console.log(packet);
    
    switch (packet.type){
        case 1:
        case 2: 
            if(onMessage) onMessage(packet);
            break;
        case 3:
            //User Request 
            console.log(packet.ip, packet.port);
            break;
        case 4:
            if (addUser) {
                addUser({id: packet.userId, hasNewMessage: false});
            }
            break;
        default:
            console.log("Unkown packet type");
    }
}