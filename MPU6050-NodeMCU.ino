//Include Wire library to use I2C communication protocl
#include<Wire.h>
//Check if we are using ESP8266 module or NodeMCU
#ifdef ESP8266
 #include <ESP8266WiFi.h>
 #else
 #include <WiFi.h>
#endif
//Include secrets.h file, containing the Wifi login, MQTT Connection and CA Cert credentials
#include "secrets.h"
//Include libraries for packaging JSON and sending the packets over WiFi to MQTT Broker 
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

const int led = 5;

//Initializing Secure WiFi Client object
WiFiClientSecure espClient;

//Initializing MQTT Client using the secure WiFi Client
PubSubClient client(espClient);
//Creates a buffer size JSON packets or the messages that are to be sent over MQTT
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];

//I2C Address for MPU6050
const int MPU=0x68;  // Can be 0x69 if AD0 Pin is True (VCC)

//Variables for raw values from MPU ADC
int AcX,AcY,AcZ,GyX,GyY,GyZ;
int16_t Tmp;

int minVal=0;
int maxVal=65536; //2^16 levels from MPU6050 ADC 

int offsetTemp = 35; // from MPU datasheet
int offsetTempLevels = 521; // from MPU datasheet

//The followiing block performs the WiFi connection and prints the IP address to the terminal
void setup_wifi(){
  delay(10);
  Serial.print("\nConnecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  randomSeed(micros());
  Serial.println("\nWiFi connected\nIP address: ");
  Serial.println(WiFi.localIP());
}

//The following block is responsible for establishing connection with the MQTT server
//It also attempts reconnection whenever the device does not connect
void reconnect(){
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "ESP8266Client-";   // Create a random client ID
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("connected");

      client.subscribe("led_state");   // subscribe the topics here

    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");   // Wait 5 seconds before retrying
      delay(5000);
    }
  }  
}
//The following function handles the payload creating and management
//It drops the payload to the MQTT Broker for sending it to the server, which in our case is HiveMQ Cloud
void callback(char* topic, byte* payload, unsigned int length) {
  String incommingMessage = "";
  for (int i = 0; i < length; i++) incommingMessage+=(char)payload[i];

  Serial.println("Message arrived ["+String(topic)+"]"+incommingMessage);

  //--- check the incomming message
    if( strcmp(topic,"led_state") == 0){
     if (incommingMessage.equals("1")) digitalWrite(led, HIGH);   // Turn the LED on
     else digitalWrite(led, LOW);  // Turn the LED off
  }

}

//The following function publishes the message packet to the server
void publishMessage(const char* topic, String payload , boolean retained){
  if (client.publish(topic, payload.c_str(), true))
      Serial.println("Message publised ["+String(topic)+"]: "+payload);
}

void setup()
{
  Serial.begin(9600); // Begin Serial communication

  Wire.begin(); //Initializing the Wire library

  delay(500);
  setup_wifi(); //Connecting to the WiFi

  Wire.beginTransmission(MPU); // begin transmission with MPU address
  Wire.write(0x6B); 
   
  //Start MPU-6050 sensor 
  Wire.write(0); 
  Wire.endTransmission(true);   

  #ifdef ESP8266
    espClient.setInsecure();
  #else
    espClient.setCACert(root_ca);      // enable this line and the the "certificate" code for secure connection
  #endif

  client.setServer(mqtt_server, mqtt_port); //establishing connection with the MQTT server
  client.setCallback(callback); 
  
}
void loop()
{
  if (!client.connected()) reconnect(); // check if client is connected
  client.loop();

  Wire.beginTransmission(MPU);
  Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  //Get data from Sensor (14 consecutive bytes)
  Wire.requestFrom(MPU,14,true);  
  //Read data byte by byte (16 bits is 8 bits | 8 bits)
  //Each value is composed by 16 bits (2 bytes)
  GyX=Wire.read()<<8|Wire.read();  //0x3B (GYRO_XOUT_H) & 0x3C (GYRO_XOUT_L)     
  GyY=Wire.read()<<8|Wire.read();  //0x3D (GYRO_YOUT_H) & 0x3E (GYRO_YOUT_L)
  GyZ=Wire.read()<<8|Wire.read();  //0x3F (GYRO_ZOUT_H) & 0x40 (GYRO_ZOUT_L)
  Tmp=Wire.read()<<8|Wire.read();  //0x41 (TEMP_OUT_H) & 0x42 (TEMP_OUT_L)
  AcX=Wire.read()<<8|Wire.read();  //0x43 (ACCEL_XOUT_H) & 0x44 (ACCEL_XOUT_L)
  AcY=Wire.read()<<8|Wire.read();  //0x45 (ACCEL_YOUT_H) & 0x46 (ACCEL_YOUT_L)
  AcZ=Wire.read()<<8|Wire.read();  //0x47 (ACCEL_ZOUT_H) & 0x48 (ACCEL_ZOUT_L)

  //Adapts to range from 0 to 2000 m/s^2
  AcX = map(AcX,minVal,maxVal,2000,0);
  AcY = map(AcY,minVal,maxVal,2000,0);
  AcZ = map(AcZ,minVal,maxVal,2000,0);
  
  //Adapts to range from -180º to 180º
  int xAng = map(GyX,minVal,maxVal,180,-180);
  int yAng = map(GyY,minVal,maxVal,180,-180);
  int zAng = map(GyZ,minVal,maxVal,180,-180); 
   
  //Send X axis accelerometer value for serial monitor
  Serial.print("AcX = "); Serial.print(AcX);
   
  //Send Y axis accelerometer value for serial monitor
  Serial.print(" | AcY = "); Serial.print(AcY);

  //Send Z axis accelerometer value for serial monitor
  Serial.print(" | AcZ = "); Serial.print(AcZ);

   
  //Send Temperature value for serial
  //Calculates the temperature given the datasheet values (ºC)
  // 340 is the number of levels per ºC (from datasheet)
  Serial.print(" | Tmp = "); Serial.print(float(Tmp + offsetTempLevels)/340 + offsetTemp);
  
   
  //Send X axis gyroscope angle value for serial monitor
  Serial.print(" | GyX = "); Serial.print(xAng);

   
  //Send Y axis gyroscope angle value for serial monitor  
  Serial.print(" | GyY = "); Serial.print(yAng);
 
   
  //Send Z axis gyroscope angle value for serial monitor
  Serial.print(" | GyZ = "); Serial.println(zAng);
 
  //Create a Json object
  DynamicJsonDocument doc(1024);

  doc["deviceId"] = "NodeMCU"; //Define the 'key':'value' pairs for the JSON file
  doc["siteId"] = "Task1.2";
  doc["AccelX"] = AcX;
  doc["AccelY"] = AcY;

  //Serialize or reduce the JSON so that there are no spaces before transmission
  char mqtt_message[128];
  serializeJson(doc, mqtt_message);
  //Publish the thus created packet
  publishMessage("esp8266_data", mqtt_message, true);

  //Wait 5000 ms, after that, start new measurement and transmit.
  delay(5000);
}