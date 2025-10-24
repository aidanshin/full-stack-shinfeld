import React, { useState } from 'react'
import './Extras.css'
import ExtrasModal from './ExtrasModal';

interface ExtrasProps {
  addUser: (ip:string, port: number) => void;
  connect: () => void;
}

const Extras: React.FC<ExtrasProps> = ({addUser, connect}) => {
  const [showModal, setShowModal] = useState<Boolean>(false);
  
  const closeModal = () => {
    setShowModal(false);
  }

  return (
    <div className='extras-mainframe'>
      <button className='extras-entry' onClick={()=>setShowModal(true)}>ADD USER</button>
      <button className='extras-entry' onClick={() => connect()}>Connect</button>
      {showModal && <ExtrasModal onClose={closeModal} sendRequest={addUser}/>}
      
      {/* <div className='extras-entry'>SETTINGS</div> */}
      {/* <div className='extras-entry'>MODE</div> */}
    </div>
  )
}

export default Extras