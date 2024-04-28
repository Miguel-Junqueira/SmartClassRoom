#include <WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <string.h>
#include <time.h>
#include <SPI.h>
#include <ESPping.h>


const char *wifi_network_ssid = "";
const char *wifi_network_password = "";

// Replace with your network credentials
const char *dumbAPssid = "ESP32-Access-Point";
const char *dumbAPpassword = "123456789";

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 0;
const int daylightOffset_sec = 0;

// MQTT Broker
// const char *mqtt_broker = "192.168.8.246";
const char *mqtt_broker = "192.168.1.84";
const char *attendanceRequestTopic = "attendanceRequest";
const char *attendanceRequestServerTopic = "attendanceRequestServer";
const char *attendanceResponseTopic = "attendanceResponse";
const char *getTempfromNodesTopic = "getTempfromNodes";
const char *sendTempfromNodesTopic = "sendTempfromNodes";
const char *storeTempfromNodesTopic ="storeTempfromNodes";
const char *mqtt_username = "";
const char *mqtt_password = "";
const int mqtt_port = 1883;

const char node_id[] = "BR01";
struct tm timeinfo;

uint8_t token[2];  // tokenLen = 2

IPAddress CoAP_DEBUG{ 192, 168, 4, 10 };

IPAddress nodeIP;
int nodePort;

WiFiUDP udp;
Coap coap(udp, 512);

WiFiClient espClient;
PubSubClient client(espClient);

//para prevenir que outra leitura de outro nó altere o ip e a porta
bool gotResponseFromLastRequest = true;

