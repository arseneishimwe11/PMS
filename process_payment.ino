#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN 9           // RFID Reset pin
#define SS_PIN  10          // RFID SDA (SS) pin

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;

// Data blocks to read/write
#define PLATE_BLOCK     2
#define BALANCE_BLOCK   4

void setup() {
  Serial.begin(9600);
  while (!Serial);      // Wait for serial port to connect
  SPI.begin();          // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522 card
  
  // Prepare the key (used both for reading and writing)
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;  // Default key
  }
  
  Serial.println("RFID Payment System Ready");
}

void loop() {
  // Look for new cards
  if (!mfrc522.PICC_IsNewCardPresent()) {
    delay(100);
    return;
  }

  // Select one of the cards
  if (!mfrc522.PICC_ReadCardSerial()) {
    delay(100);
    return;
  }

  Serial.println("Card detected!");
  
  // Get UID as a string
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) {
      uid += "0";
    }
    uid += String(mfrc522.uid.uidByte[i], HEX);
    if (i < mfrc522.uid.size - 1) {
      uid += " ";
    }
  }
  uid.toUpperCase();

  // Read plate number and balance
  String plateNumber = readBlock(PLATE_BLOCK);
  int balance = readBlockAsInt(BALANCE_BLOCK);
  
  // Send data to Python script
  Serial.print("CARD_DATA:");
  Serial.print(plateNumber);
  Serial.print(",");
  Serial.print(balance);
  Serial.print(",");
  Serial.println(uid);

  // Wait for Python to calculate amount and send back
  while (!Serial.available()) {
    delay(100);
    // Timeout can be added here
  }
  
  String response = Serial.readStringUntil('\n');
  response.trim();
  
  if (response.startsWith("DEDUCT:")) {
    int amountToDeduct = response.substring(7).toInt();
    
    if (amountToDeduct <= balance) {
      // Sufficient balance, process payment
      int newBalance = balance - amountToDeduct;
      
      // Write new balance to card
      if (writeBlockAsInt(BALANCE_BLOCK, newBalance)) {
        Serial.println("PAYMENT:SUCCESS");
      } else {
        Serial.println("PAYMENT:WRITE_ERROR");
      }
    } else {
      // Insufficient balance
      Serial.println("PAYMENT:INSUFFICIENT_FUNDS");
    }
  } else if (response.startsWith("LOW_BALANCE:")) {
    // Handle low balance notification
    int lowBalance = response.substring(12).toInt();
    Serial.print("LOW BALANCE ALERT: ");
    Serial.print(lowBalance);
    Serial.println(" RWF - Please top up your card soon!");
  } else if (response == "CANCEL") {
    Serial.println("PAYMENT:CANCELLED");
  }
  
  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
  
  delay(1000);  // Small delay before next read
}

String readBlock(byte blockNumber) {
  byte buffer[18];
  byte size = sizeof(buffer);
  
  // Authenticate using key A
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNumber, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.println("Authentication failed");
    return "";
  }
  
  // Read block
  status = mfrc522.MIFARE_Read(blockNumber, buffer, &size);
  if (status != MFRC522::STATUS_OK) {
    Serial.println("Reading failed");
    return "";
  }
  
  // Convert to string and trim spaces
  String result = "";
  for (byte i = 0; i < 16; i++) {
    if (buffer[i] != 0 && buffer[i] != 32) {  // Skip nulls and spaces
      result += (char)buffer[i];
    }
  }
  return result;
}

int readBlockAsInt(byte blockNumber) {
  String strValue = readBlock(blockNumber);
  return strValue.toInt();
}

bool writeBlockAsInt(byte blockNumber, int value) {
  // Convert int to string
  String strValue = String(value);
  // Pad with spaces to fill 16 bytes
  while (strValue.length() < 16) {
    strValue += " ";
  }
  
  byte buffer[16];
  for (byte i = 0; i < 16; i++) {
    buffer[i] = (byte)strValue.charAt(i);
  }
  
  // Authenticate using key A
  status = mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, blockNumber, &key, &(mfrc522.uid));
  if (status != MFRC522::STATUS_OK) {
    Serial.println("Authentication failed");
    return false;
  }
  
  // Write block
  status = mfrc522.MIFARE_Write(blockNumber, buffer, 16);
  if (status != MFRC522::STATUS_OK) {
    Serial.println("Writing failed");
    return false;
  }
  
  return true;
}