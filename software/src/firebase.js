import { initializeApp } from "firebase/app";
import { getDatabase } from "firebase/database";
import { getAuth } from "firebase/auth";

const firebaseConfig = {
  apiKey: "AIzaSyBSgYXQX9IalJO9h6kZXNv6vzI0Lu7cEn8",
  authDomain: "smartrailwaypayment.firebaseapp.com",
  projectId: "smartrailwaypayment",
  storageBucket: "smartrailwaypayment.firebasestorage.app",
  messagingSenderId: "147630263730",
  appId: "1:147630263730:web:1e189325fc328950ace8ab",
  databaseURL: "https://smartrailwaypayment-default-rtdb.firebaseio.com"
};

// Initialize Firebase
const app = initializeApp(firebaseConfig);

// Initialize services and export them
export const db = getDatabase(app);
export const auth = getAuth(app);