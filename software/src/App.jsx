import { BrowserRouter as Router, Routes, Route, Navigate } from 'react-router-dom';
import Homepage from './pages/Homepage';
import CardIssue from './pages/CardIssue';
import LandingPage from './pages/LandingPage';
import AuthPage from './pages/AuthPage';

function App() {
    return (
        <Router>
            <Routes>
                <Route path="/" element={<LandingPage />} />
                <Route path="/AuthPage" element={<AuthPage />} />
                <Route path="/login" element={<Navigate to="/AuthPage" state={{ mode: 'login-mode' }} />} />
                <Route path="/signup" element={<Navigate to="/AuthPage" state={{ mode: 'signup-mode' }} />} />
                <Route path="/Homepage" element={<Homepage />} />
                <Route path="/CardIssue" element={<CardIssue />} />
                {/* Add other routes here */}
            </Routes>
        </Router>
    );
}

export default App;