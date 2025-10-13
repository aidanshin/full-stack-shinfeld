import React from 'react'
import './Users.css'

type User = {
  name: string
  hasNewMessage: boolean
}

type UsersProps = {
  users: User[]
}

const Users: React.FC<UsersProps> = ({users}) => {
  return (
    <div className='user-mainframe'>
      {users.map((user, index) => (
        <div key={index} className="user-entry">
          
          <div className='user-name'>{user.name}</div>
        {user.hasNewMessage && <div className='user-messageCount'>{5}</div>}
        </div>
      ))}
    </div>
  )
}

export default Users