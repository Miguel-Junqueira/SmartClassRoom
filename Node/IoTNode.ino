#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include "time.h"
#include <string.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <coap-simple.h>

#define SS_PIN 5
#define RST_PIN 0
const int ledPin = 15;
const int redPin = 2;
const char node_id[] = "T64";
struct tm timeinfo;
unsigned long readDelay = 5000; // 5 seconds
unsigned long lastReadTime = 0;
char lastSentUID[50];

MFRC522 mfrc522(SS_PIN, RST_PIN);   // Create MFRC522 instance.

const char* ssid = "";
const char* password = "";


//IPAddress CoAPServer_IP{192,168,8,153};
//IPAddress CoAPServer_IP{192,168,8,246};
IPAddress CoAPServer_IP{192,168,4,1};
IPAddress CoAP_DEBUG{192,168,4,10};
int CoAPServerPort = 5683;
const char* CoAPURL = "smartClassRoom";
const char* CoAPtimeURL = "getTime";


//CoAP
WiFiUDP udp;
Coap coap(udp,512); //512 = buffer size


const char* dayOfWeek(int dow) {
  switch (dow) {
    case 0:
      return "Sunday";
    case 1:
      return "Monday";
    case 2:
      return "Tuesday";
    case 3:
      return "Wednesday";
    case 4:
      return "Thursday";
    case 5:
      return "Friday";
    case 6:
      return "Saturday";
    default:
      return "Unknown";
  }
}

const char* monthName(int month) {
  switch (month) {
    case 0:
      return "January";
    case 1:
      return "February";
    case 2:
      return "March";
    case 3:
      return "April";
    case 4:
      return "May";
    case 5:
      return "June";
    case 6:
      return "July";
    case 7:
      return "August";
    case 8:
      return "September";
    case 9:
      return "October";
    case 10:
      return "November";
    case 11:
      return "December";
    default:
      return "Unknown";
  }
}

void printTime(struct tm Time){

  Serial.printf("%s, %s %02d %04d %02d:%02d:%02d",
               dayOfWeek(Time.tm_wday),  // Convert day of the week to string
               monthName(Time.tm_mon),   // Convert month to string
               Time.tm_mday,             // Day of the month
               Time.tm_year + 1900,      // Year
               Time.tm_hour,             // Hour
               Time.tm_min,              // Minute
               Time.tm_sec);             // Second


               return;

}

const char* tmToString(const tm &timeInfo) {
    static char buffer[15]; // Static buffer to hold the formatted string
    sprintf(buffer, "%02d%02d%02d%02d%02d%04d",
            timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec,
            timeInfo.tm_mday, timeInfo.tm_mon + 1, timeInfo.tm_year + 1900);
    return buffer; // Return the buffer
}

void StringToTm(const String& timeString) {

    // Extract individual components from the string
    int hour = timeString.substring(0, 2).toInt();
    int minute = timeString.substring(2, 4).toInt();
    int second = timeString.substring(4, 6).toInt();
    int day = timeString.substring(6, 8).toInt();
    int month = timeString.substring(8, 10).toInt() - 1; // Months are zero-based in tm struct
    int year = timeString.substring(10, 14).toInt() - 1900; // Years are relative to 1900 in tm struct

    // Populate the tm structure
    timeinfo.tm_hour = hour;
    timeinfo.tm_min = minute;
    timeinfo.tm_sec = second;
    timeinfo.tm_mday = day;
    timeinfo.tm_mon = month;
    timeinfo.tm_year = year;
}

