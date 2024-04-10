#include <WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <string.h>
#include <time.h>
#include <SPI.h>

const char* wifi_network_ssid     = "";
const char* wifi_network_password =  "";

// Replace with your network credentials
const char* dumbAPssid     = "";
const char* dumbAPpassword = "";

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 0;

// MQTT Broker
const char *mqtt_broker = "192.168.8.246";
const char *topic = "smartClassRoom";
const char *mqtt_username = "";
const char *mqtt_password = "";
const int mqtt_port = 1883;


const char node_id[] = "BR01";
struct tm timeinfo;

uint8_t token[2];  //tokenLen = 2

IPAddress CoAP_DEBUG{192,168,4,10};


IPAddress nodeIP;
int nodePort;

WiFiUDP udp;
Coap coap(udp,512);

WiFiClient espClient;
PubSubClient client(espClient);


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

  // Publish the JSON message via MQTT
  if (client.connected()) {
      client.publish(topic, jsonBuffer, n);
      Serial.printf("(EVENT) MQTT Message of type %d sent to Server at time: ",typeOfEvent);
      getLocalTime(&timeinfo);
      printTime(timeinfo);
      Serial.println();
  }
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    if (client.connect("ArduinoClient")) {
      Serial.println("connected");
      client.subscribe(topic);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
  return;
}

void callback_attendance(CoapPacket &packet, IPAddress ip, int port) { //funcao que é executada quando é recebido pacote do tipo CoAP


  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;
  String message(p);

  // Parse JSON payload
  StaticJsonDocument<200> doc; // Adjust the buffer size as needed
  DeserializationError error = deserializeJson(doc, p, packet.payloadlen+1);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  // Extract values from JSON
  const char* nodeID = doc["nodeID"];

  //if(strcmp(nodeID, node_id) != 0) return; //ignora mensagem se nao for para este nó especifico

  const char* localTimeDate = doc["localTimeDate"];
  int typeOfEvent = doc["typeOfEvent"];
  const char* cardUID = doc["cardUID"];
  const char* direction = doc["direction"];
  int answer = doc["answer"];
  const char* studentName = doc["studentName"];



  Serial.printf("(EVENT) CoAP Message of type %d received at time: ",typeOfEvent);
  getLocalTime(&timeinfo);
  printTime(timeinfo);
  Serial.println();


   for (size_t i = 0; i < packet.tokenlen; i++) {
        token[i] = packet.token[i];
    }

  nodeIP = ip;
  nodePort = port;
  sendJsonMessage(nodeID,localTimeDate, typeOfEvent, cardUID, direction, answer, studentName);

}

void callback_time(CoapPacket &packet, IPAddress ip, int port){
  getLocalTime(&timeinfo);
  coap.sendResponse(ip, port, 2, tmToString(timeinfo)); //message ID = 2 (requisicao da hora local)
}


// CoAP client response callback
void callback_response(CoapPacket &packet, IPAddress ip, int port) {



  Serial.println("[Coap Response got]");
  
  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;
  
  Serial.println(p);
}

void callbackMQTT(char *topic, byte *payload, unsigned int length) {

    // Parse JSON payload
  StaticJsonDocument<200> doc; // Adjust the buffer size as needed
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.print("deserializeJson() failed: ");
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


  if(strcmp(direction, "NS") == 0){ //ignora mensagem se originar no nó 
    return; 
  }

  Serial.printf("(EVENT) MQTT Message of type %d received at time: ",typeOfEvent);
  getLocalTime(&timeinfo);
  printTime(timeinfo);
  Serial.println();


  char jsonBuffer[256]; // Adjust the buffer size as needed
  size_t n = serializeJson(doc, jsonBuffer);


 coap.sendResponse(nodeIP, nodePort, 1, jsonBuffer,n,COAP_VALID,COAP_APPLICATION_JSON,token,2); //message ID = 1 (requisicao de presenca)
 coap.sendResponse(CoAP_DEBUG, nodePort, 1, jsonBuffer,n,COAP_VALID,COAP_APPLICATION_JSON,token,2); //message ID = 1 (requisicao de presenca)

 Serial.printf("(EVENT) CoAP Message of type %d sent to Node at time: ",typeOfEvent);
 getLocalTime(&timeinfo);
 printTime(timeinfo);
 Serial.println();

  return;
}

void setup() {
  Serial.begin(115200);


  WiFi.mode(WIFI_AP_STA);

  Serial.println("\n[*] Creating ESP32 Dumb AP");
  WiFi.softAP(dumbAPssid, dumbAPpassword);
  Serial.print("[+] Dumb AP Created with IP Gateway ");
  Serial.println(WiFi.softAPIP());

  WiFi.begin(wifi_network_ssid, wifi_network_password);
  Serial.println("\n[*] Connecting to WiFi Network");


  while(WiFi.status() != WL_CONNECTED)
    {
      Serial.print(".");
      delay(100);
    }

  Serial.print("\n[+] Connected to the WiFi network with local IP : ");
  Serial.println(WiFi.localIP());

  //establish mqtt connection

  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callbackMQTT);
  while (!client.connected()) {
        String client_id = "BR01";
        Serial.printf("The client %s is attempting to connect to the Cloud MQTT broker\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
          Serial.println("MQTT Server CS01 broker connected");
    } else {
          Serial.print("failed with state ");
          Serial.print(client.state());
          delay(2000);
    }

    client.subscribe(topic);
  }


  Serial.println("Setting up Attendance CoAP Server Feature");
  coap.server(callback_attendance, "smartClassRoom");

  Serial.println("Setting up Time CoAP Server Feature");
  coap.server(callback_time,"getTime");

  Serial.println("Setup Response Callback");
  coap.response(callback_response);

  // start coap server/client
  coap.start();


   //get current time

  configTime(gmtOffset_sec, 3600, ntpServer);
  getLocalTime(&timeinfo);


  
}

void loop(){

   coap.loop();

  
  if (!client.connected()) {
    reconnect(); // Reconnect to MQTT broker if connection is lost
  }
  client.loop(); // Handle MQTT client events

}
