import React, { useState, useEffect, useRef } from 'react'
import './Input.css'

type InputProps = {
  onSendMessage: (message: string) => void
}

const Input: React.FC<InputProps> = ({ onSendMessage }) => {
  const [input, setInput] = useState('')
  const inputRef = useRef<HTMLDivElement>(null)
  const [showCaret, setShowCaret] = useState(true)

  // Blink the caret
  useEffect(() => {
    const interval = setInterval(() => {
      setShowCaret((prev) => !prev)
    }, 500)
    return () => clearInterval(interval)
  }, [])

  // Handle Enter key
  const handleKeyDown = (e: React.KeyboardEvent<HTMLDivElement>) => {
    if (e.key === 'Enter') {
      e.preventDefault()
      if (input.trim()) {
        onSendMessage(input)
        setInput('')
      }
    }
  }

  // Focus the div on click
  useEffect(() => {
    inputRef.current?.focus()
  }, [])

  return (
    <div
      ref={inputRef}
      className="terminal-input"
      contentEditable
      suppressContentEditableWarning
      onInput={(e) => setInput(e.currentTarget.textContent || '')}
      onKeyDown={handleKeyDown}
      spellCheck={false}
    >
      {input}
      <span className={`caret ${showCaret ? 'visible' : ''}`}>|</span>
    </div>
  )
}

export default Input
