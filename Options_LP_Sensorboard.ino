/*
 * Sketch to sample analog sensors (pressure sensors) and output the readings in a UDP packet over broadcast.
 * To be used on Arduino Nano with Arduino MKR ETH Shield.
 * 
 * A0 - NXP Semiconductors MPX5100 DP. Differential pressure, ambient and sensor housing (0-1 bar)
 * A1 - Panasonic ADP5141. Gauge pressure (relative to sensor housing) of compensator 1 (0-1 bar)
 * A2 - Panasonic ADP5141. Gauge pressure (relative to sensor housing) of compensator 2 (0-1 bar)
 * A3 - Panasonic ADP5141. Gauge pressure (relative to sensor housing) of compensator 3 (0-1 bar)
 * A4 - Panasonic ADP5141. Gauge pressure (relative to sensor housing) of compensator 4 (0-1 bar)
 * A5 - Panasonic ADP5141. Gauge pressure (relative to sensor housing) of compensator 5 (0-1 bar)
 * A6 - Panasonic ADP5141. Gauge pressure (relative to sensor housing) of compensator 6 (0-1 bar)
 * A7 - Panasonic ADP5141. Gauge pressure (relative to sensor housing) of compensator 7 (0-1 bar)
 * 
 * 
 * Wiring:
 * 
 * Nano | MKR ETH   | Descr
 * D10  | Pin 5     | SS/CS, slave select. 
 * D11  | Pin 8     | MOSI
 * D12  | Pin 10    | MISO
 * D13  | Pin 9     | SCK
 * 5V   | 5V        | Power Input (5V)
 * GND  | GND       | GND
 * 
 * A0   | MPX5100   | Ambient pressure
 * A1   | ADP5141 1 | Comp 1 pressure
 * A2   | ADP5141 2 | Comp 2 pressure
 * A3   | ADP5141 3 | Comp 3 pressure
 * A4   | ADP5141 4 | Comp 4 pressure
 * A5   | ADP5141 5 | Comp 5 pressure
 * A6   | ADP5141 6 | Comp 6 pressure
 * A7   | ADP5141 7 | Comp 7 pressure
 * 
 * 
 * 
 * No LEDs on MKR ETH, when powered and patch cable is connected green LED on RJ45 will light up.
 * 
 * Created 10.12.2018
 * by Lars Kambe Fjæra, Options AS
 */

#include <SPI.h>
#include <Ethernet.h>

// Prototypes
void debug(String str);
void debugln(String str);
void debug(int str);
void debugln(int str);
void udpReceive(int PacketSize);
void udpBroadcast();

// MAC address needs to be unique on network. Keep log of all MAC addresses assigned.
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

IPAddress ip(192, 168,1,111);
IPAddress broadcastIp(192, 168, 1, 255);

// Select port to be used for udp messages
int port = 5500;

// Serial debug output, on off
int DEBUG = 0;

// Buffers for sending and receiving UDP packets.
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];
char ReplyBuffer[] = "acknowledged";
char sensorBuffer[256];

// An Ethernet UDP instace enables sending and receiving packets over UDP
EthernetUDP udp;

// Initialize the Ethernet server library
// with the IP address and port to be used. 
// Use port 80 for HTTP
EthernetServer server(80);


void setup() {
  // Output pin for Ethernet module SS SPI
  // This is the output of the arduino controller, not the SS pin on the ETH card.
  Ethernet.init(10);

  // Start the ethernnet
  Ethernet.begin(mac, ip);

  // Open serial communications for debug
  Serial.begin(9600);
  while(!Serial);

  // Internal LED used for error codes
  pinMode(LED_BUILTIN, OUTPUT);
  int led = 0;
  // Check for ethernet hardware, blink internal LED once per second if no HW
  do{
    delay(1000); // do nothing if HW is not connected. 
    digitalWrite(LED_BUILTIN, !led);
  }while(Ethernet.hardwareStatus() == EthernetNoHardware);

  // Check if cable is connected to ethernet card
  do{
    delay(2000); // do nothing if ethernet cable is not connected. 
    digitalWrite(LED_BUILTIN, !led);
  }while(Ethernet.linkStatus() == LinkOFF);

  // Start UDP
  udp.begin(port); 

  // Start webserver
  server.begin();
  debug("Server is at ");
  debugln(Ethernet.localIP());
}

