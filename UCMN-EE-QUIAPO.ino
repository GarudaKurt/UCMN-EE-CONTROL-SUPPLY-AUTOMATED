#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Keypad.h>
#include <SPI.h>
#include <MFRC522.h>

#define SS_PIN 53
#define RST_PIN 5

MFRC522 rfid(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

LiquidCrystal_I2C lcd(0x27, 20, 4);  // 20x4 LCD

// Keypad setup (4x4)
const byte ROWS = 4; 
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1', '4', '7', '*'},
  {'2', '5', '8', '0'},                                                   
  {'3', '6', '9', '#'},
  {'A', 'B', 'C', 'D'}
};
byte rowPins[ROWS] = {22, 23, 24, 25}; 
byte colPins[COLS] = {26, 27, 28, 29}; 
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// Relay setup
int relayPins[8] = {30, 31, 32, 33, 34, 35, 36, 37};
bool relayStates[8] = {false, false, false, false, false, false, false, false};

// List of valid RFIDs
String validRFIDs[] = {
  "F3 5C 32 22", "34 99 2C F9", "34 67 2B F9", 
  "F4 73 4F DF", "44 7F 28 DB", "24 9F 60 DF", "54 B8 40 DB"
};

// Function prototypes
void scanRFIDPrompt();
bool validateRFID();
void showMenu();

void setup() {
  Serial.begin(115200);
  SPI.begin();
  rfid.PCD_Init();

  lcd.init(); 
  lcd.backlight();
  lcd.setCursor(3, 0);
  lcd.print("Light Control");
  lcd.setCursor(2, 1);
  lcd.print("System Loading...");
  delay(2000);
  
  scanRFIDPrompt();  // Start with RFID scanning

  for (int i = 0; i < 8; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], HIGH);  // Relays OFF (Active LOW)
  }
}

void loop() {
  if (!validateRFID()) {
    return;  // Keep scanning for a valid card
  }

  showMenu();

  while (true) {
    char key = keypad.getKey();
    if (!key) continue;

    lcd.clear();
   
    if (key == '1') {  // Individual Light Control
      lcd.setCursor(0, 0);
      lcd.print("Select Light [1-8]:");
      char lightKey;
      while (!(lightKey = keypad.getKey()));

      if (lightKey >= '1' && lightKey <= '8') {
        int relayIndex = lightKey - '1';
        relayStates[relayIndex] = true;
        digitalWrite(relayPins[relayIndex], LOW);
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Light ");
        lcd.print(lightKey);
        lcd.print(" ON");
      }
    }

    else if (key == '2') {  // Group Light Control (All ON)
      lcd.clear();
      lcd.setCursor(3, 1);
      lcd.print("Turning All ON...");
      for (int i = 0; i < 8; i++) {
        relayStates[i] = true;
        digitalWrite(relayPins[i], LOW);
      }
    }

    else if (key == '3') {  // Multiple Light Control
      lcd.setCursor(0, 0);
      lcd.print("Select Lights [1-8]:");
      lcd.setCursor(0, 1);
      lcd.print("Press # to end.");

      char lightKey;
      while (1) {
        lightKey = keypad.getKey();
        if (lightKey == '#') break; 

        if (lightKey >= '1' && lightKey <= '8') {
          int relayIndex = lightKey - '1';
          relayStates[relayIndex] = true;
          digitalWrite(relayPins[relayIndex], LOW);
          
          lcd.setCursor(0, 2);
          lcd.print("Light ");
          lcd.print(lightKey);
          lcd.print(" ON  ");
        }
      }
    }

   else if (key == '4') {  // Reset Light Control
    lcd.clear();
    lcd.setCursor(2,0);
    lcd.print("Switch OFF");
    lcd.setCursor(2, 1);
    lcd.print("[1] Individual");
    lcd.setCursor(2, 2);
    lcd.print("[2] Group");
    lcd.setCursor(2,3);
    lcd.print("[3] Exit");

    char resetKey;
    while (!(resetKey = keypad.getKey()));

    if (resetKey == '1') {  // Individual Reset
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Select Lights [1-8]:");
      lcd.setCursor(0, 1);
      lcd.print("Press # to end.");

      char lightKey;
      while (1) {
        lightKey = keypad.getKey();
        if (lightKey == '#') break; 

        if (lightKey >= '1' && lightKey <= '8') {
          int relayIndex = lightKey - '1';
          relayStates[relayIndex] = false;
          digitalWrite(relayPins[relayIndex], HIGH);
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Select Lights [1-8]:");
          lcd.setCursor(0, 1);
          lcd.print("Press # to end.");
          lcd.setCursor(0, 2);
          lcd.print("Light ");
          lcd.print(lightKey);
          lcd.print(" OFF  ");
        }
      }
    }
    else if (resetKey == '2') {  // Group Reset (Turn off all lights)
      lcd.clear();
      lcd.setCursor(2,1);
      lcd.print("Resetting Lights..");
      for (int i = 0; i < 8; i++) {
        relayStates[i] = false;
        digitalWrite(relayPins[i], HIGH);
      }
      delay(1000);
      lcd.clear();
      lcd.setCursor(4, 1);
      lcd.print("All Lights OFF");
    } else if(resetKey == '3') {
      showMenu();
    }
   }
   
    delay(2000);
    scanRFIDPrompt();  // After execution, go back to scanning
    return;
  }
}

void showMenu() {
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("[1] Individual");
  lcd.setCursor(2, 1);
  lcd.print("[2] Group");
  lcd.setCursor(2, 2);
  lcd.print("[3] Multiple");
  lcd.setCursor(2, 3);
  lcd.print("[4] Reset");
}


void scanRFIDPrompt() {
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Scan Your RFID");
}

bool validateRFID() {
  if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
    return false;
  }
  Serial.println("[DEBUG] pass here!");
  String scannedRFID = "";
  for (byte i = 0; i < rfid.uid.size; i++) {
    scannedRFID += String(rfid.uid.uidByte[i], HEX);
    if (i < rfid.uid.size - 1) scannedRFID += " ";
  }
  scannedRFID.toUpperCase();

  for (String validID : validRFIDs) {
    if (scannedRFID == validID) {
      Serial.println("RFID Accepted: " + scannedRFID);
      return true;
    }
  }

  Serial.println("RFID Denied: " + scannedRFID);
  lcd.clear();
  lcd.setCursor(3, 1);
  lcd.print("Access Denied!");
  delay(2000);
  scanRFIDPrompt();
  return false;
}
