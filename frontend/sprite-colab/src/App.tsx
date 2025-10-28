// TODO: Fix Sizes of Usernames

import {useState, useCallback} from 'react';
import WebSocketApi from './api/WebSocketApi';
import VIMPacket from './api/VIMPacketApi';
import MessageList from './components/messages/MessageList';
import Input from './components/input/Input';
import Users from './components/users/Users';
import Extras from './components/extras/Extras';
import './App.css'
type User = {
  id: number
  hasNewMessage: boolean
}

function App() {
  const [wsApi, setWsApi] = useState<WebSocketApi | null>(null);
  const [messages, setMessages] = useState<VIMPacket[]>([]);
  const [users, setUsers] = useState<User[]>([]);

  const handleNewPacket = useCallback((packet: VIMPacket) => {
    if (packet) {
      setMessages(prev => [...prev, packet]);
    }
  }, []);
  
  const handleNewUser = useCallback((user: User) => {
    if (user) {
      setUsers(prev => [...prev, user]);
    }
  }, []);

  const addMessage = (packet: VIMPacket) => {
    if(packet) {
      setMessages(prev => [...prev, packet]);
    }
  }
  const handleConnect = (ip: string, port: number) => {
    if(!wsApi) {
      const api = new WebSocketApi(`ws://${ip}:${port}`);
      setWsApi(api);
      api.connect(handleNewPacket, handleNewUser);
    }
  }

  const sendMessage = (msg: string) => {
    if(wsApi) {
      const packet : Uint8Array | undefined = VIMPacket.createMsgPacket(1,1, msg);
      if(packet) {
        wsApi.send(packet);
        //confirm sent 
        const forwardPacket = VIMPacket.decodePacket(packet);
        if (forwardPacket) addMessage(forwardPacket);
      }
    } 
  }

  const sendUserRequest = (ip: string, port: number) => {
    if (wsApi) {
      const packet : Uint8Array | undefined = VIMPacket.createUserPacket(ip, port);
      if (packet) {
        wsApi.send(packet);
        // add user request logic to the user tab 
      }
    }
  }

  return (
    <>
      <div className='app-mainframe'>
        <div className='app-sidebar'>
          <div className='app-users'>
            <Users users={users}/>
          </div>
          <div className='app-extras'>
            <Extras addUser={sendUserRequest} connect={handleConnect} />
          </div>
        </div>
        <div className='app-maindisplay'>
          <div className='app-messages'>
            <MessageList currentUserId={1} messages={messages}/>
          </div>
          <div className='app-input'>
            <Input username={"1"} onSendMessage={sendMessage} />
          </div>
        </div>
      </div>
    </>
  )
}

export default App
