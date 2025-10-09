import {useState} from 'react';
import WebSocketApi from './api/WebSocketApi';

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

  return (
    <>
      <div className='app-mainframe'>
        <button 
          className='app-button'
          onClick={handleConnect}  
        >
          Connect
        </button>
        <button 
          className='app-button'
          onClick={sendMessage}  
        >
          Send
        </button>
      </div>
    </>
  )
}

export default App
