# Parking Management System - PMS

## Overview
The Parking Management System is an intelligent, vision-based solution for automated vehicle entry management. This system uses computer vision and machine learning to detect license plates, validate them against predefined formats, and control access gates for authorized vehicles.

---

## Features

### 🚘 Automated License Plate Recognition (ALPR)
- Real-time license plate detection using YOLOv8  
- Optical Character Recognition (OCR) for plate text extraction  
- Robust validation of plate formats  

### 🧠 Smart Entry Management
- Proximity detection using ultrasonic sensors  
- Duplicate entry prevention with configurable cooldown periods  
- Automated gate control via Arduino integration  

### 🗂️ Data Logging & Management
- CSV-based logging of all vehicle entries  
- Timestamp recording for audit trails  
- Payment status tracking  

### ⚙️ Hardware Integration
- Arduino-based gate control mechanism  
- Webcam integration for video capture  
- Automatic hardware detection and configuration  

---

## System Architecture

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│   Webcam    │───▶│  Computer   │───▶│   Arduino   │
│  (Capture)  │    │ (Processing)│    │ (Gate Ctrl) │
└─────────────┘    └─────────────┘    └─────────────┘
                          │
                          ▼
                   ┌─────────────┐
                   │  Database   │
                   │  (CSV Log)  │
                   └─────────────┘
```

---

## Requirements

### 🔧 Hardware
- Computer with webcam  
- Arduino Uno or compatible board  
- Ultrasonic sensor (HC-SR04 recommended)  
- Servo motor for gate control  
- USB cable for Arduino connection  

### 💻 Software
- Python 3.8+  
- OpenCV  
- Ultralytics YOLOv8  
- PyTesseract  
- PySerial  
- Tesseract OCR engine  

---

## Installation

### 1. Clone the repository
```bash
git clone https://github.com/yourusername/parking-management-system.git
cd parking-management-system
```

### 2. Install dependencies
```bash
pip install -r requirements.txt
```

### 3. Install Tesseract OCR

**Ubuntu/Debian/Kali:**
```bash
sudo apt install tesseract-ocr
```

**Windows:**  
Download and install from [Tesseract GitHub](https://github.com/tesseract-ocr/tesseract)

---

## Arduino Setup
- Upload the provided Arduino sketch to your board  
- Connect the ultrasonic sensor and servo motor as per the wiring diagram  

---

## Usage

### Start the system
```bash
python car_entry.py
```

### System Operation
- The system detects a vehicle within 50cm  
- License plates are detected, validated, and logged  
- The gate opens automatically for valid plates  
- Press `q` to exit the application  

---

## Configuration

You can modify the following parameters in `car_entry.py`:

- `entry_cooldown`: Time in seconds before the same plate can re-enter (default: `300s`)  
- `model = YOLO('best.pt')`: Path to your YOLO model file  
- Arduino port detection can be customized in the `detect_arduino_port()` function  

---

## License Plate Format

The system is configured to detect license plates with the following format:

- **Prefix:** Contains `"RA"`  
- **Pattern:** `3 uppercase letters + 3 digits + 1 uppercase letter`  
- **Example:** `RAB123C`  

To modify this format, edit the validation logic in the code.

---

## Troubleshooting

**Arduino not detected:**
- Make sure you are using the correct port depending on your OS
- Use: 
  ```bash
  python3 -m serial.tools.list_ports -v
  ```
  to find the port on which the arduino is connected to. 

**Poor plate detection:**  
- Adjust lighting conditions and camera position  

**OCR issues:**  
- Fine-tune the image preprocessing parameters  

---

## Contributing
Contributions are welcome! Please feel free to submit a Pull Request.

---

## License
This project is licensed under the MIT License – see the [LICENSE](LICENSE) file for details.

---

## Acknowledgments
- Developed as part of the *Intelligent Robotics* course  
- Thanks to the [Ultralytics](https://github.com/ultralytics) team for YOLOv8  
- Thanks to the [Tesseract OCR](https://github.com/tesseract-ocr) team for the text recognition engine  
