import React, { useRef } from 'react'
import './Input.css'

type InputProps = {
  onSendMessage: (message: string) => void
  username: string
}

const Input: React.FC<InputProps> = ({ onSendMessage, username }) => {
  const inputRef = useRef<HTMLDivElement>(null)

  const handleInput = () => {
    // Get text content when needed
    const text = inputRef.current?.textContent || ''
    // Optionally store or send it
    // setText(text) <-- don't do this, or caret jumps
  }

  const handleWrapperClick = () => {
    inputRef.current?.focus()
  }

  return (
    <div className="input-wrapper" onClick={handleWrapperClick}>
      <div className="input-username"contentEditable="false">{username}</div>
      <div
        ref={inputRef}
        contentEditable="plaintext-only"
        className="input-display"
        onInput={handleInput}
        onKeyDown={(e) => {
          if(e.key === "Enter") {
            e.preventDefault();
            
            if (inputRef.current) {
              const text = inputRef.current.textContent.trim();
              onSendMessage(text);
              inputRef.current.textContent = "";
            }
          }
        }}
        suppressContentEditableWarning
      >
      </div>
    </div>
  )
}

export default Input
