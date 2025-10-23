import {useState, useRef} from 'react';
import WebSocketApi from './api/WebSocketApi';
import VIMPacket from './api/VIMPacketApi';
import MessageList from './components/messages/MessageList';
import Input from './components/input/Input';
import Users from './components/users/Users';
import Extras from './components/extras/Extras';
import './App.css'

function App() {
  const [wsApi, setWsApi] = useState<WebSocketApi | null>(null);

  const handleConnect = () => {
    if(!wsApi) {
      const api = new WebSocketApi("ws://localhost:8080");
      setWsApi(api);
      api.connect();
    }
  }

  const sendMessage = () => {
    if(wsApi) {
      const packet : Uint8Array | undefined = VIMPacket.createMsgPacket(1,1,1, "Hello from react");
      if(packet) wsApi.send(packet);
    } 
  }

  const onSendMessage = (msg : string) => {
    console.log(msg);
  }

  const usersList = [
  { name: 'Aidan', hasNewMessage: true },
  { name: 'Emily', hasNewMessage: false },
  { name: 'Carlos', hasNewMessage: true },
]

  return (
    <>
      <div className='app-mainframe'>
        <div className='app-sidebar'>
          <div className='app-users'>
            <Users users={usersList}/>
          </div>
          <div className='app-extras'>
            <button onClick={handleConnect}>Connect</button>
            <button onClick={sendMessage}>Message</button>
            <Extras />
          </div>
        </div>
        <div className='app-maindisplay'>
          {/* TODO: fix the calculations of the username widths have it be relative to the length and that determines the sapce for the text*/}
          <div className='app-messages'>
            <MessageList currentUserId='userA'/>
          </div>
          {/* TODO: Fix calculation of the usernames widths like above */}
          <div className='app-input'>
            <Input username={"userA"} onSendMessage={onSendMessage} />
          </div>
        </div>
      </div>
    </>
  )
}

export default App
