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
                console.log('Unauthorized: Redirecting to authpage');
                navigate('/AuthPage');
            }
        });

        return () => unsubscribe();
    }, [auth, navigate]);

    if (loading) return <p>Loading...</p>;

    return <>{children}</>;
};

export default AuthRoute;
