import React, { useRef, useEffect } from 'react'
import messages from '../../data/messages.json'
import MessageEntry from './MessageEntry'
import './MessageList.css'

type Message = {
  id: number
  userId: string
  message: string
}

type MessageListProps = {
  currentUserId: string
}

const typedMessages = [
  ...messages as Message[],
  ...messages as Message[], 
  ...messages as Message[],
]

const MessageList: React.FC<MessageListProps> = ({ currentUserId }) => {
  const containerRef = useRef<HTMLDivElement>(null)
  
  useEffect(() => {
    if (containerRef.current) {
      containerRef.current.scrollTop = containerRef.current.scrollHeight
    }
  }, [typedMessages]) 

  return (
    <div
      ref={containerRef}
      className='message-list'
    >
      {typedMessages.map((msg, index) => {
        const previousMsg = typedMessages[index - 1]
        const isSameSender = previousMsg && previousMsg.userId === msg.userId

        return (
          <MessageEntry
            key={`${msg.id}-${index}`}
            userId={msg.userId}
            text={msg.message}
            fromCurrentUser={msg.userId === currentUserId}
            isGrouped={isSameSender}
          />
        )
      })}
    </div>
  )
}

export default MessageList
