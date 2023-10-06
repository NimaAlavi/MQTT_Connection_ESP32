#include <HardwareSerial.h>
#include <WiFi.h>
#include <PubSubClient.h>

HardwareSerial SerialPort(2); // use UART2

const int msgSize = 20;
char buf_in[msgSize];
byte buf_out[8];

int delayTime = 1000;

String message;
// WiFi
const char *ssid = ""; // Enter your WiFi name
const char *password = "";  // Enter WiFi password

// MQTT Broker
const char *mqtt_broker = "mbr.atro1.com";
const char *topic = "esp/master";
const char *mqtt_username = "ahalab";
const char *mqtt_password = "ahalab";
const int mqtt_port = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

// relay Const
byte slaveRelayAddr = 0x01;
byte writeCoilFunc = 0x05;
byte readCoilFunc = 0x01;
byte startingAddrCoil1[] = {0x00, 0x01};
byte startingAddrCoil2[] = {0x00, 0x02};
byte valueCoil_on[] = {0xFF, 0x00};
byte valueCoil_off[] = {0x00, 0x00};
byte quantityofCoilsRelay[] = {0x00, 0x02};

// temp Const
byte slaveTempAddr = 0x02;
byte readRegFunc = 0x03;
byte startingAddrReg[2] = {0x00, 0x02};
byte quantityofRegTemp[2] = {0x00, 0x02};

void setup()  
{
  SerialPort.begin(4800, SERIAL_8N1, 16, 17); 
  Serial.begin(115200);
  // connecting to a WiFi network
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
  //connecting to a mqtt broker
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(callback);
  while (!client.connected()) {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
    if (client.connect(client_id.c_str(), mqtt_username, mqtt_password)) {
      Serial.println("Public emqx mqtt broker connected");
    } 
    else {
      Serial.print("failed with state ");
      Serial.print(client.state());
      delay(2000);
    }
  }
  // publish and subscribe
  client.publish(topic, "connection started");
  client.subscribe(topic);
}

void loop()
{
  client.loop();
  // Read the Relay Status
  byte msg1[] = {slaveRelayAddr, readCoilFunc, startingAddrCoil1[0], startingAddrCoil1[1], quantityofCoilsRelay[0], quantityofCoilsRelay[1]};
  ADU_Request(msg1);
  delay(100);

  byte msg2[] = {slaveTempAddr, readRegFunc, startingAddrReg[0], startingAddrReg[1], quantityofRegTemp[0], quantityofRegTemp[1]};
  ADU_Request(msg2);
  delay(10);

  delayTime = 0;
  while(delayTime <= 10000)
  {
    client.loop();
    // request
    if (message != "")
    {
      // switch the relay situation
      if (message == "relay1=> on") 
      {
        byte msg[] = {slaveRelayAddr, writeCoilFunc, startingAddrCoil1[0], startingAddrCoil1[1], valueCoil_on[0], valueCoil_on[1]};
        ADU_Request(msg);
      }
      if (message == "relay1=>off") 
      {
        byte msg[] = {slaveRelayAddr, writeCoilFunc, startingAddrCoil1[0], startingAddrCoil1[1], valueCoil_off[0], valueCoil_off[1]};
        ADU_Request(msg);
      }
      if (message == "relay2=> on") 
      {
        byte msg[] = {slaveRelayAddr, writeCoilFunc, startingAddrCoil2[0], startingAddrCoil2[1], valueCoil_on[0], valueCoil_on[1]};
        ADU_Request(msg);
      }
      if (message == "relay2=>off") 
      {
        byte msg[] = {slaveRelayAddr, writeCoilFunc, startingAddrCoil2[0], startingAddrCoil2[1], valueCoil_off[0], valueCoil_off[1]};
        ADU_Request(msg);
      }
    }
    // response
    if (SerialPort.available() > 0)
    {
      SerialPort.readBytesUntil('\n', buf_out, 8);
      
      uint16_t crc_u = 0;
      crc_u = crc16(buf_out, 6);
      byte crc_b[2] = {crc_u & 0xFF, (crc_u>>8) & 0xFF};
      
      Serial.print("Response : ");
      Serial.printf("%.2X", buf_out[0]);  
      Serial.printf("%.2X", buf_out[1]);  
      Serial.printf("%.2X", buf_out[2]);  
      Serial.printf("%.2X", buf_out[3]);  
      Serial.printf("%.2X", buf_out[4]); 
      Serial.printf("%.2X", buf_out[5]);
      Serial.printf("%.2X", buf_out[6]); 
      Serial.printf("%.2X", buf_out[7]);  
      Serial.println();

      if (crc_b[0] == buf_out[6] && crc_b[1] == buf_out[7])
      {
        client.publish(topic, buf_out, 8);
      }
      else
      {
        // Serial.print("Response : FALSE");
        // Serial.println();
        client.publish(topic, "Wrong");
      }
      for(int i=0 ; i<8 ; i++){
      buf_in[i] = '\0';}
    }
    delayTime += 100;
    delay(200);
  }
  // message = "";
}

void callback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  Serial.print("Message:");
  message="";
  for (int i = 0; i < length; i++) {
    message = message + (char)payload[i];
  }
  Serial.println(String(message));
  Serial.println("-----------------------");
}

void ADU_Request(byte msg[])
{
  uint16_t crc_u = 0;
  crc_u = crc16(msg, 6);

  byte crc_b[2] = {crc_u & 0xFF, (crc_u>>8) & 0xFF};

  // Serial.printf("%.2X", msg[0]);  
  // Serial.printf("%.2X", msg[1]);  
  // Serial.printf("%.2X", msg[2]);  
  // Serial.printf("%.2X", msg[3]);  
  // Serial.printf("%.2X", msg[4]); 
  // Serial.printf("%.2X", msg[5]); 
  // Serial.printf("%.2X", crc_b[0]); 
  // Serial.printf("%.2X", crc_b[1]); 
  // Serial.println();

  SerialPort.write(msg, 6);
  SerialPort.write(crc_b, 2);
  SerialPort.write('\n');
}

byte crc16(byte msg[], int len) {

  uint16_t crc = 0xFFFF;

  for (int i=0; i<len; i++)
  {
    crc = crc ^ ((uint16_t)msg[i] << 8);
    for (int j=0; j<8; j++){
      if (crc & 0x8000){
        crc = (crc << 1) ^ 0xA001;
      }
      else
      {
        crc <<= 1;
      }
    }
  }
  return crc;
}

