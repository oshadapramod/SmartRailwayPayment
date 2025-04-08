import { useState } from 'react';
import { getAuth, signInWithEmailAndPassword } from 'firebase/auth';
import { useNavigate } from 'react-router-dom';
import './Login.css';

const Login = () => {
    const auth = getAuth();
    const navigate = useNavigate();
    const [email, setEmail] = useState('');
    const [password, setPassword] = useState('');
    const [error, setError] = useState('');

    const handleLogin = async () => {
        signInWithEmailAndPassword(auth, email, password)
            .then(response => {
                console.log(response.user.uid);
                navigate('/');
            })
            .catch(error => {
                console.log(error);
                setError(error.message);
            });
    };

    return (
        <div className='login'>
            <div className='content'>
                <div className='left-side'>
                    <img
                        src="/loginbg.jpg"
                        alt="debug"
                        style={{ width: '100%', height: '100%', objectFit: 'cover' }}
                        onError={(e) => console.error("Image failed to load:", e)}
                    />
                </div>
                <div className='right-side'>
                    <div className='form-container'>
                        <div className='header'>
                            <h3>LOGIN</h3>
                            <p>Welcome back! Please enter your information below to log in.</p>
                        </div>
                        <div className='form'>
                            <input
                                type='email'
                                placeholder='Email'
                                value={email}
                                onChange={(e) => setEmail(e.target.value)}
                            />
                            <input
                                type='password'
                                placeholder='Password'
                                value={password}
                                onChange={(e) => setPassword(e.target.value)}
                            />
                            {error && <div className='error'>{error}</div>}
                            <button onClick={handleLogin}>Log In</button>
                        </div>
                        <div className='footer'>
                            <p>Don't have an account? <a href='/signup'>Sign Up</a></p>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    );
}

export default Login;