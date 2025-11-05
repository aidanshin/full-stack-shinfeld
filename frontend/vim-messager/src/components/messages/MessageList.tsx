import React, { useRef, useEffect, useState } from 'react'
// import messages from '../../data/messages.json'
import VIMPacket from '../../api/VIMPacketApi'
import MessageEntry from './MessageEntry'
import './MessageList.css'

// type Message = {
//   id: number
//   userId: string
//   message: string
// }

type MessageListProps = {
  currentUserId: number
  messages: VIMPacket[]
}

// const typedMessages = [
//   ...messages as Message[],
//   ...messages as Message[], 
//   ...messages as Message[],
// ]

const MessageList: React.FC<MessageListProps> = ({ currentUserId, messages }) => {
  const containerRef = useRef<HTMLDivElement>(null)

  useEffect(() => {
    if (containerRef.current) {
      containerRef.current.scrollTop = containerRef.current.scrollHeight
    }
  }, [messages]) 

  return (
    <div
      ref={containerRef}
      className='message-list'
    >
      {messages.map((msg, index) => {
        const previousMsg = messages[index - 1]
        const isSameSender = previousMsg && previousMsg.userId === msg.userId

        return (
          <MessageEntry
            key={`${msg.msgId}-${index}`}
            userId={msg.userId}
            text={msg.msg_data}
            fromCurrentUser={msg.userId === currentUserId}
            isGrouped={isSameSender}
          />
        )
      })}
    </div>
  )
}

export default MessageList
