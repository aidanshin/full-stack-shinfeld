import React from 'react'
import './Users.css'

type User = {
  id: number
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
          
          <div className='user-name'>{user.id}</div>
        {user.hasNewMessage && <div className='user-messageCount'>{"!"}</div>}
        </div>
      ))}
    </div>
  )
}

export default Users