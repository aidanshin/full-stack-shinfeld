import React, { useEffect, useState } from 'react'
import "./ExtrasModal.css"
interface ExtraProps {
    onClose: () => void;
    sendRequest: (ip: string, port: number) => void;
}

const ExtrasModal: React.FC<ExtraProps> = ({onClose, sendRequest}) => {

    const [ip, setIp] = useState<string>("");
    const [port, setPort] = useState<string>("");

    useEffect(()=>{
        const handleKeyDown = (e: KeyboardEvent) => {
            if (e.key === "Escape") {
                setIp("");
                setPort("");
                onClose();
            }
        };
        window.addEventListener("keydown", handleKeyDown);
        return () => window.removeEventListener("keydown", handleKeyDown);
    }, [onClose])

    const sendMessageRequest = () => {
        if(!ip || !port) return;
        const portNum = Number(port);
        if (!portNum) return;
        sendRequest(ip, portNum);
        
    }

    return (
        <div className="extramodal-mainframe">
            <div className="extramodal-modal">
                <input 
                    className="extramodal-input"
                    type="text"
                    placeholder='IP'
                    value={ip}
                    onChange={(event)=>{setIp(event.target.value)}} 
                />
                <input 
                    className="extramodal-input"
                    type="text"
                    placeholder='Port'
                    value={port}
                    onChange={(event)=>{setPort(event.target.value)}}
                />
                <button className="extramodal-button" onClick={sendMessageRequest}>Send Request</button>
            </div>
        </div>
    )
}

export default ExtrasModal