import { useEffect, useState } from 'react';
import { ref, get, update, remove } from 'firebase/database';
import { db } from '../firebase';
import './CardIssue.css';

const CardIssue = () => {
    const [applications, setApplications] = useState([]);
    const [selectedApplication, setSelectedApplication] = useState(null);
    const [rfidUid, setRfidUid] = useState('');

    // Fetch pending applications from Firebase
    useEffect(() => {
        const fetchApplications = async () => {
            const applicationsRef = ref(db, 'rfidApplications');
            const snapshot = await get(applicationsRef);
            if (snapshot.exists()) {
                const data = snapshot.val();
                const pendingApplications = Object.keys(data)
                    .filter(key => data[key].status === 'pending')
                    .map(key => ({ id: key, ...data[key] }));
                setApplications(pendingApplications);
            }
        };

        fetchApplications();
    }, []);

    // Handle RFID Card Approval
    const handleApprove = async () => {
        if (!rfidUid.trim()) {
            alert('RFID UID is required for approval.');
            return;
        }

        if (selectedApplication) {
            const applicationRef = ref(db, `rfidApplications/${selectedApplication.id}`);
            await update(applicationRef, { rfidUid, status: 'approved' });
            alert('Card issued successfully!');
            setApplications(applications.filter(app => app.id !== selectedApplication.id));
            setSelectedApplication(null);
            setRfidUid('');
        }
    };

    // Handle RFID Card Rejection
    const handleReject = async () => {
        if (selectedApplication) {
            const applicationRef = ref(db, `rfidApplications/${selectedApplication.id}`);
            await remove(applicationRef);
            alert('Application rejected.');
            setApplications(applications.filter(app => app.id !== selectedApplication.id));
            setSelectedApplication(null);
            setRfidUid('');
        }
    };

    return (
        <div className="card-issue-page">
            <h2>RFID Card Issuance</h2>
            <div className="applications-list">
                <h3>Pending Applications</h3>
                {applications.length === 0 ? (
                    <p>No pending applications</p>
                ) : (
                    applications.map(app => (
                        <div
                            key={app.id}
                            className={`application-item ${selectedApplication?.id === app.id ? 'selected' : ''}`}
                            onClick={() => setSelectedApplication(app)}
                        >
                            <p><strong>Name:</strong> {app.name}</p>
                            <p><strong>NIC:</strong> {app.nic}</p>
                        </div>
                    ))
                )}
            </div>

            {selectedApplication && (
                <div className="application-details">
                    <h3>Applicant Details</h3>
                    <p><strong>Name:</strong> {selectedApplication.name}</p>
                    <p><strong>Address:</strong> {selectedApplication.address}</p>
                    <p><strong>NIC:</strong> {selectedApplication.nic}</p>
                    <p><strong>Gender:</strong> {selectedApplication.gender}</p>
                    <p><strong>Phone:</strong> {selectedApplication.phone}</p>

                    <label>Enter RFID UID:</label>
                    <input
                        type="text"
                        value={rfidUid}
                        onChange={(e) => setRfidUid(e.target.value)}
                        placeholder="Enter RFID UID"
                    />

                    <div className="buttons">
                        <button onClick={handleApprove} disabled={!rfidUid.trim()}>Approve</button>
                        <button onClick={handleReject}>Reject</button>
                    </div>
                </div>
            )}
        </div>
    );
};

export default CardIssue;
