# **Embedded Public Transport Payment System**

This project is an **Embedded System for Public Transportation (Train) Payments**, developed as part of a university project. It combines hardware and software to create a modern, efficient railway ticketing solution for Sri Lanka, allowing passengers to use RFID cards for seamless journey logging and automated monthly billing.

---

## **Features**

### **1. RFID Card Scanning**
- Users scan their RFID card at the station machine.
- The system extracts the user's NIC number or ID.
- Validates the user in real-time against the database.
- Displays a personalized welcome message: *"Welcome [Name]"*.

### **2. Destination Selection**
- Users input their destination using a keypad.
- Displays a list of destinations and their corresponding codes.
- Confirms the chosen destination.

### **3. Class Selection**
- Offers train class options: Third Class, Second Class, First Class.
- Confirms the selected class on the display.

### **4. Journey Authorization**
- Logs journey details (timestamp, entry station, destination, and class) to the database.
- Provides journey authorization with a confirmation message.

### **5. Automated Billing**
- Aggregates journey data at the end of the month.
- Calculates fares based on travel distance and class.
- Sends a detailed e-bill to the user's registered email.

---

## **System Components**

### **Hardware**
1. **Microcontroller**: ESP32 or Arduino Uno with Wi-Fi capabilities.
2. **RFID Card Scanner**: Reads user data from RFID Cards.
3. **Display Modules**: A display module for displaying instructions and confirmations.
4. **Keypad**: For entering destinations and selecting classes.
5. **Connectivity Module**: ESP32 or ESP8266 for real-time server communication.
6. **Buzzer/LED**: Provides feedback for valid/invalid inputs.
7. **Power Supply**: Reliable source for powering the system.

### **Software**
1. **Embedded Code**: Manages RFID Card scanning, input processing, and communication with the server.
2. **Backend Server**: Handles user validation, journey logging, and bill calculation.
3. **Database**: Stores user information, journey details, and billing data.
4. **Email Automation**: Sends monthly bills using email APIs.
---

## **System Workflow**

### **1. User Interaction**
1. Scan RFID Card → Display Welcome Message.
2. Input Destination → Confirm Selection.
3. Select Train Class → Confirm Selection.
4. Authorize Journey → Display Success Message.

### **2. Backend Process**
1. Validate User → Log Journey Details.
2. Aggregate Monthly Data → Generate Bill.
3. Send E-Bill via Email.

---

## **Repository Structure**

```plaintext
.
├── hardware/       # Circuit diagrams and hardware schematics
├── firmware/       # Embedded code for ESP32/Arduino
├── backend/        # Backend server scripts and database models
├── frontend/       # User interface components (if applicable)
├── docs/           # Documentation, workflows, and system architecture
├── LICENSE         # License information
└── README.md       # Project overview and setup instructions
