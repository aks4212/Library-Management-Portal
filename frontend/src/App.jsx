import { useState, useEffect } from 'react'

const getTodayDDMMYYYY = () => {
  const d = new Date();
  return parseInt(String(d.getDate()).padStart(2, '0') + String(d.getMonth() + 1).padStart(2, '0') + d.getFullYear());
};

function App() {
  const [token, setToken] = useState(null)
  const [user, setUser] = useState(null) // Basic user info
  const [profile, setProfile] = useState({ fine: 0, active_borrows: [], history: [] }) // Detailed user info
  
  const [books, setBooks] = useState([])
  const [students, setStudents] = useState([])
  const [currentDate, setCurrentDate] = useState(getTodayDDMMYYYY())
  
  const [usernameInput, setUsernameInput] = useState("")
  const [passwordInput, setPasswordInput] = useState("")

  const [newBook, setNewBook] = useState({ id: "", title: "", author: "", publisher: "", year: "", isbn: "" })
  const [newUser, setNewUser] = useState({ id: "", name: "", password: "" })

  // --- CORE FETCHING ---
  const handleLogin = async (e) => {
    e.preventDefault()
    const res = await fetch('/api/login', { method: 'POST', body: JSON.stringify({ username: usernameInput, password: passwordInput }) })
    if (res.ok) {
      const data = await res.json()
      setToken(data.token); setUser({ name: data.name, role: data.role })
      fetchBooks()
      if (data.role === 'Student') fetchProfile(data.token)
      if (data.role === 'Librarian') fetchStudents(data.token)
    } else alert("Login failed!")
  }

  const fetchBooks = async () => setBooks(await (await fetch('/api/books')).json())
  
  const fetchProfile = async (currentToken = token) => {
    const res = await fetch('/api/me', { method: 'POST', body: JSON.stringify({ token: currentToken }) })
    if(res.ok) setProfile(await res.json())
  }

  const fetchStudents = async (currentToken = token) => {
    const res = await fetch('/api/get_users', { method: 'POST', body: JSON.stringify({ token: currentToken }) })
    if(res.ok) setStudents(await res.json())
  }

  // --- STUDENT ACTIONS ---
  const handleBorrow = async (bookId) => {
    const res = await fetch('/api/borrow', { method: 'POST', body: JSON.stringify({ token, book_id: bookId, current_date: currentDate }) })
    alert(await res.text()); fetchBooks(); fetchProfile();
  }

  const handleReturn = async (bookId) => {
    const res = await fetch('/api/return', { method: 'POST', body: JSON.stringify({ token, book_id: bookId, current_date: currentDate }) })
    if (res.ok) alert((await res.json()).message)
    else alert(await res.text())
    fetchBooks(); fetchProfile();
  }

  const handlePayFine = async () => {
    await fetch('/api/payfine', { method: 'POST', body: JSON.stringify({ token }) })
    alert("Fines cleared!"); fetchProfile();
  }

  // --- LIBRARIAN ACTIONS ---
  const handleAddBook = async (e) => {
    e.preventDefault()
    await fetch('/api/add_book', { method: 'POST', body: JSON.stringify({ token, id: parseInt(newBook.id), ...newBook, year: parseInt(newBook.year) }) })
    fetchBooks(); setNewBook({ id: "", title: "", author: "", publisher: "", year: "", isbn: "" })
  }
  const handleDeleteBook = async (bookId) => {
    await fetch('/api/delete_book', { method: 'POST', body: JSON.stringify({ token, book_id: bookId }) }); fetchBooks()
  }
  const handleAddUser = async (e) => {
    e.preventDefault()
    await fetch('/api/add_user', { method: 'POST', body: JSON.stringify({ token, id: parseInt(newUser.id), name: newUser.name, password: newUser.password }) })
    fetchStudents(); setNewUser({ id: "", name: "", password: "" })
  }
  const handleDeleteUser = async (targetId) => {
    const res = await fetch('/api/delete_user', { method: 'POST', body: JSON.stringify({ token, target_id: targetId }) })
    if(!res.ok) alert(await res.text())
    fetchStudents()
  }

  // --- RENDERING ---
  const renderStudentDashboard = () => (
    <div>
      <div style={{ backgroundColor: '#f4f4f9', padding: '1rem', borderRadius: '8px', marginBottom: '1rem' }}>
        <p style={{ color: 'red', fontWeight: 'bold' }}>Outstanding Fine: ₹{profile.fine}</p>
        {profile.fine > 0 && <button onClick={handlePayFine}>Pay Fines</button>}
        <div style={{ marginTop: '1rem', padding: '1rem', border: '1px dashed #999' }}>
          <label><strong>Simulated Date (DDMMYYYY): </strong></label>
          <input type="number" value={currentDate} onChange={(e) => setCurrentDate(parseInt(e.target.value))} />
        </div>
      </div>

      <div style={{ display: 'flex', gap: '2rem' }}>
        {/* MY BOOKS */}
        <div style={{ flex: 1 }}>
          <h3>📖 My Active Borrowings</h3>
          {profile.active_borrows.length === 0 ? <p>No books borrowed.</p> : 
            <ul style={{ listStyle: 'none', padding: 0 }}>
              {profile.active_borrows.map(b => (
                <li key={b.id} style={{ border: '1px solid #3498db', padding: '1rem', marginBottom: '1rem', borderRadius: '8px', backgroundColor: '#ebf5fb' }}>
                  <strong>{b.title}</strong> (ID: {b.id})<br/>
                  <small>Borrowed: {b.date}</small>
                  <button onClick={() => handleReturn(b.id)} style={{ display: 'block', marginTop: '10px', backgroundColor: '#e74c3c' }}>Return Book</button>
                </li>
              ))}
            </ul>
          }
        </div>

        {/* CATALOG */}
        <div style={{ flex: 1 }}>
          <h3>📚 Library Catalog</h3>
          <ul style={{ listStyle: 'none', padding: 0 }}>
            {books.map(book => (
              <li key={book.id} style={{ border: '1px solid #ddd', padding: '1rem', marginBottom: '1rem', borderRadius: '8px' }}>
                <strong>{book.title}</strong> by {book.author}<br/>
                <span style={{ color: book.status === 'Available' ? 'green' : 'red' }}>{book.status}</span>
                {book.status === 'Available' && <button onClick={() => handleBorrow(book.id)} style={{ marginLeft: '10px' }}>Borrow</button>}
              </li>
            ))}
          </ul>
        </div>
      </div>

      {/* HISTORY */}
      <h3 style={{ marginTop: '2rem' }}>🕒 Borrowing History</h3>
      <table style={{ width: '100%', textAlign: 'left', borderCollapse: 'collapse' }}>
        <thead><tr style={{ borderBottom: '2px solid #ccc' }}><th>Book</th><th>Borrowed</th><th>Returned</th></tr></thead>
        <tbody>
          {profile.history.map((h, i) => (
            <tr key={i} style={{ borderBottom: '1px solid #eee' }}>
              <td style={{ padding: '8px 0' }}>{h.title}</td><td>{h.borrow_date}</td><td>{h.return_date}</td>
            </tr>
          ))}
        </tbody>
      </table>
    </div>
  )

  const renderLibrarianDashboard = () => (
    <div>
      <div style={{ display: 'flex', gap: '2rem', marginBottom: '2rem' }}>
        {/* ADD BOOK */}
        <form onSubmit={handleAddBook} style={{ flex: 1, border: '1px solid #2c3e50', padding: '1rem', borderRadius: '8px' }}>
          <h3>➕ Add Book</h3>
          {['id', 'title', 'author', 'publisher', 'year', 'isbn'].map(f => <input key={f} type={f==='id'||f==='year'?'number':'text'} placeholder={f.toUpperCase()} value={newBook[f]} onChange={e => setNewBook({...newBook, [f]: e.target.value})} style={{ width: '90%', marginBottom: '8px' }} required />)}
          <button type="submit">Save</button>
        </form>

        {/* ADD USER */}
        <form onSubmit={handleAddUser} style={{ flex: 1, border: '1px solid #27ae60', padding: '1rem', borderRadius: '8px' }}>
          <h3>👤 Add Student</h3>
          {['id', 'name', 'password'].map(f => <input key={f} type={f==='id'?'number':'text'} placeholder={f.toUpperCase()} value={newUser[f]} onChange={e => setNewUser({...newUser, [f]: e.target.value})} style={{ width: '90%', marginBottom: '8px' }} required />)}
          <button type="submit">Register</button>
        </form>
      </div>

      {/* LISTS */}
      <div style={{ display: 'flex', gap: '2rem' }}>
        <div style={{ flex: 1 }}>
          <h3>Manage Books</h3>
          {books.map(b => <div key={b.id} style={{ borderBottom: '1px solid #ccc', padding: '8px 0' }}>{b.title} <button onClick={() => handleDeleteBook(b.id)}>Delete</button></div>)}
        </div>
        <div style={{ flex: 1 }}>
          <h3>Registered Students</h3>
          {students.map(s => <div key={s.id} style={{ borderBottom: '1px solid #ccc', padding: '8px 0' }}>ID: {s.id} | {s.name} | Active: {s.active} <button onClick={() => handleDeleteUser(s.id)}>Delete</button></div>)}
        </div>
      </div>
    </div>
  )

  return (
    <div style={{ padding: '2rem', fontFamily: 'system-ui, sans-serif', maxWidth: '900px', margin: '0 auto' }}>
      <h1>Library System</h1>
      {!token ? (
        <form onSubmit={handleLogin} style={{ border: '1px solid #ccc', padding: '1.5rem', borderRadius: '8px', maxWidth: '400px' }}>
          <h3>Login</h3>
          <input type="text" placeholder="Username" value={usernameInput} onChange={e => setUsernameInput(e.target.value)} style={{ width: '90%', padding: '0.5rem', marginBottom: '1rem' }} required />
          <input type="password" placeholder="Password" value={passwordInput} onChange={e => setPasswordInput(e.target.value)} style={{ width: '90%', padding: '0.5rem', marginBottom: '1rem' }} required />
          <button type="submit" style={{ width: '100%' }}>Login</button>
        </form>
      ) : (
        <div>
          <div style={{ display: 'flex', justifyContent: 'space-between', marginBottom: '1rem' }}>
            <h2 style={{ margin: 0 }}>Logged in as: {user.name}</h2>
            <button onClick={() => {setToken(null); setUsernameInput(""); setPasswordInput("")}}>Logout</button>
          </div>
          {user.role === 'Student' ? renderStudentDashboard() : renderLibrarianDashboard()}
        </div>
      )}
    </div>
  )
}

export default App