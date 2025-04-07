import React, { useState } from 'react';
import './LandingPage.css';

import Lottie from 'lottie-react';
import train_img from '../assets/train_img.json';
import rfidicon from '../assets/rfidicon.json';
import railgo_logo from '../assets/railgo_logo.png';

const LandingPage = () => {
    const [activeTeamMember, setActiveTeamMember] = useState(null);

    // Sample team data - replace with actual team information
    const teamMembers = [
        {
            id: 1,
            name: "Alex Jayawardena",
            role: "Lead Developer",
            bio: "Alex specializes in system architecture and backend development, with expertise in Firebase and real-time database management. He leads the technical direction of our platform.",
            imgSrc: "/api/placeholder/200/200",
            contacts: {
                email: "alex@example.com",
                linkedin: "linkedin.com/in/alex-jayawardena",
                github: "github.com/alexjay"
            }
        },
        {
            id: 2,
            name: "Priya Perera",
            role: "Hardware Integration Specialist",
            bio: "Priya focuses on RFID integration and hardware-software interfacing, ensuring our physical components work seamlessly with the digital system.",
            imgSrc: "/api/placeholder/200/200",
            contacts: {
                email: "priya@example.com",
                linkedin: "linkedin.com/in/priya-perera",
                github: "github.com/priyaperera"
            }
        },
        {
            id: 3,
            name: "Malik Fernando",
            role: "Frontend Developer",
            bio: "Malik is responsible for creating intuitive user interfaces and ensuring a smooth user experience across all touchpoints of our application.",
            imgSrc: "/api/placeholder/200/200",
            contacts: {
                email: "malik@example.com",
                linkedin: "linkedin.com/in/malik-fernando",
                github: "github.com/malikf"
            }
        },
        {
            id: 4,
            name: "Tharushi Silva",
            role: "Database & Security Expert",
            bio: "Tharushi manages our database architecture and ensures that all user data is securely stored and accessed according to best practices.",
            imgSrc: "/api/placeholder/200/200",
            contacts: {
                email: "tharushi@example.com",
                linkedin: "linkedin.com/in/tharushi-silva",
                github: "github.com/tharushis"
            }
        }
    ];

    const openTeamMemberModal = (id) => {
        setActiveTeamMember(id);
    };

    const closeTeamMemberModal = () => {
        setActiveTeamMember(null);
    };

    const advantages = [
        {
            title: "Cost Efficiency",
            description: "Eliminates the need for paper tickets and reduces operational costs for transport authorities.",
            icon: "📉"
        },
        {
            title: "Convenience",
            description: "No need to queue for tickets - just tap your card and go.",
            icon: "⚡"
        },
        {
            title: "Transparent Billing",
            description: "Clear monthly statements with journey details and pricing breakdown.",
            icon: "📊"
        },
        {
            title: "Environmentally Friendly",
            description: "Reduces paper waste from traditional ticketing systems.",
            icon: "🌱"
        },
        {
            title: "Data-Driven Improvements",
            description: "Collects usage data to help optimize train schedules and capacity.",
            icon: "📱"
        },
        {
            title: "Enhanced Security",
            description: "Personal identification helps improve security within the transport network.",
            icon: "🔒"
        }
    ];

    return (
        <div className="landing-page">
            {/* Navigation Bar */}
            <nav class="navbar">
                <div class="logo-container">
                    <img src={railgo_logo} alt="RailGo Logo" className="logo-icon" />
                    <h1 class="logo-text">Train Pay</h1>
                </div>

                <div class="nav-links">
                    <a href="#home">Home</a>
                    <a href="#about">About</a>
                    <a href="#features">Features</a>
                    <a href="#advantages">Advantages</a>
                    <a href="#team">Team</a>
                </div>

                <div class="auth-buttons">
                    <button class="login-btn">Log In</button>
                    <button class="signup-btn">Sign Up</button>
                </div>
            </nav>

            {/* Hero Section */}
            <section id="home" className="hero-section">
                <div className="hero-content">
                    <h1>Modern Public Transport Payment System</h1>
                    <p>Revolutionizing train travel in Sri Lanka with RFID technology and automated payments</p>
                    <div className="hero-cta">
                        <button className="primary-btn">Get Started</button>
                        <button className="secondary-btn" onClick={() => document.getElementById('about').scrollIntoView({ behavior: 'smooth' })}>Learn More</button>
                    </div>
                </div>
                <div className="hero-image">
                    <Lottie animationData={train_img} loop={true} />
                </div>

            </section>


            {/* About Section */}
            <section id="about" className="about-section">
                <div className="container">
                    <h2>About Our Project</h2>
                    <div className="about-content">
                        <div className="about-image">
                            <Lottie animationData={rfidicon} loop={true} alt="Project overview" />
                        </div>
                        <div className="about-text">
                            <p>
                                The Embedded Public Transport Payment System is an innovative university project
                                designed to modernize Sri Lanka's railway ticketing infrastructure. Our solution
                                replaces traditional paper tickets with a seamless RFID-based system that logs
                                journeys and processes payments automatically.
                            </p>
                            <p>
                                Our goal is to reduce queues, eliminate paper waste, and improve the overall
                                commuter experience while providing transport authorities with valuable data
                                and streamlined operations.
                            </p>
                        </div>
                    </div>
                </div>
            </section>

            {/* Features Section */}
            <section id="features" className="features-section">
                <div className="container">
                    <h2>Key Features</h2>
                    <div className="features-grid">
                        <div className="feature-card">
                            <div className="feature-icon">🔖</div>
                            <h3>RFID Card Scanning</h3>
                            <p>Scan your RFID card for instant identification and a personalized welcome message.</p>
                        </div>
                        <div className="feature-card">
                            <div className="feature-icon">🚉</div>
                            <h3>Destination Selection</h3>
                            <p>Easily input your destination using an intuitive interface with station codes.</p>
                        </div>
                        <div className="feature-card">
                            <div className="feature-icon">⭐</div>
                            <h3>Class Selection</h3>
                            <p>Choose from First, Second, or Third Class based on your comfort preferences.</p>
                        </div>
                        <div className="feature-card">
                            <div className="feature-icon">✅</div>
                            <h3>Journey Authorization</h3>
                            <p>Receive instant journey confirmation and logging of travel details.</p>
                        </div>
                        <div className="feature-card">
                            <div className="feature-icon">💳</div>
                            <h3>Automated Billing</h3>
                            <p>Monthly e-bills with a detailed breakdown of journeys and fares.</p>
                        </div>
                        <div className="feature-card">
                            <div className="feature-icon">📈</div>
                            <h3>Data Analytics</h3>
                            <p>Travel patterns and usage data to improve transport services.</p>
                        </div>
                    </div>
                </div>
            </section>

            {/* Advantages Section */}
            <section id="advantages" className="advantages-section">
                <div className="container">
                    <h2>System Advantages</h2>
                    <div className="advantages-grid">
                        {advantages.map((advantage, index) => (
                            <div className="advantage-card" key={index}>
                                <div className="advantage-icon">{advantage.icon}</div>
                                <h3>{advantage.title}</h3>
                                <p>{advantage.description}</p>
                            </div>
                        ))}
                    </div>
                </div>
            </section>

            {/* Team Section */}
            <section id="team" className="team-section">
                <div className="container">
                    <h2>Meet the Developer Team</h2>
                    <div className="team-grid">
                        {teamMembers.map((member) => (
                            <div className="team-member-card" key={member.id} onClick={() => openTeamMemberModal(member.id)}>
                                <div className="member-image">
                                    <img src={member.imgSrc} alt={member.name} />
                                </div>
                                <h3>{member.name}</h3>
                                <p>{member.role}</p>
                            </div>
                        ))}
                    </div>
                </div>
            </section>

            {/* Team Member Modal */}
            {activeTeamMember && (
                <div className="modal-overlay" onClick={closeTeamMemberModal}>
                    <div className="modal-content" onClick={(e) => e.stopPropagation()}>
                        {teamMembers.filter(member => member.id === activeTeamMember).map(member => (
                            <div className="team-member-details" key={member.id}>
                                <span className="close-modal" onClick={closeTeamMemberModal}>&times;</span>
                                <div className="member-header">
                                    <img src={member.imgSrc} alt={member.name} />
                                    <div>
                                        <h3>{member.name}</h3>
                                        <p className="member-role">{member.role}</p>
                                    </div>
                                </div>
                                <div className="member-bio">
                                    <h4>Biography</h4>
                                    <p>{member.bio}</p>
                                </div>
                                <div className="member-contacts">
                                    <h4>Contact</h4>
                                    <p><strong>Email:</strong> {member.contacts.email}</p>
                                    <p><strong>LinkedIn:</strong> {member.contacts.linkedin}</p>
                                    <p><strong>GitHub:</strong> {member.contacts.github}</p>
                                </div>
                            </div>
                        ))}
                    </div>
                </div>
            )}

            {/* Footer */}
            <footer className="footer">
                <div className="container">
                    <div className="footer-content">
                        <div className="footer-logo">
                            <h3>TrainPay</h3>
                            <p>Modern Public Transport Payment System</p>
                        </div>
                        <div className="footer-links">
                            <div className="footer-column">
                                <h4>Navigate</h4>
                                <a href="#home">Home</a>
                                <a href="#about">About</a>
                                <a href="#features">Features</a>
                                <a href="#advantages">Advantages</a>
                                <a href="#team">Team</a>
                            </div>
                            <div className="footer-column">
                                <h4>Contact</h4>
                                <p>University of Example</p>
                                <p>Department of Computer Science</p>
                                <p>example@university.edu</p>
                            </div>
                        </div>
                    </div>
                    <div className="copyright">
                        <p>&copy; {new Date().getFullYear()} TrainPay System. All rights reserved.</p>
                    </div>
                </div>
            </footer>
        </div>
    );
};

export default LandingPage;