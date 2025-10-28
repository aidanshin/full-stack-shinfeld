import React, { useState } from 'react'
import './Extras.css'
import ExtrasModal from './ExtrasModal';

interface ExtrasProps {
  addUser: (ip:string, port: number) => void;
  connect: (ip:string, port:number) => void;
}

const Extras: React.FC<ExtrasProps> = ({addUser, connect}) => {
  const [showUserModal, setShowUserModal] = useState<Boolean>(false);
  const [showConnectModal, setShowConnectModal] = useState<Boolean>(false);
  
  const closeUserModal = () => {
    setShowUserModal(false);
  }

  const closeConnectModal = () => {
    setShowConnectModal(false);
  }

  return (
    <div className='extras-mainframe'>
      <button className='extras-entry' onClick={()=>setShowUserModal(true)}>ADD USER</button>
      <button className='extras-entry' onClick={() => setShowConnectModal(true)}>Connect</button>
      {showUserModal && <ExtrasModal onClose={closeUserModal} sendRequest={addUser} buttonText="Send Request"/>}
      {showConnectModal && <ExtrasModal onClose={closeConnectModal} sendRequest={connect} buttonText="Connect"/>}
      
      {/* <div className='extras-entry'>SETTINGS</div> */}
      {/* <div className='extras-entry'>MODE</div> */}
    </div>
  )
}

export default Extras