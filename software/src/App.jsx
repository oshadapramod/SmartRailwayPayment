import { BrowserRouter as Router, Routes, Route } from 'react-router-dom';
import Homepage from './pages/Homepage';
import Signup from './pages/Signup';
import Login from './pages/Login';
import CardIssue from './pages/CardIssue';
import LandingPage from './pages/LandingPage';

function App() {
    return (
        <Router>
            <Routes>
                <Route path="/" element={<LandingPage />} />
                <Route path="/Homepage" element={<Homepage />} />
                <Route path="/signup" element={<Signup />} />
                <Route path="/login" element={<Login />} />
                <Route path="/CardIssue" element={<CardIssue />} />
                {/* Add other routes here */}
            </Routes>
        </Router>
    );
}

export default App;