void loop() {
  // UDP send-receive
  // Read packet, if available
  int packetSize = udp.parsePacket();
  if (packetSize){
      udpReceive(packetSize);
      // Switch-case statement for received package. Calibration renaming etc.
      // For future use.

      // Send reply to the IP address and port the packet was received from
      // For future use. Send acknowledge message based on received command type.
      // integrate in switch statement.
      udp.beginPacket(udp.remoteIP(), udp.remotePort());
      udp.write(ReplyBuffer);
      udp.endPacket();
      delay(10);
    }

    // UDP Broadcast
    udpBroadcast();
    

  // SERVER
  //listen for incoming clients
  EthernetClient client = server.available();
  if(client) {
    debugln("New client");
    bool currentLineIsBlank = true; // HTTP request ends with blank line
    while(client.connected()) {
      if  (client.available()) {
        char c = client.read();
        Serial.write(c);
        // HTTP request has ended when newline charactes is received and new line is blank.
        if (c == '\n' && currentLineIsBlank){
          debugln("Sending response");
          httpResponse(client); 
          break;        
        }

        if (c == '\n') {
          currentLineIsBlank = true; // Starting new line
        } else if (c != '\r') {
          currentLineIsBlank = false; // received character on current line.
        }
    
      }
    }
    // Pause to let browser receive data
    delay(1);
    // Close connection
    client.stop();
    debugln("client disconnected");
  }
}


void httpResponse(EthernetClient client){
  // Send a standard HTTP response header
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println("Connection: close"); // connection will be cloesd after completion of response
  client.println("Refresh: 1"); // Refresh page automatically every second
  client.println();
  client.println("<!DOCTYPE HTML>");
  client.println("<html>");

  // Output value of each analog input pin
  for (int analogChannel = 0; analogChannel < 8; analogChannel++){
    int sensorReading = analogRead(analogChannel);
    client.print("analog input ");
    client.print(analogChannel);
    client.print(" : ");
    client.print(sensorReading);
    client.println("<br />");
  }
  client.println("</html>");
}

void udpReceive(int packetSize){
  debugln("Received packet of size ");
  debugln(packetSize);
  debug("From ");
  IPAddress remote = udp.remoteIP();
  for (int i=0; i < 4; i++){
    debug(remote[i]);
    if ( i < 3){
      debugln(".");
    }
  }
  debug(", port ");
  debugln(udp.remotePort());

  // Read packetinto packetBuffer
  udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
  debugln("Contents: ");
  debugln(packetBuffer);
}

void udpBroadcast(){
  udp.beginPacket(broadcastIp, port);

  memcpy(sensorBuffer, 0, 256);
  char reading[8];
  int index = 0;
  for (int input = 0; input < 8; input++) {
    int val = analogRead(input);
    int str = sprintf(reading, "A%d:%d-",input, val);
    memcpy(sensorBuffer + (input*7), reading, strlen(reading));
  }
  memcpy(sensorBuffer + 55, '\n', 1);
  debugln(sensorBuffer);
  delay(1000);
  udp.write(sensorBuffer);
  udp.endPacket();
}

void debug(String str){
  if (DEBUG) {
    Serial.print(str);
  }
}

void debugln(String str) {
  if (DEBUG){
    Serial.println(str);
  }
}

void debug(int str){
  if (DEBUG) {
    Serial.print(str);
  }
}

void debugln(int str) {
  if (DEBUG){
    Serial.println(str);
  }
}
