import serial
import serial.tools.list_ports
import csv
import time
import os
from datetime import datetime

# Constants
HOURLY_RATE = 200  # RWF per hour
CSV_FILE = 'plates_log.csv'
LOW_BALANCE_THRESHOLD = 200  # RWF

# Auto-detect Arduino Serial Port
def detect_arduino_port():
    ports = list(serial.tools.list_ports.comports())
    for port in ports:
        if "ttyACM" in port.device:
            return port.device
    return None

# Calculate parking duration and fee
def calculate_fee(entry_time):
    entry_dt = datetime.strptime(entry_time, '%Y-%m-%d %H:%M:%S')
    current_dt = datetime.now()
    
    # Calculate duration in hours (rounded up)
    duration_seconds = (current_dt - entry_dt).total_seconds()
    duration_hours = max(1, int((duration_seconds + 3599) / 3600))  # Round up to nearest hour
    
    fee = duration_hours * HOURLY_RATE
    return duration_hours, fee, current_dt.strftime('%Y-%m-%d %H:%M:%S')

# Find plate in CSV and get entry time
def find_plate_entry(plate_number):
    if not os.path.exists(CSV_FILE):
        print(f"Error: {CSV_FILE} not found")
        return None
    
    with open(CSV_FILE, 'r', newline='') as file:
        reader = csv.reader(file)
        header = next(reader)  # Skip header
        
        # Find the most recent unpaid entry for this plate
        for row in reversed(list(reader)):
            if row[0] == plate_number and row[4] == '0':  # Unpaid entry
                return row[1]  # Return entry time
    
    return None

# Update payment record in CSV
def update_payment_record(plate_number, exit_time, amount):
    temp_file = CSV_FILE + '.tmp'
    updated = False
    
    with open(CSV_FILE, 'r', newline='') as infile, open(temp_file, 'w', newline='') as outfile:
        reader = csv.reader(infile)
        writer = csv.writer(outfile)
        
        header = next(reader)
        writer.writerow(header)
        
        for row in reader:
            if row[0] == plate_number and row[4] == '0' and not updated:
                # Update exit time, amount, and payment status
                row[2] = exit_time  # Exit Time
                row[3] = str(amount)  # Due Amount
                row[4] = '1'  # Set payment status to paid
                updated = True
            writer.writerow(row)
    
    # Replace the original file with the updated file
    os.replace(temp_file, CSV_FILE)
    return updated

def main():
    arduino_port = detect_arduino_port()
    if not arduino_port:
        print("Arduino not detected. Please connect Arduino and try again.")
        return
    
    print(f"Connecting to Arduino on {arduino_port}...")
    arduino = serial.Serial(arduino_port, 9600, timeout=1)
    time.sleep(2)  # Wait for connection to establish
    
    print("Payment Processing System Ready. Waiting for RFID card...")
    
    try:
        while True:
            if arduino.in_waiting > 0:
                line = arduino.readline().decode('utf-8').strip()
                print(f"Received: {line}")
                
                if line.startswith("CARD_DATA:"):
                    # Parse card data
                    data = line[10:].split(',')
                    if len(data) >= 3:
                        plate_number = data[0]
                        card_balance = int(data[1])
                        card_uid = data[2]
                        
                        print(f"Plate: {plate_number}, Balance: {card_balance} RWF, UID: {card_uid}")
                        
                        # Check for low balance
                        if card_balance < LOW_BALANCE_THRESHOLD:
                            print(f"WARNING: Low balance detected ({card_balance} RWF)")
                            arduino.write(f"LOW_BALANCE:{card_balance}\n".encode())
                            
                        # Find entry time for this plate
                        entry_time = find_plate_entry(plate_number)
                        
                        if entry_time:
                            # Calculate fee
                            duration, fee, exit_time = calculate_fee(entry_time)
                            
                            print(f"Entry Time: {entry_time}")
                            print(f"Exit Time: {exit_time}")
                            print(f"Parking Duration: {duration} hours")
                            print(f"Amount Due: {fee} RWF")
                            
                            if card_balance >= fee:
                                # Send deduction command to Arduino
                                arduino.write(f"DEDUCT:{fee}\n".encode())
                                
                                # Wait for confirmation
                                response = arduino.readline().decode('utf-8').strip()
                                
                                if response == "PAYMENT:SUCCESS":
                                    # Update payment record in CSV
                                    if update_payment_record(plate_number, exit_time, fee):
                                        print(f"Payment of {fee} RWF successful!")
                                        new_balance = card_balance - fee
                                        print(f"New balance: {new_balance} RWF")
                                        
                                        # Check if balance is low after payment
                                        if new_balance < LOW_BALANCE_THRESHOLD:
                                            print(f"WARNING: Balance after payment is low ({new_balance} RWF)")
                                            arduino.write(f"LOW_BALANCE:{new_balance}\n".encode())
                                    else:
                                        print("Error updating payment record.")
                                else:
                                    print(f"Payment failed: {response}")
                            else:
                                print("Insufficient funds on card.")
                                arduino.write("CANCEL\n".encode())
                        else:
                            print(f"No unpaid parking record found for plate {plate_number}")
                            arduino.write("CANCEL\n".encode())
                
                elif line == "PAYMENT:SUCCESS":
                    print("Payment processed successfully.")
                
                elif line.startswith("PAYMENT:"):
                    print(f"Payment status: {line[8:]}")
            
            time.sleep(0.1)
    
    except KeyboardInterrupt:
        print("\nExiting payment system...")
    finally:
        if arduino.is_open:
            arduino.close()
            print("Serial connection closed.")

if __name__ == "__main__":
    main()