const char *dayOfWeek(int dow) {
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

const char *monthName(int month) {
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

void printTime(struct tm Time) {

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

const char *tmToString(const tm &timeInfo) {
  static char buffer[15];  // Static buffer to hold the formatted string
  sprintf(buffer, "%02d%02d%02d%02d%02d%04d",
          timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec,
          timeInfo.tm_mday, timeInfo.tm_mon + 1, timeInfo.tm_year + 1900);
  return buffer;  // Return the buffer
}

void sendMQTTMessagetoServer(const char *nodeID, const char *localTimeDate, int typeOfEvent, const char *cardUID, const char *direction, int answer, const char *studentName) {
  StaticJsonDocument<200> doc;

  doc["nodeID"] = nodeID;
  doc["localTimeDate"] = localTimeDate;
  doc["typeOfEvent"] = typeOfEvent;
  doc["cardUID"] = cardUID;
  doc["direction"] = direction;
  doc["answer"] = answer;
  doc["studentName"] = studentName;

  char jsonBuffer[256];  // Adjust the buffer size as needed
  size_t n = serializeJson(doc, jsonBuffer);

  if (client.connected()) {
    client.publish(attendanceRequestServerTopic, jsonBuffer, false);
    Serial.printf("(EVENT) MQTT Message of type %d sent to Server at time: ", typeOfEvent);
    getLocalTime(&timeinfo);
    printTime(timeinfo);
    Serial.println();
  }
}

void reconnect() {

  String client_id = "BR01";

  Serial.println("(INFO) Lost connection to MQTT Server!");

  while (!client.connected()) {
    Serial.println("(EVENT) Attempting MQTT reconnection...");
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("(INFO) Connected to MQTT Server CS01!");
      client.subscribe(attendanceRequestTopic);
      client.subscribe(attendanceResponseTopic);
    } else {
      Serial.println();
      if (Ping.ping("google.com", 3) == 0 || WiFi.status() != WL_CONNECTED) {
        Serial.println("(INFO) WiFi connectivity is DOWN!");
        restartWiFi();  //also periodically check connection to wifi
      }
      Serial.println("(INFO) WiFi connectivity is UP!");
      delay(5000);
    }
  }
  return;
}

void restartWiFi() {
  // Disconnect from current network
  Serial.println("(EVENT) Connection lost to WiFi network. Restarting WiFi...");
  WiFi.disconnect();
  delay(1000);  // Wait for the disconnection to complete
  Serial.println("(EVENT) Attempting to reconnect to WiFi network...");


  // Reconnect to the network
  while (WiFi.status() != WL_CONNECTED) {
    WiFi.begin(wifi_network_ssid, wifi_network_password);  // Replace ssid and password with your network credentials
    delay(5000);                                           // Wait for connection
  }

  Serial.println("(EVENT) WiFi reconnected.");
  Serial.println();
}

void callback_attendance(CoapPacket &packet, IPAddress ip, int port) {  // funcao que é executada quando é recebido pacote do tipo CoAP requisicao de presenca

  if(gotResponseFromLastRequest == false){
    return; //processa uma mensagem de cada vez
  }

  char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;
  String message(p);

  // Parse JSON payload
  StaticJsonDocument<200> doc;  // Adjust the buffer size as needed
  DeserializationError error = deserializeJson(doc, p, packet.payloadlen + 1);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  // Extract values from JSON
  const char *nodeID = doc["nodeID"];
  const char *localTimeDate = doc["localTimeDate"];
  int typeOfEvent = doc["typeOfEvent"];
  const char *cardUID = doc["cardUID"];
  const char *direction = doc["direction"];
  int answer = doc["answer"];
  const char *studentName = doc["studentName"];

  Serial.printf("(EVENT) CoAP Message of type %d received at time: ", typeOfEvent);
  getLocalTime(&timeinfo);
  printTime(timeinfo);
  Serial.println();

  for (size_t i = 0; i < packet.tokenlen; i++) {
    token[i] = packet.token[i];
  }

  nodeIP = ip;
  nodePort = port;
  gotResponseFromLastRequest = false;
  sendMQTTMessagetoServer(nodeID, localTimeDate, typeOfEvent, cardUID, direction, answer, studentName);
}

void callback_time(CoapPacket &packet, IPAddress ip, int port) {
  getLocalTime(&timeinfo);
  coap.sendResponse(ip, port, 2, tmToString(timeinfo));          // message ID = 2 (requisicao da hora local)
  coap.sendResponse(CoAP_DEBUG, port, 2, tmToString(timeinfo));  // message ID = 2 (requisicao da hora local)
}


void callback_tempHumsentfromNodes(CoapPacket &packet, IPAddress ip, int port){

    char p[packet.payloadlen + 1];
  memcpy(p, packet.payload, packet.payloadlen);
  p[packet.payloadlen] = NULL;
  String message(p);

  // Parse JSON payload
  StaticJsonDocument<200> doc;  // Adjust the buffer size as needed
  DeserializationError error = deserializeJson(doc, p, packet.payloadlen + 1);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  const char *nodeID = doc["nodeID"];
  const char *localTimeDate = doc["localTimeDate"];
  int typeOfEvent = doc["typeOfEvent"];
  const char *direction = doc["direction"];
  const char *temperature = doc["temperature"];
  const char *humidity = doc["humidity"];

  char jsonBuffer[256];  // Adjust the buffer size as needed
  size_t n = serializeJson(doc, jsonBuffer);

  if (client.connected()) {
    client.publish(storeTempfromNodesTopic, jsonBuffer, false);
    Serial.println();
    Serial.printf("(EVENT) MQTT Message of type %d sent to Server at time: ", typeOfEvent);
    getLocalTime(&timeinfo);
    printTime(timeinfo);
    Serial.println();
  }
}


void callbackMQTT(char *topic, byte *payload, unsigned int length) {

  // verifica o topico MQTT

  if (strcmp(topic, "attendanceResponse") == 0) {
    gotResponseFromLastRequest = true;
    sendAttendanceResponsetoNode(payload, length);
    return;
  }

  if (strcmp(topic,"getTempfromNodes") == 0){
    requestTemperatureInformation();
    return;
  }


  else {
    return;  // caso nao estejamos no topico MQTT correto
  }
}

void sendAttendanceResponsetoNode(byte *payload, unsigned int length) {

  // funcao recebe mensagem MQTT vinda do servidor processa-a e envia mensagem do tipo CoAP para o nó correto
  //  Parse JSON payload

  StaticJsonDocument<200> doc;  // Adjust the buffer size as needed
  DeserializationError error = deserializeJson(doc, payload, length);

  if (error) {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  // Extract values from JSON
  const char *nodeID = doc["nodeID"];
  const char *localTimeDate = doc["localTimeDate"];
  int typeOfEvent = doc["typeOfEvent"];
  const char *cardUID = doc["cardUID"];
  const char *direction = doc["direction"];
  int answer = doc["answer"];
  const char *studentName = doc["studentName"];

  Serial.printf("(EVENT) MQTT Message of type %d received at time: ", typeOfEvent);
  getLocalTime(&timeinfo);
  printTime(timeinfo);
  Serial.println();

  char jsonBuffer[256];  // Adjust the buffer size as needed
  size_t n = serializeJson(doc, jsonBuffer);

  coap.sendResponse(nodeIP, nodePort, 1, jsonBuffer, n, COAP_VALID, COAP_APPLICATION_JSON, token, 2);      // message ID = 1 (requisicao de presenca)
  coap.sendResponse(CoAP_DEBUG, nodePort, 1, jsonBuffer, n, COAP_VALID, COAP_APPLICATION_JSON, token, 2);  // message ID = 1 (requisicao de presenca)

  Serial.printf("(EVENT) CoAP Message of type %d sent to Node at time: ", typeOfEvent);
  getLocalTime(&timeinfo);
  printTime(timeinfo);
  Serial.println();

  return;
}

void requestTemperatureInformation(){

    nodePort = 5683;
  
    for (int i = 2; i <= 254; ++i) { // Scan through IP range 192.168.4.1 to 192.168.4.254
      IPAddress targetIP(192, 168, 4, i);
      coap.sendResponse(targetIP,nodePort,0);
    }

    Serial.println("(EVENT) Sent temperature and humidity requests to all nodes!");
    gotResponseFromLastRequest = true;
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

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }

  Serial.print("\n[+] Connected to the WiFi network with local IP : ");
  Serial.println(WiFi.localIP());

  // establish mqtt connection

  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callbackMQTT);
  while (!client.connected()) {
    String client_id = "BR01";
    Serial.printf("(INFO) The client %s is attempting to connect to the Cloud MQTT server\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("(INFO) Connected to MQTT Server CS01!");
    } else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }

    client.subscribe(attendanceRequestTopic);
    client.subscribe(attendanceResponseTopic);
    client.subscribe(getTempfromNodesTopic);
  }

  Serial.println("Setting up Attendance CoAP Handler");
  coap.server(callback_attendance, "attendanceHandler");

  Serial.println("Setting up Time CoAP Handler");
  coap.server(callback_time, "getTime");

  Serial.println("Setting up Temp and Humidity CoAP Handler");
  coap.server(callback_tempHumsentfromNodes , "sendTempHumInfofromNodes");

  // start coap server/client
  coap.start();

  // get current time

  configTime(gmtOffset_sec, 3600, ntpServer);
  getLocalTime(&timeinfo);


}

void loop() {

  coap.loop();

  if (!client.connected()) {
    reconnect();  // Reconnect to MQTT broker if connection is lost
  }
  client.loop();  // Handle MQTT client events



}
