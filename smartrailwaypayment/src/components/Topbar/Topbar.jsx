import { useNavigate } from 'react-router-dom';
import './TopBar.css';

const TopBar = () => {
    const navigate = useNavigate();

    return (
        <div className='topbar'>
            <div className="logo-container">
                <img src="/public/logo.png" alt="Railway Logo" className="logo" />
                <div className='title'>Smart Railway Payment</div>
            </div>
            <div className='nav-buttons'>
                <button onClick={() => navigate('/card-for-child')}>Card for Child</button>
                <button onClick={() => navigate('/inquiries')}>Inquiries</button>
                <button onClick={() => navigate('/reset-password')}>Reset Password</button>
                <button onClick={() => navigate('/about-us')}>About Us</button>
                <button onClick={() => navigate('/logout')}>Logout</button>
            </div>
        </div>
    );
}

export default TopBar;