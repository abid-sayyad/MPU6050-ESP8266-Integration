# MPU6050-NodeMCU interfacing and data transmission over MQTT
This project demonstrates how to interface MPU6050 with NodeMCU and transmit data over MQTT protocol. The MPU6050 is a 6 Degree of Freedom (DOF) accelerometer and gyroscope that can detect motion and position changes in all three axes. NodeMCU is a low-cost open-source IoT platform that uses the ESP8266 Wi-Fi chip. MQTT is a lightweight messaging protocol designed for IoT devices.
This code is designed for the NodeMCU board to interface with an MPU6050 sensor via I2C protocol and send the collected data to an MQTT server for cloud storage. The transmitted data is in the JSON format.


## Hardware Requirements
- NodeMCU board
- MPU6050 sensor
- WiFi connection
- MQTT Broker (in our case, we are using HiveMQ Cloud)


## Wiring Schematic
| NodeMCU  | MPU6050 |
| ------------- | ------------- |
| VCC | 3V3  |
| GND  | GND  |
| SDA  | D2  |
| SCL | D1  |
![Uploading WhatsApp Image 2023-04-05 at 07.02.49 (1).jpeg…]()


## Usage
Clone this repository
Open the code in the Arduino IDE
Make the required changes to the "secrets.h" file with your credentials
Upload the code to the NodeMCU board
Open the Serial Monitor to monitor the collected data
View the collected data on the MQTT Broker or on the dashboard provided by the Broker.
## Code Details
The code begins by including the required libraries, "Wire.h", "WiFi.h", "ArduinoJson.h", and "PubSubClient.h". The secrets.h file is included, which contains the WiFi login, MQTT Connection, and CA Cert credentials.

The WiFi connection is established, and the IP address is printed to the terminal. The MQTT server connection is then attempted, and the device attempts reconnection whenever the device does not connect.

The callback function handles the payload creating and management. It drops the payload to the MQTT Broker for sending it to the server.

The publishMessage function publishes the message packet to the server.

The loop function gets data from the MPU6050 sensor and sends it to the MQTT server in the JSON format.

## Demo
Insert demo video here