void sendJsonMessage(const char* nodeID, const char* localTimeDate, int typeOfEvent, const char* cardUID, const char* direction,  int answer, const char* studentName) {
  // Create a JSON document
  StaticJsonDocument<200> doc;

 // Serial.println("MQTT Message uploading...");

  // Populate the JSON document with the provided data
  doc["nodeID"] = nodeID;
  doc["localTimeDate"] = localTimeDate;
  doc["typeOfEvent"] = typeOfEvent;
  doc["cardUID"] = cardUID;
  doc["direction"] = direction;
  doc["answer"] = answer; 
  doc["studentName"] = studentName;
 
  //direction: NB(Node-Border) ,BN(Border-Node), BC(Border-Cloud), CB(Cloud_Border)

  // Serialize the JSON document to a string
  char jsonBuffer[256]; // Adjust the buffer size as needed
  size_t n = serializeJson(doc, jsonBuffer);

  uint8_t tokenlen = 2;
  uint8_t randToken[tokenlen];

  srand(time(NULL));

  // Generate random numbers for the token array
  for (size_t i = 0; i < 2; i++) {
      randToken[i] = rand() % 256; // Generate a random number between 0 and 255
  }

  const uint8_t token[] = {randToken[0], randToken[1]}; // Assuming hexadecimal representation of 8074

  //1 = eventType or Message Type
  coap.send(CoAPServer_IP, CoAPServerPort, CoAPURL, COAP_CON, COAP_POST, token, tokenlen, (uint8_t *)jsonBuffer, n, COAP_APPLICATION_JSON,1);
  coap.send(CoAP_DEBUG, CoAPServerPort, CoAPURL, COAP_CON, COAP_POST, token, tokenlen, (uint8_t *)jsonBuffer, n, COAP_APPLICATION_JSON,1);


  Serial.printf("(EVENT) CoAP Message of type %d sent at time: ",typeOfEvent);
  requestLocalTime();
  printTime(timeinfo);

}

void analyzeMessage(const char* nodeID, const char* localTimeDate, int typeOfEvent, const char* cardUID, const char* direction,  int answer, const char* studentName){


  if (typeOfEvent == 1){ //1- request attendance 


    if (answer == -3)
    {
      Serial.println();
      Serial.printf("(EVENT) Denied attendance registration at time: ");
      requestLocalTime();
      printTime(timeinfo);
      Serial.println();

      Serial.printf("Reason: Student (%s) with card UID: (%s) already registered!", studentName, cardUID);
      Serial.println("");

             digitalWrite (redPin, HIGH);	// turn on the LED
      delay(250);	// wait for half a second or 500 milliseconds
      digitalWrite (redPin, LOW);	// turn off the LED
      delay(250);	// wait for half a second or 500 milliseconds
    }

    if (answer == -2)
    {
      Serial.println();
      Serial.printf("(EVENT) Denied attendance registration for student (%s) with card UID (%s) registration at time: ",studentName,cardUID);
      requestLocalTime();
      printTime(timeinfo);
      Serial.println();

      Serial.println("Reason: No class was found in this room!");
      Serial.println("");

             digitalWrite (redPin, HIGH);	// turn on the LED
      delay(250);	// wait for half a second or 500 milliseconds
      digitalWrite (redPin, LOW);	// turn off the LED
      delay(250);	// wait for half a second or 500 milliseconds
    }

    if (answer == -1)
    {
        Serial.println();
        Serial.printf("(EVENT) Denied attendance registration for student (%s) with card UID (%s) at time: ",studentName,cardUID);
        requestLocalTime();
        printTime(timeinfo);
        Serial.println();
        Serial.println("Reason: Student not registered in this class!");

               digitalWrite (redPin, HIGH);	// turn on the LED
      delay(250);	// wait for half a second or 500 milliseconds
      digitalWrite (redPin, LOW);	// turn off the LED
      delay(250);	// wait for half a second or 500 milliseconds
    }

        if (answer == -4)
    {
        Serial.println();
        Serial.printf("(EVENT) Denied attendance registration for student (%s) with card UID (%s) at time: ",studentName,cardUID);
        requestLocalTime();
        printTime(timeinfo);
        Serial.println();
        Serial.println("Reason: Card not registered!");

       digitalWrite (redPin, HIGH);	// turn on the LED
      delay(250);	// wait for half a second or 500 milliseconds
      digitalWrite (redPin, LOW);	// turn off the LED
      delay(250);	// wait for half a second or 500 milliseconds
    }
    

    if (answer == 1)
    {
      Serial.println();
       Serial.printf("(EVENT) Authorized attendance registration for student (%s) with card UID (%s) at time: ", studentName,cardUID);
       requestLocalTime();
       printTime(timeinfo);
       Serial.println();

       digitalWrite (ledPin, HIGH);	// turn on the LED
      delay(250);	// wait for half a second or 500 milliseconds
      digitalWrite (ledPin, LOW);	// turn off the LED
      delay(250);	// wait for half a second or 500 milliseconds
    }

      
  }




}

