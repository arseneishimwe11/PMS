#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN 9           // RFID Reset pin
#define SS_PIN 10          // RFID SDA (SS) pin

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance
MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;

// Data blocks to write
#define PLATE_BLOCK     2
#define BALANCE_BLOCK   4

// Your license plate number and initial balance
String plateNumber = "RAH972U";  // Replace with your actual plate number
int initialBalance = 20000;       // Initial balance in RWF

void setup() {
  Serial.begin(9600);
  while (!Serial);      // Wait for serial port to connect
  SPI.begin();          // Init SPI bus
  mfrc522.PCD_Init();   // Init MFRC522 card
  
  // Prepare the key (used for reading and writing)
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;  // Default key
  }
  
  Serial.println("RFID Card Writer Ready");
  Serial.println("Place your card on the reader...");
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

  // Show card UID
  Serial.print("Card UID: ");
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) {
      Serial.print(" 0");
      uid += "0";
    } else {
      Serial.print(" ");
    }
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    uid += String(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println();
  uid.toUpperCase();
  
  // Process any card
  Serial.println("Card detected! Processing...");
  Serial.print("Card UID: ");
  Serial.println(uid);
  
  // Write plate number
  if (writeBlock(PLATE_BLOCK, plateNumber)) {
    Serial.println("Plate number written successfully!");
  } else {
    Serial.println("Failed to write plate number!");
  }
    
  // Write balance
  if (writeBlockAsInt(BALANCE_BLOCK, initialBalance)) {
    Serial.println("Balance written successfully!");
  } else {
    Serial.println("Failed to write balance!");
  }
    
  // Read back to verify
  String readPlate = readBlock(PLATE_BLOCK);
  int readBalance = readBlockAsInt(BALANCE_BLOCK);
    
  Serial.println("Verification:");
  Serial.print("Plate Number: ");
  Serial.println(readPlate);
  Serial.print("Balance: ");
  Serial.println(readBalance);
    
  Serial.println("Card setup complete!");
  
  // Halt PICC
  mfrc522.PICC_HaltA();
  // Stop encryption on PCD
  mfrc522.PCD_StopCrypto1();
  
  delay(3000);  // Wait before next read
}

bool writeBlock(byte blockNumber, String data) {
  // Pad the data with spaces to fill 16 bytes
  while (data.length() < 16) {
    data += " ";
  }
  
  byte buffer[16];
  for (byte i = 0; i < 16; i++) {
    buffer[i] = (byte)data.charAt(i);
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

bool writeBlockAsInt(byte blockNumber, int value) {
  return writeBlock(blockNumber, String(value));
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