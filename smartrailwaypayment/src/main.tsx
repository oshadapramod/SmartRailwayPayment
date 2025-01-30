import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import './index.css'
import App from './App.tsx'
import Login from './Login.tsx'
import Signup from './Signup.tsx'
import { BrowserRouter as Router, Routes, Route, Navigate } from 'react-router-dom'

// Import the functions you need from the SDKs you need
import { initializeApp } from "firebase/app";
// TODO: Add SDKs for Firebase products that you want to use
// https://firebase.google.com/docs/web/setup#available-libraries

// Your web app's Firebase configuration
const firebaseConfig = {
  apiKey: "AIzaSyBSgYXQX9IalJO9h6kZXNv6vzI0Lu7cEn8",
  authDomain: "smartrailwaypayment.firebaseapp.com",
  projectId: "smartrailwaypayment",
  storageBucket: "smartrailwaypayment.firebasestorage.app",
  messagingSenderId: "147630263730",
  appId: "1:147630263730:web:1e189325fc328950ace8ab"
};

// Initialize Firebase
const app = initializeApp(firebaseConfig);

createRoot(document.getElementById('root')!).render(
  <StrictMode>
    <Router>
      <Routes>
        <Route path="/" element={<App />} />
        <Route path="/login" element={<Login />} />
        <Route path="/signup" element={<Signup />} />
        <Route path="*" element={<Navigate to="/" />} />
      </Routes>  
    </Router>  
  </StrictMode>,
)
