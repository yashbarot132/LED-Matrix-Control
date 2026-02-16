#include "Arduino_LED_Matrix.h"
#include "WiFiS3.h"
#include "index.h"

ArduinoLEDMatrix matrix;

///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = "Itachi";        // your network SSID (name)
char pass[] = "Itachi003";    // your network password (use for WPA, or use as key for WEP)

int status = WL_IDLE_STATUS;
WiFiServer server(80);

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  matrix.begin();

  // checking for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }
  server.begin();
  printWifiStatus();
}

void loop() {
  WiFiClient client = server.available();   // listen for incoming clients

  if (client) {                             // if you get a client,
    Serial.println("New Client.");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    String requestBody = "";
    boolean readingBody = false;
    int contentLength = 0;
    boolean isPost = false;

    while (client.connected()) {            // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        // Serial.write(c);                    // print it out the serial monitor
        
        if (readingBody) {
            requestBody += c;
            if (requestBody.length() >= 96) { // Expecting 96 characters
                break;
            }
        } else {
            if (currentLine.startsWith("Content-Length: ")) {
                contentLength = currentLine.substring(16).toInt();
            }

            if (c == '\n') {                    // if the byte is a newline character
              // if the current line is blank, you got two newline characters in a row.
              // that's the end of the client HTTP request headers:
              if (currentLine.length() == 0) {
                // Check if we need to read body
                if (isPost) {
                     readingBody = true;
                } else {
                    break;
                }
                
                // If it was a GET request, we are done reading headers and have no body
                // But we need to know if it was GET or POST before.
                // Simplified logic: checking buffer in "process" or flag.
              } else {    // if you got a newline, then clear currentLine:
                if (currentLine.startsWith("GET / ")) {
                    sendHomePage(client);
                    readingBody = false; // No body for GET
                    break; 
                }
                if (currentLine.startsWith("POST /api/matrix")) {
                    isPost = true;
                }
                currentLine = "";
              }
            } else if (c != '\r') {  // if you got anything else but a carriage return character,
              currentLine += c;      // add it to the end of the currentLine
            }
        }
      }
    }
    
    // If we have a body, process it
    if (requestBody.length() > 0) {
        processMatrixData(requestBody);
        client.println("HTTP/1.1 200 OK");
        client.println("Content-type:text/plain");
        client.println("Connection: close");
        client.println();
        client.print("OK");
    }
    
    // close the connection:
    client.stop();
    Serial.println("Client Disconnected.");
  }
}

void sendHomePage(WiFiClient &client) {
    client.println("HTTP/1.1 200 OK");
    client.println("Content-type:text/html");
    client.println("Connection: close");
    client.println();
    client.print(index_html);
}

void processMatrixData(String data) {
    // ArduinoLEDMatrix renderBitmap takes uint8_t[8][12]
    // where each byte is a pixel (1 = on, 0 = off).
    // This function handles the physical mapping internally.
    
    uint8_t frame[8][12];
    
    // Parse the linear string (96 chars) into the 8x12 grid
    // The string comes in row-major order:
    // Row 0: chars 0-11
    // Row 1: chars 12-23
    // ...
    
    for (int i = 0; i < 96; i++) {
        int row = i / 12;
        int col = i % 12;
        
        if (i < data.length() && data[i] == '1') {
            frame[row][col] = 1;
        } else {
            frame[row][col] = 0;
        }
    }
    
    matrix.renderBitmap(frame, 8, 12);
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}