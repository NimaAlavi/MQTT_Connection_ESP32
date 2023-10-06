#include <HardwareSerial.h>

HardwareSerial SerialPort(2); // use UART2

const int msgSize = 8;
byte buf_in[msgSize];

const int yellowLed = 26;
const int blueLed = 14;

byte slaveRelayAddr = 0x01;
byte writeCoilFunc = 0x05;
byte readCoilFunc = 0x01;
byte startingAddrCoil1[] = {0x00, 0x01};
byte startingAddrCoil2[] = {0x00, 0x02};
byte valueCoil_on[] = {0xFF, 0x00};
byte valueCoil_off[] = {0x00, 0x00};
byte quantityofCoilsRelay[] = {0x00, 0x02};

void setup()  
{
  SerialPort.begin(4800, SERIAL_8N1, 16, 17); 
  Serial.begin(4800);
  pinMode(yellowLed, OUTPUT);
  pinMode(blueLed, OUTPUT);

  digitalWrite(yellowLed, LOW);
  digitalWrite(blueLed, LOW);
}


void loop()  
{ 
  while (SerialPort.available() <= 0);
  
  byte buf_out[8];

  SerialPort.readBytesUntil('\n', buf_in, 8);

  // Serial.print("buf_in: ");  
  // Serial.printf("%.2X", buf_in[0]);  
  // Serial.printf("%.2X", buf_in[1]);  
  // Serial.printf("%.2X", buf_in[2]);  
  // Serial.printf("%.2X", buf_in[3]);  
  // Serial.printf("%.2X", buf_in[4]);  
  // Serial.printf("%.2X", buf_in[5]);  
  // Serial.printf("%.2X", buf_in[6]);  
  // Serial.printf("%.2X", buf_in[7]);  
  // Serial.println();

  // Check if it's for this slave or not
  if (buf_in[0] != slaveRelayAddr)
  {
    return;
  }

  if (buf_in[1] == readCoilFunc)
  {
    buf_out[0] = buf_in[0];
    buf_out[1] = readCoilFunc;
    if (buf_in[2] == 0x00 && buf_in[3] == 0x01)
    {
      buf_out[2] = 0x00;
      buf_out[3] = 0x01;
      if (buf_in[4] == 0x00 && buf_in[5] == 0x01)
      {
        if (digitalRead(blueLed) == HIGH)
        {
          buf_out[4] = 0x00;
          buf_out[5] = 0x01;
        }
        else
        {
          buf_out[4] = 0x00;
          buf_out[5] = 0x00;
        }
      }
      else if (buf_in[4] == 0x00 && buf_in[5] == 0x02)
      {
        if (digitalRead(blueLed) == HIGH)
        {
          if (digitalRead(yellowLed) == HIGH)
          {
            buf_out[4] = 0x00;
            buf_out[5] = 0x03;
          }
          if (digitalRead(yellowLed) == LOW)
          {
            buf_out[4] = 0x00;
            buf_out[5] = 0x01;
          }
        }
        else
        {
          if (digitalRead(yellowLed) == HIGH)
          {
            buf_out[4] = 0x00;
            buf_out[5] = 0x02;
          }
          if (digitalRead(yellowLed) == LOW)
          {
            buf_out[4] = 0x00;
            buf_out[5] = 0x00;
          }
        }
      }
    }
    else
    {
      buf_out[0] = 0x80 + readCoilFunc;
      buf_out[1] = 0x03;
      ADU_Response(buf_out, 2);
      // return;
    }

    uint16_t crc_u = 0;
    crc_u = crc16(buf_in, 6);
    byte crc_b[2] = {crc_u & 0xFF, (crc_u>>8) & 0xFF};
    
    if (crc_b[0] == buf_in[6] && crc_b[1] == buf_in[7])
    {
      crc_u = crc16(buf_out, 6);
      byte crc_b[2] = {crc_u & 0xFF, (crc_u>>8) & 0xFF};

      buf_out[6] = crc_b[0];
      buf_out[7] = crc_b[1];
      ADU_Response(buf_out, 8);
    }
    else
    {
      buf_out[0] = 0x80 + writeCoilFunc;
      buf_out[1] = 0x04;
      ADU_Response(buf_out, 2);
    }
  }

  else if (buf_in[1] == writeCoilFunc)
  {
    if (buf_in[2] == startingAddrCoil1[0] && buf_in[3] == startingAddrCoil1[1])
    { 
      if (buf_in[5] == valueCoil_on[1])
      {
        digitalWrite(blueLed, HIGH);
        delay(10);
      }
      if (buf_in[4] == valueCoil_off[0] && buf_in[5] == valueCoil_off[1])
      {
        digitalWrite(blueLed, LOW);
        delay(10);
      }
    }
    else if (buf_in[2] == startingAddrCoil2[0] && buf_in[3] == startingAddrCoil2[1])
    {
      if (buf_in[4] == valueCoil_on[0] && buf_in[5] == valueCoil_on[1])
      {
        digitalWrite(yellowLed, HIGH);
        delay(10);
      }
      if (buf_in[4] == valueCoil_off[0] && buf_in[5] == valueCoil_off[1])
      {
        digitalWrite(yellowLed, LOW);
        delay(10);
      }
    }
    else 
    {
      buf_out[0] = 0x80 + writeCoilFunc;
      buf_out[1] = 0x02;
      ADU_Response(buf_out, 2);
      // return;
    }

    buf_out[0] = buf_in[0];
    buf_out[1] = buf_in[1];
    buf_out[2] = buf_in[2];
    buf_out[3] = buf_in[3];
    buf_out[4] = buf_in[4];
    buf_out[5] = buf_in[5];
    buf_out[6] = buf_in[6];
    buf_out[7] = buf_in[7];

    uint16_t crc_u = 0;
    crc_u = crc16(buf_in, 6);
    byte crc_b[2] = {crc_u & 0xFF, (crc_u>>8) & 0xFF};
    
    if (crc_b[0] == buf_in[6] && crc_b[1] == buf_in[7])
    {
      ADU_Response(buf_out, 8);
    }
    else
    {
      buf_out[0] = 0x80 + writeCoilFunc;
      buf_out[1] = 0x04;
      ADU_Response(buf_out, 2);
    }
  }

  else 
  {
    buf_out[0] = 0x80 + writeCoilFunc;
    buf_out[1] = 0x01;
    ADU_Response(buf_out, 2);
    return;
  }
}

void ADU_Response(byte msg[], int len)
{
  SerialPort.write(msg, len);
  SerialPort.write('\n');

  // Serial.print("buf_out: ");  
  // Serial.printf("%.2X", msg[0]);  
  // Serial.printf("%.2X", msg[1]);  
  // Serial.printf("%.2X", msg[2]);  
  // Serial.printf("%.2X", msg[3]);  
  // Serial.printf("%.2X", msg[4]);  
  // Serial.printf("%.2X", msg[5]);  
  // Serial.printf("%.2X", msg[6]);   
  // Serial.printf("%.2X", msg[7]);   

  // Serial.println();
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
      else{
        crc <<= 1;
      }
    }
  }
  return crc;
}




