import { useState } from 'react';
import { getAuth, GoogleAuthProvider, signInWithPopup, createUserWithEmailAndPassword } from 'firebase/auth';
import { useNavigate, Link } from 'react-router-dom'; // ✅ Use Link instead of <a>
import './Signup.css';
import { getDatabase, ref, set } from 'firebase/database'; // ✅ Import Firebase Database

const Signup = () => {
    const auth = getAuth();
    const navigate = useNavigate();
    const [authing, setAuthing] = useState(false);
    const [email, setEmail] = useState('');
    const [password, setPassword] = useState('');
    const [confirmPassword, setConfirmPassword] = useState('');
    const [error, setError] = useState('');

    // Google Signup
    const signUpWithGoogle = async () => {
        setAuthing(true);
        try {
            const response = await signInWithPopup(auth, new GoogleAuthProvider());
            console.log("Google Signup Success:", response.user.uid);
            navigate('/');
        } catch (error) {
            console.error("Google Signup Error:", error.message);
            setError(error.message);
        } finally {
            setAuthing(false);
        }
    };

    // Email Signup
    const signUpWithEmail = async () => {
        if (password !== confirmPassword) {
            setError('Passwords do not match');
            return;
        }

        setAuthing(true);
        setError('');

        try {
            const response = await createUserWithEmailAndPassword(auth, email, password);
            console.log("Signup Success:", response.user.uid);

            // ✅ Store user data in Firebase Database
            const db = getDatabase();
            set(ref(db, 'users/' + response.user.uid), {
                email: email,
                createdAt: new Date().toISOString(),
                role: 'user' // You can add more fields
            });

            navigate('/');
        } catch (error) {
            console.error("Signup Error:", error.message);
            setError(error.message);
        } finally {
            setAuthing(false);
        }
    };

    return (
        <div className='signup'>
            <div className='content'>
                <div className='left-side'>
                    <img
                        src="/signupbg.jpg"
                        alt="debug"
                        style={{ width: '100%', height: '100%', objectFit: 'cover' }}
                        onError={(e) => {
                            console.error("Image failed to load:", e);
                            e.target.src = '/fallback.jpg'; // ✅ Provide fallback image
                        }}
                    />
                    <div className='left-content'>
                        <img src="/logo.png" alt="logo" className='logo' style={{ width: '20%', height: '20%' }} />
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
                                required
                            />
                            <input
                                type='password'
                                placeholder='Password'
                                value={password}
                                onChange={(e) => setPassword(e.target.value)}
                                required
                            />
                            <input
                                type='password'
                                placeholder='Re-Enter Password'
                                value={confirmPassword}
                                onChange={(e) => setConfirmPassword(e.target.value)}
                                required
                            />
                            {error && <div className='error'>{error}</div>}
                            <button onClick={signUpWithEmail} disabled={authing}>Sign Up</button>
                            <button onClick={signUpWithGoogle} disabled={authing} className='google-btn'>
                                Sign Up with Google
                            </button>
                            <p>Already have an account? <Link to='/login'>Log in</Link></p>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    );
};

export default Signup;
