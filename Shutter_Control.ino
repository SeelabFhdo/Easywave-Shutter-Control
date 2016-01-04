// Marcel Zillekens
// 2.11.2015
// Steuerung der drei Rolladen im SEELab via openHAB (REST interface)

#include <Wire.h>
#include <SPI.h>
#include <Ethernet.h>
#include <aREST.h>

// Debug mode
// #define DEBUG

// Bit masks
#define BIT_0  0x01  // 0000 0001
#define BIT_1  0x02  // 0000 0010
#define BIT_2  0x04  // 0000 0100
#define BIT_3  0x08  // 0000 1000
#define BIT_4  0x10  // 0001 0000
#define BIT_5  0x20  // 0010 0000
#define BIT_6  0x40  // 0100 0000
#define BIT_7  0x80  // 1000 0000

// Transceiver module I2C slave address
#define	I2C_TRANSCEIVER_ADDR  0x62

// Transceiver module register addresses
#define	TX_STATUS      0x10
#define TX_CHANNEL     0x11
#define TX_BUTTON      0x12
#define TX_ACK         0x13

// Transceiver module flag bits
#define TX_TRANSMIT_FLAG_BIT  BIT_7  // 1000 0000

// Shutter configuration
#define SHUTTER_LEFT_CHANNEL    0
#define SHUTTER_MIDDLE_CHANNEL  1
#define SHUTTER_RIGHT_CHANNEL   2

#define SHUTTER_UP    0
#define SHUTTER_DOWN  1
#define SHUTTER_STOP  2

byte mac[] = { 0x90, 0xA2, 0xDA, 0x0F, 0x6B, 0xF8 };
// IPAddress ip(192, 168, 2, 2);
EthernetServer server(80);
aREST rest = aREST();

void setup() {
  delay(5000);
  Serial.begin(115200);      // Console output
  Wire.begin();              // I2C bus
  
  if(!Ethernet.begin(mac)) {
#ifdef DEBUG
    Serial.println("DHCP connection failed.");
#endif
  }
  
#ifdef DEBUG
  Serial.println(Ethernet.localIP());
#endif

  rest.function("shutter", control_shutter);
  server.begin(); 
}

void loop() {
  EthernetClient client = server.available();
  rest.handle(client);
}

int control_shutter(String command) {
  // Command format: [Shutter],[Action]
  // Shutter: {1, 2, 3} (1: left, 2: middle, 3: right)
  // Action: {0, 1 ,2} (0: up, 1: down, 2: stop)
  
#ifdef DEBUG
  Serial.println(command);
#endif
  
  if(command.length() == 3) {
    short shutter = (short)command.charAt(0) - 48;
    short action = (short)command.charAt(2) - 48;
    
    if(shutter == SHUTTER_LEFT_CHANNEL || shutter == SHUTTER_MIDDLE_CHANNEL || shutter == SHUTTER_RIGHT_CHANNEL) {
      if(action == SHUTTER_UP || action == SHUTTER_DOWN || action == SHUTTER_STOP) {
#ifdef DEBUG
        Serial.print("Shutter: "); Serial.print(shutter); Serial.print(", Action: "); Serial.println(action);
#endif
        transceiver_send_telegram(shutter, action);
        return 1;
      }
    }
  }
  
  return 0;
}

void transceiver_send_telegram(short channel, short button) {
  // Wait until the TX_TRANSMIT-bit in the TX_STATUS register is set to 0
  while((transceiver_read(TX_STATUS) & TX_TRANSMIT_FLAG_BIT) != 0) {
    delay(100);
  }
  
  // Set channel and button
  transceiver_write(TX_CHANNEL, channel);
  transceiver_write(TX_BUTTON, button);
  
  // Send telegram
  transceiver_write(TX_ACK, 0x00);
#ifdef DEBUG
  Serial.print("Telegram sent ");
  Serial.print("(Channel: ");
  Serial.print(channel);
  Serial.print(", Button: ");
  Serial.print(button);
  Serial.println(")");
#endif
}

// Function writes the given value into the given register of the transceiver module
void transceiver_write(byte reg, byte value) {
  
  char data[2] = {reg, value};
  
  Wire.beginTransmission(I2C_TRANSCEIVER_ADDR);
  Wire.write(data, sizeof(data));
  Wire.endTransmission();
}

// Function returns value of the given register
byte transceiver_read(byte reg) {
  // Set register address pointer
  Wire.beginTransmission(I2C_TRANSCEIVER_ADDR);
  Wire.write(reg);
  Wire.endTransmission();
  
  // Read specific register
  byte b;
  Wire.requestFrom(I2C_TRANSCEIVER_ADDR, 1);
  while(Wire.available()) {
    b = Wire.read();
    break;
  }
  return b;
}
