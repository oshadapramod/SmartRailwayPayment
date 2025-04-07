import { StrictMode } from 'react';
import { createRoot } from 'react-dom/client';
import { BrowserRouter as Router, Routes, Route, Navigate } from 'react-router-dom';
import Homepage from './pages/Homepage';
import Login from './pages/Login';
import Signup from './pages/Signup';
import './index.css';
import App from './App';

createRoot(document.getElementById('root')).render(
  <StrictMode>
        <App />
  </StrictMode>
);
