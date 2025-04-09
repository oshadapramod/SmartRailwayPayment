import { useState, useEffect } from 'react';
import { useNavigate, useLocation } from 'react-router-dom';
import { getAuth, signInWithEmailAndPassword, createUserWithEmailAndPassword, GoogleAuthProvider, signInWithPopup } from 'firebase/auth';
import { getDatabase, ref, set } from 'firebase/database';
import './AuthPage.css';
import railgoLogo from '../assets/railgo_logo.png';

const AuthPage = () => {
    const location = useLocation();

    const [isLogin, setIsLogin] = useState(true);
    const [isAnimating, setIsAnimating] = useState(false);
    const auth = getAuth();
    const navigate = useNavigate();

    // Login state
    const [loginEmail, setLoginEmail] = useState('');
    const [loginPassword, setLoginPassword] = useState('');
    const [loginError, setLoginError] = useState('');
    const [loginLoading, setLoginLoading] = useState(false);

    // Signup state
    const [signupEmail, setSignupEmail] = useState('');
    const [signupPassword, setSignupPassword] = useState('');
    const [confirmPassword, setConfirmPassword] = useState('');
    const [signupError, setSignupError] = useState('');
    const [signupLoading, setSignupLoading] = useState(false);
    const [passwordStrength, setPasswordStrength] = useState('');

    useEffect(() => {
        if (location.state?.mode === 'signup-mode') {
            setIsLogin(false);
        } else {
            setIsLogin(true);
        }
    }, [location.state]);

    // Toggle between login and signup
    const toggleAuthMode = () => {
        setIsAnimating(true);

        // Wait for animation to complete before changing the state
        setTimeout(() => {
            setIsLogin(!isLogin);
            setIsAnimating(false);
        }, 600); // Match this with CSS transition duration
    };

    // Login handlers
    const handleLogin = async () => {
        if (!loginEmail || !loginPassword) {
            setLoginError('Please enter both email and password');
            return;
        }

        setLoginLoading(true);
        setLoginError('');

        try {
            const response = await signInWithEmailAndPassword(auth, loginEmail, loginPassword);
            console.log('Login successful:', response.user.uid);
            navigate('/');
        } catch (error) {
            console.error('Login error:', error);
            setLoginError(error.message.includes('auth/')
                ? 'Invalid email or password. Please try again.'
                : error.message);
        } finally {
            setLoginLoading(false);
        }
    };

    const handleGoogleLogin = async () => {
        const loadingState = isLogin ? setLoginLoading : setSignupLoading;
        const errorState = isLogin ? setLoginError : setSignupError;

        loadingState(true);
        errorState('');

        try {
            const response = await signInWithPopup(auth, new GoogleAuthProvider());
            console.log('Google login successful:', response.user.uid);

            // If signing up, store user data
            if (!isLogin) {
                const db = getDatabase();
                set(ref(db, 'users/' + response.user.uid), {
                    email: response.user.email,
                    createdAt: new Date().toISOString(),
                    role: 'user',
                    authProvider: 'google'
                });
            }

            navigate('/');
        } catch (error) {
            console.error('Google login error:', error);
            errorState('Failed to login with Google. Please try again.');
        } finally {
            loadingState(false);
        }
    };

    // Password strength checker
    const checkPasswordStrength = (password) => {
        if (!password) {
            setPasswordStrength('');
            return;
        }

        const hasLower = /[a-z]/.test(password);
        const hasUpper = /[A-Z]/.test(password);
        const hasNumber = /[0-9]/.test(password);
        const hasSpecial = /[!@#$%^&*(),.?":{}|<>]/.test(password);
        const isLongEnough = password.length >= 8;

        const score = [hasLower, hasUpper, hasNumber, hasSpecial, isLongEnough]
            .filter(Boolean).length;

        if (score <= 2) return 'weak';
        if (score <= 4) return 'medium';
        return 'strong';
    };

    const handlePasswordChange = (e) => {
        const newPassword = e.target.value;
        setSignupPassword(newPassword);
        setPasswordStrength(checkPasswordStrength(newPassword));
    };

    // Email Signup
    const signUpWithEmail = async () => {
        // Validation
        if (!signupEmail || !signupPassword || !confirmPassword) {
            setSignupError('Please fill in all fields');
            return;
        }

        if (signupPassword !== confirmPassword) {
            setSignupError('Passwords do not match');
            return;
        }

        if (passwordStrength === 'weak') {
            setSignupError('Please choose a stronger password');
            return;
        }

        setSignupLoading(true);
        setSignupError('');

        try {
            const response = await createUserWithEmailAndPassword(auth, signupEmail, signupPassword);
            console.log("Signup Success:", response.user.uid);

            // Store user data in Firebase Database
            const db = getDatabase();
            set(ref(db, 'users/' + response.user.uid), {
                email: signupEmail,
                createdAt: new Date().toISOString(),
                role: 'user',
                authProvider: 'email'
            });

            navigate('/');
        } catch (error) {
            console.error("Signup Error:", error.message);

            // Handle specific Firebase errors with user-friendly messages
            if (error.code === 'auth/email-already-in-use') {
                setSignupError('This email is already registered. Please log in instead.');
            } else if (error.code === 'auth/invalid-email') {
                setSignupError('Please enter a valid email address.');
            } else if (error.code === 'auth/weak-password') {
                setSignupError('Your password is too weak. Please choose a stronger password.');
            } else {
                setSignupError(error.message);
            }
        } finally {
            setSignupLoading(false);
        }
    };

    return (
        <div className='auth-page'>
            <div className={`auth-container ${isLogin ? 'login-mode' : 'signup-mode'} ${isAnimating ? 'animating' : ''}`}>
                {/* Left side (Login form in login mode, content in signup mode) */}
                <div className='auth-left'>
                    {/* Login Form */}
                    <div className={`auth-form-container ${isLogin ? 'active' : ''}`}>
                        <div className='auth-header'>
                            <h3>Login</h3>
                        </div>
                        <div className='auth-form'>
                            <div className="auth-input-group">
                                <label htmlFor="login-email">Email</label>
                                <input
                                    id="login-email"
                                    type='email'
                                    placeholder='Enter your email'
                                    value={loginEmail}
                                    onChange={(e) => setLoginEmail(e.target.value)}
                                />
                            </div>
                            <div className="auth-input-group">
                                <label htmlFor="login-password">Password</label>
                                <input
                                    id="login-password"
                                    type='password'
                                    placeholder='Enter your password'
                                    value={loginPassword}
                                    onChange={(e) => setLoginPassword(e.target.value)}
                                />
                            </div>
                            {loginError && <div className='error'>{loginError}</div>}
                            <button
                                onClick={handleLogin}
                                disabled={loginLoading}
                                className="auth-primary-btn"
                            >
                                {loginLoading ? 'Signing in...' : 'Sign In'}
                            </button>

                            <div className="auth-divider">
                                <span>OR</span>
                            </div>

                            <button
                                onClick={handleGoogleLogin}
                                disabled={loginLoading}
                                className="auth-google-btn"
                            >
                                <img src="https://cdn.cdnlogo.com/logos/g/35/google-icon.svg" alt="Google" />
                                Sign in with Google
                            </button>
                        </div>
                        <div className='auth-footer'>
                            <p>Don't have an account? <button className="auth-link" onClick={toggleAuthMode}>Sign Up</button></p>
                        </div>
                    </div>

                    {/* Signup Content */}
                    <div className={`auth-content ${!isLogin ? 'active' : ''}`}>
                        <div className="auth-logo-container">
                            <img src={railgoLogo} alt="RailGo Logo" className="auth-logo-icon" />
                            <h2 className="auth-logo-text">Rail Go</h2>
                        </div>
                        <h1>Start Your Journey</h1>
                        <p>Create an account to experience the future of public transport payments with RailGo.</p>
                    </div>
                </div>

                {/* Right side (Content in login mode, signup form in signup mode) */}
                <div className='auth-right'>
                    {/* Login Content */}
                    <div className={`auth-content ${isLogin ? 'active' : ''}`}>
                        <div className="auth-logo-container">
                            <img src={railgoLogo} alt="RailGo Logo" className="auth-logo-icon" />
                            <h2 className="auth-logo-text">Rail Go</h2>
                        </div>
                        <h1>Welcome Back</h1>
                        <p>Log in to continue your seamless journey with RailGo - the future of public transport payments.</p>
                    </div>

                    {/* Signup Form */}
                    <div className={`auth-form-container ${!isLogin ? 'active' : ''}`}>
                        <div className='auth-header'>
                            <h3>Create Account</h3>
                        </div>
                        <div className='auth-form'>
                            <div className="auth-input-group">
                                <label htmlFor="signup-email">Email</label>
                                <input
                                    id="signup-email"
                                    type='email'
                                    placeholder='Enter your email'
                                    value={signupEmail}
                                    onChange={(e) => setSignupEmail(e.target.value)}
                                    required
                                />
                            </div>

                            <div className="auth-input-group">
                                <label htmlFor="signup-password">Password</label>
                                <input
                                    id="signup-password"
                                    type='password'
                                    placeholder='Create a password'
                                    value={signupPassword}
                                    onChange={handlePasswordChange}
                                    required
                                />
                            </div>

                            {passwordStrength && (
                                <div className="auth-password-strength">
                                    <div className="auth-strength-meter">
                                        <div className={`auth-strength-meter-fill auth-${passwordStrength}`}></div>
                                    </div>
                                    <p>Password strength: {passwordStrength.charAt(0).toUpperCase() + passwordStrength.slice(1)}</p>
                                </div>
                            )}

                            <div className="auth-input-group">
                                <label htmlFor="confirm-password">Confirm Password</label>
                                <input
                                    id="confirm-password"
                                    type='password'
                                    placeholder='Re-enter your password'
                                    value={confirmPassword}
                                    onChange={(e) => setConfirmPassword(e.target.value)}
                                    required
                                />
                            </div>

                            {signupError && <div className='error'>{signupError}</div>}

                            <button
                                onClick={signUpWithEmail}
                                disabled={signupLoading}
                                className="auth-primary-btn"
                            >
                                {signupLoading ? 'Creating account...' : 'Create Account'}
                            </button>

                            <div className="auth-divider">
                                <span>OR</span>
                            </div>

                            <button
                                onClick={handleGoogleLogin}
                                disabled={signupLoading}
                                className="auth-google-btn"
                            >
                                <img src="https://cdn.cdnlogo.com/logos/g/35/google-icon.svg" alt="Google" />
                                Sign up with Google
                            </button>
                        </div>
                        <div className='auth-footer'>
                            <p>Already have an account? <button className="auth-link" onClick={toggleAuthMode}>Log in</button></p>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    );
};

export default AuthPage;