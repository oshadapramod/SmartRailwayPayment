import React, { useEffect, useState } from 'react';
import { getAuth, onAuthStateChanged } from 'firebase/auth';
import { useNavigate } from 'react-router-dom';

const AuthRoute = ({ children }) => {
    const auth = getAuth();
    const navigate = useNavigate();
    const [loading, setLoading] = useState(true);

    useEffect(() => {
        const unsubscribe = onAuthStateChanged(auth, (user) => {
            if (user) {
                setLoading(false);
            } else {
                console.log('Unauthorized: Redirecting to login');
                navigate('/login');
            }
        });

        return () => unsubscribe(); // ✅ Proper cleanup function
    }, [auth, navigate]);

    if (loading) return <p>Loading...</p>; // ✅ Display a proper loading indicator

    return <>{children}</>;
};

export default AuthRoute;
