#include <SPI.h>
#include <MFRC522.h>
#include <Arduino.h>
#include <LiquidCrystal.h> //LCD
#include <MQTT.h>
#include <WiFiNINA.h>

#define SS_PIN 1
#define RST_PIN 40
 
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);  //LCD

MFRC522 rfid(SS_PIN, RST_PIN); // Instance of the class

MQTTClient client;
WiFiClient wificlient;

//const char ssid[] = "Crunch LAB";
//const char pass[] = "90xV@FsT";
//const char hostname[14] = "192.168.1.109";//MQTT Broker IP

//const char ssid[] = "utbm_visiteurs";
//const char pass[] = "";
//const char hostname[14] = "172.17.15.227";//MQTT Broker IP

const char ssid[] = "NUMERICABLE-93BC";
const char pass[] = "C731FDEDBE";
const char hostname[14] = "192.168.0.11";//MQTT Broker IP

/**
 * Helper routine to dump a byte array as hex values to Serial. 
 */
void printHex(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    lcd.print(buffer[i], HEX); //LCD
    lcd.print(" ");
    Serial.print(buffer[i], HEX);
  }
}

/**
 * Helper routine to dump a byte array as dec values to Serial.
 */
void printDec(byte *buffer, byte bufferSize) {
  for (byte i = 0; i < bufferSize; i++) {
    Serial.print(buffer[i] < 0x10 ? " 0" : " ");
    lcd.print(buffer[i], DEC); //LCD
    lcd.print(" ");
    Serial.print(buffer[i], DEC);
  }
}

String RFIDToDec(byte *buffer, byte bufferSize){
  String UIDDec;
  for (byte i = 0; i < bufferSize; i++) {
    UIDDec = UIDDec + (buffer[i]);
    //Serial.print(UIDDec);
  }

  return UIDDec;
}

unsigned long lastMillis = 0;

void connect() {

  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.print("connecting...");
  while (!client.connect("RFIDREV2", "try", "try")) {
    Serial.print("+");
    delay(1000);
  }

  Serial.println("\nconnected!");

  client.subscribe("/RFID/Access");
  client.subscribe("/RFID/Nom");
  client.subscribe("/RFID/Prenom");
  // client.unsubscribe("/hello");
}

String DataReceived;

void messageReceived(String &topic, String &payload) {
  Serial.println("\nincoming: " + topic + " - " + payload);
  
  if(topic == "/RFID/Access")
  {
    if(payload == "1"){
      lcd.print("ACCES ACCEPTE   ");
    }else{
      lcd.print("ACCES REFUSE    ");
    }
    DataReceived = payload;
    Serial.print(DataReceived);
  }
  //lcd.setCursor(0, 1); //LCD
  //lcd.print("                ");

  if(topic == "/RFID/Prenom"){
    lcd.setCursor(0, 1); //LCD
    lcd.print("                ");
    lcd.setCursor(0, 1); //LCD
    lcd.print(payload[0]);
    lcd.setCursor(1, 1); //LCD
    lcd.print(".");
  }

  if(topic == "/RFID/Nom" ){
    lcd.setCursor(2, 1); //LCD
    lcd.print(payload);
  }

}

MFRC522::MIFARE_Key key; 

// Init array that will store new NUID 
byte nuidPICC[4];

void setup() { 
  Serial.begin(9600);
  lcd.begin(16, 2); //LCD
  SPI.begin(); // Init SPI bus
  rfid.PCD_Init(); // Init MFRC522 

  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }

  Serial.println(F("This code scan the MIFARE Classsic NUID."));
  Serial.print(F("Using the following key:"));
  printHex(key.keyByte, MFRC522::MF_KEY_SIZE);

  Serial.begin(9600);
  Serial.println("\nWELCOME");
  lcd.setCursor(0, 0); //LCD
  lcd.print("    WELCOME!    ");
  WiFi.begin(ssid, pass);
  Serial.println();

  //dht.setup(12,DHT::DHT11); // data pin 12

  
  client.begin(hostname, 1883, wificlient);
  client.onMessage(messageReceived);

  connect();
}
 
String RetDec;

void loop() {

  client.loop();

  if (!client.connected()) {
    connect();
  }

  lcd.setCursor(0, 0); //LCD
  // Reset the loop if no new card present on the sensor/reader. This saves the entire process when idle.
  if ( ! rfid.PICC_IsNewCardPresent())
    return;

  // Verify if the NUID has been readed
  if ( ! rfid.PICC_ReadCardSerial())
    return;

  Serial.print(F("PICC type: "));
  MFRC522::PICC_Type piccType = rfid.PICC_GetType(rfid.uid.sak);
  //Serial.println(rfid.PICC_GetTypeName(piccType));

  // Check is the PICC of Classic MIFARE type
  if (piccType != MFRC522::PICC_TYPE_MIFARE_MINI &&  
    piccType != MFRC522::PICC_TYPE_MIFARE_1K &&
    piccType != MFRC522::PICC_TYPE_MIFARE_4K) {
    Serial.println(F("Your tag is not of type MIFARE Classic."));
    return;
  }

  if (rfid.uid.uidByte[0] != nuidPICC[0] || 
    rfid.uid.uidByte[1] != nuidPICC[1] || 
    rfid.uid.uidByte[2] != nuidPICC[2] || 
    rfid.uid.uidByte[3] != nuidPICC[3] ) {
    Serial.println(F("A new card has been detected."));

    // Store NUID into nuidPICC array
    for (byte i = 0; i < 4; i++) {
      nuidPICC[i] = rfid.uid.uidByte[i];
    }
   
    Serial.println(F("The NUID tag is:"));
    Serial.print(F("In hex: "));
    printHex(rfid.uid.uidByte, rfid.uid.size);
    lcd.setCursor(0, 1);
    Serial.print(F("In dec: "));
    printDec(rfid.uid.uidByte, rfid.uid.size);

    RetDec = RFIDToDec(rfid.uid.uidByte, rfid.uid.size);

    lcd.print("     ");
    Serial.println();
  }
  else Serial.println(F("Card read previously."));

  // Halt PICC
  rfid.PICC_HaltA();

  // Stop encryption on PCD
  rfid.PCD_StopCrypto1();

  Serial.print(RetDec);
  if (millis() - lastMillis > 1000) {
    lastMillis = millis();

    client.publish("/RFID", RetDec);
}

}