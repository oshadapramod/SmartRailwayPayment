import { useState } from 'react';
import { ref, push } from 'firebase/database';  // Changed from firestore imports
import TopBar from '../components/Topbar/Topbar';
import './Homepage.css';
import { db } from '../firebase';

const Homepage = () => {
    const [name, setName] = useState('');
    const [address, setAddress] = useState('');
    const [nic, setNic] = useState('');
    const [gender, setGender] = useState('');
    const [phone, setPhone] = useState('');
    const [isSubmitting, setIsSubmitting] = useState(false);
    const [error, setError] = useState(null);
    
    const handleSubmit = async (e) => {
        e.preventDefault();
        
        // Form validation
        if (!name || !address || !nic || !gender || !phone) {
            setError('All fields are required');
            return;
        }
    
        // Validate NIC format (assuming Sri Lankan NIC)
        const nicRegex = /^([0-9]{9}[x|X|v|V]|[0-9]{12})$/;
        if (!nicRegex.test(nic)) {
            setError('Invalid NIC format');
            return;
        }
    
        // Validate phone number (assuming Sri Lankan phone number)
        const phoneRegex = /^(?:7|0|(?:\+94))[0-9]{9,10}$/;
        if (!phoneRegex.test(phone)) {
            setError('Invalid phone number format');
            return;
        }
    
        setIsSubmitting(true);
        setError(null);
    
        try {
            const rfidApplicationsRef = ref(db, 'rfidApplications');
            await push(rfidApplicationsRef, {
                name,
                address,
                nic,
                gender,
                phone,
                status: 'pending',
                createdAt: new Date().toISOString()
            });
            
            alert('Application submitted successfully!');
            setName('');
            setAddress('');
            setNic('');
            setGender('');
            setPhone('');
            
        } catch (err) {
            console.error('Submission error:', err);
            if (err.code === 'PERMISSION_DENIED') {
                setError('Access denied. Please contact administrator.');
            } else {
                setError('Failed to submit application: ' + err.message);
            }
        } finally {
            setIsSubmitting(false);
        }
    };

    return (
        <div className="homepage">
            <TopBar />
            <div className="content">
                <div className="form-container">
                    <div className="header">
                        <h3>Apply for RFID Card</h3>
                        <p>Your seamless journey starts here</p>
                    </div>
                    <form className="form" onSubmit={handleSubmit}>
                        <input
                            type="text"
                            placeholder="Name"
                            value={name}
                            onChange={(e) => setName(e.target.value)}
                            required
                        />
                        <input
                            type="text"
                            placeholder="Address"
                            value={address}
                            onChange={(e) => setAddress(e.target.value)}
                            required
                        />
                        <input
                            type="text"
                            placeholder="NIC Number"
                            value={nic}
                            onChange={(e) => setNic(e.target.value)}
                            required
                        />
                        <select
                            value={gender}
                            onChange={(e) => setGender(e.target.value)}
                            required
                        >
                            <option value="" disabled>Select Gender</option>
                            <option value="male">Male</option>
                            <option value="female">Female</option>
                        </select>
                        <input
                            type="tel"
                            placeholder="Phone Number"
                            value={phone}
                            onChange={(e) => setPhone(e.target.value)}
                            required
                        />
                        <button type="submit" disabled={isSubmitting}>
                            {isSubmitting ? 'Submitting...' : 'Submit'}
                        </button>
                        {error && <p className="error">{error}</p>}
                    </form>
                </div>
            </div>
        </div>
    );
};

export default Homepage;