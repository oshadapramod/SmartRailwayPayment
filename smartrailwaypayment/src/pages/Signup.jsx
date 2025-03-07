import { useState } from 'react';
import { getAuth, GoogleAuthProvider, signInWithPopup, createUserWithEmailAndPassword } from 'firebase/auth';
import { useNavigate } from 'react-router-dom';
import './Signup.css';

const Signup = () => {
    const auth = getAuth();
    const navigate = useNavigate();
    const [authing, setAuthing] = useState(false);
    const [email, setEmail] = useState('');
    const [password, setPassword] = useState('');
    const [confirmPassword, setConfirmPassword] = useState('');
    const [error, setError] = useState('');

    const signUpWithGoogle = async () => {
        setAuthing(true);
        signInWithPopup(auth, new GoogleAuthProvider())
            .then(response => {
                console.log(response.user.uid);
                navigate('/');
            })
            .catch(error => {
                console.log(error);
                setAuthing(false);
            });
    };

    const signUpWithEmail = async () => {
        if (password !== confirmPassword) {
            setError('Passwords do not match');
            return;
        }

        setAuthing(true);
        setError('');

        createUserWithEmailAndPassword(auth, email, password)
            .then(response => {
                console.log(response.user.uid);
                navigate('/');
            })
            .catch(error => {
                console.log(error);
                setError(error.message);
                setAuthing(false);
            });
    };

    return (
        <div className='signup'>
            <div className='content'>
                <div className='left-side'>
                    <img
                        src="/signupbg.jpg"
                        alt="debug"
                        style={{ width: '100%', height: '100%', objectFit: 'cover' }}
                        onError={(e) => console.error("Image failed to load:", e)}
                    />
                    <div className='left-content'>
                <img src="/logo.png" alt="logo" className='logo'
                style={{ width: '20%', height: '20%' }} />
                <h1 className='title'>Smart Railway Payment</h1>
                <p className='subtitle'>Your seamless journey starts here</p>
            </div>
                </div>
                <div className='right-side'>
                    <div className='form-container'>
                        <div className='header'>
                            <h3>SIGN UP</h3>
                            <p>Welcome! Please enter your information below to begin.</p>
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
                            <input
                                type='password'
                                placeholder='Re-Enter Password'
                                value={confirmPassword}
                                onChange={(e) => setConfirmPassword(e.target.value)}
                            />
                            {error && <div className='error'>{error}</div>}
                            <button onClick={signUpWithEmail} disabled={authing}>Sign Up With Email and Password</button>
                            <div className='divider'>OR</div>
                            <button onClick={signUpWithGoogle} disabled={authing}>Sign Up With Google</button>
                        </div>
                        <div className='footer'>
                            <p>Already have an account? <a href='/login'>Log In</a></p>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    );
}

export default Signup;