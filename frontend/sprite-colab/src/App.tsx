import {useState} from 'react';
import WebSocketApi from './api/WebSocketApi';
import MessageList from './components/messages/MessageList';
import Input from './components/Input';
import './App.css'


function App() {
  const [wsApi, setWsApi] = useState<WebSocketApi | null>(null);

  const handleConnect = () => {
    if(!wsApi) {
      const api = new WebSocketApi("ws://localhost:8080");
      setWsApi(api);

      api.connect((msg: string) => {
        console.log("Received from server:", msg);
      });

    }
  }

  const sendMessage = () => {
    if(wsApi) {
      wsApi.send("Hello, this is the react server and I would like to connect");
    } 
  }

  const onSendMessage = (msg : string) => {
    console.log(msg);
  }

  return (
    <>
      <div className='app-mainframe'>
        <div className='app-sidebar'>
          <div className='app-user'>Aidan</div>
          <div className='app-user'>Aidan</div>
          
        </div>
        <div className='app-maindisplay'>
          <div className='app-messages'>
            <MessageList currentUserId='userA'/>
          </div>
          <div className='app-input'>
            <Input onSendMessage={onSendMessage} />
          </div>
        </div>
      </div>
    </>
  )
}

export default App
