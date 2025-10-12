import React from 'react'
import './MessageEntry.css'

type MessageEntryProps = {
    userId: string
    text: string
    fromCurrentUser: boolean
    isGrouped: boolean
}

const MessageEntry: React.FC<MessageEntryProps> = ({userId, text, isGrouped}) => {
  return (
    <div 
        className="message-entry"
    >
        <div className={`message-userId ${isGrouped ? '' : 'grouped'}`}>{isGrouped ? "" : userId}</div> 
        <div className="message-text">{text}</div>
    </div>
  )
}

export default MessageEntry