void restartWiFi() {
  // Disconnect from current network
  WiFi.disconnect();
  delay(1000); // Wait for the disconnection to complete

  // Reconnect to the network
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("Attempting to reconnect to WiFi...");
    WiFi.begin(ssid, password); // Replace ssid and password with your network credentials
    delay(5000); // Wait for connection
  }

  Serial.println("WiFi reconnected.");
}

void requestLocalTime(){
  coap.get(CoAPServer_IP,CoAPServerPort,CoAPtimeURL);
  delay(200); //ensure time is received
}

// CoAP client response callback
void callback_response(CoapPacket &packet, IPAddress ip, int port) {

  
  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;

  if(packet.messageid == 1){
      String message(p);

      StaticJsonDocument<200> doc; // Adjust the buffer size as needed
      DeserializationError error = deserializeJson(doc, p, packet.payloadlen+1);

      if (error) {
        Serial.println();
        Serial.print("deserializeJson() failed: in callback_response ");
        Serial.println(error.c_str());
        return;
      }

      // Extract values from JSON
      const char* nodeID = doc["nodeID"];
      const char* localTimeDate = doc["localTimeDate"];
      int typeOfEvent = doc["typeOfEvent"];
      const char* cardUID = doc["cardUID"];
      const char* direction = doc["direction"];
      int answer = doc["answer"];
      const char* studentName = doc["studentName"];

      analyzeMessage(nodeID,localTimeDate, typeOfEvent, cardUID, direction, answer, studentName);

   
  }



 if (packet.messageid == 2){
    String timeString = String(p);
    StringToTm(timeString);
  }
  
}


void setup() 
{
  Serial.begin(115200);   // Initiate a serial communication
  SPI.begin();      // Initiate  SPI bus
  mfrc522.PCD_Init();   // Initiate MFRC522
  pinMode (ledPin, OUTPUT); //led
  pinMode (redPin,OUTPUT);

  //connect to wifi 

  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to WiFi");

  coap.response(callback_response);

  coap.start();

  //requestTime 
  requestLocalTime();





}

void loop() 
{

    coap.loop();    

    // Look for new cards
    if ( ! mfrc522.PICC_IsNewCardPresent()) 
    {
      return;
    }

    // Select one of the cards
    if ( ! mfrc522.PICC_ReadCardSerial()) 
    {
      return;
    }

    unsigned long currentTime = millis();


    String content= "";
    byte letter;

    for (byte i = 0; i < mfrc522.uid.size; i++) 
    {
      content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
      content.concat(String(mfrc522.uid.uidByte[i], HEX));
    }

    content.toUpperCase();

    const char* timeString = tmToString(timeinfo);

    content = content.substring(1); //remove first character
    const char* UID = content.c_str();

    if (lastReadTime == 0){
      Serial.println();
      Serial.printf("(EVENT) Card is read with UID (%s) at time: ",content);
      requestLocalTime();
      printTime(timeinfo);
      Serial.println();

      strncpy(lastSentUID, UID, sizeof(lastSentUID) - 1);
      lastSentUID[sizeof(lastSentUID) - 1] = '\0'; // Ensure null termination

      sendJsonMessage(node_id,timeString,1,UID,"NS",0,"null");
      lastReadTime = currentTime;

    }


    if (currentTime - lastReadTime >= readDelay && strcmp(lastSentUID, UID) == 0){ //caso tempo passado seja menor que o delay definido e o cartao seja o mesmo
      Serial.println();
      Serial.printf("(EVENT) Card is read with UID (%s) at time: ",content);
      requestLocalTime();
      printTime(timeinfo);
      Serial.println();

      strncpy(lastSentUID, UID, sizeof(lastSentUID) - 1);
      lastSentUID[sizeof(lastSentUID) - 1] = '\0'; // Ensure null termination

      sendJsonMessage(node_id,timeString,1,UID,"NS",0,"null");
      lastReadTime = currentTime;

    }

        
    if (strcmp(UID, lastSentUID) != 0){ //caso cartao seja diferente independetmente do delay
      Serial.println();
      Serial.printf("(EVENT) Card is read with UID (%s) at time: ",content);
      requestLocalTime();
      printTime(timeinfo);
      Serial.println();

      strncpy(lastSentUID, UID, sizeof(lastSentUID) - 1);
      lastSentUID[sizeof(lastSentUID) - 1] = '\0'; // Ensure null termination

      sendJsonMessage(node_id,timeString,1,UID,"NS",0,"null");
      lastReadTime = currentTime;
    }





} 
