/*********
  Control a circuit-test robotarm from a 8266 
  https://www.amazon.com/Circuit-Test-Robotic-Edge-Wired-Controller/dp/B008MONL8O
  Gösta Forsum
  based on sample from http://randomnerdtutorials.com  
*********/

// Load Wi-Fi library
#include <ESP8266WiFi.h>
#include <Ticker.h>

Ticker timeout;

void StopAllMotors();

class MotorControl{
private:
  String name;
  String openCaption;
  String closeCaption;
  int openPin;
  int closePin;
  enum state{
    OPEN,
    CLOSE
  } state;
  
public:
  MotorControl(String name,int openPin, int closePin, String openCaption, String closeCaption)
  :name(name),openPin(openPin),closePin(closePin),openCaption(openCaption),closeCaption(closeCaption),state(OPEN){}

  void setup(){
    pinMode(openPin, OUTPUT);
    pinMode(closePin, OUTPUT);
    digitalWrite(openPin, LOW);    
    digitalWrite(closePin, LOW);
  }

  void display(WiFiClient& client){
    client.println("<p>Motor "+name+"</p>");
    if (state==OPEN) {
      client.println("<p><a href=\"/"+name+"/open\"><button disabled class=\"button button2\">"+openCaption+"</button></a>"
                     "<a href=\"/"+name+"/close\"><button class=\"button\">"+closeCaption+"</button></a>"
                     "<a href=\"/"+name+"/action\"><button class=\"button\">Action</button></a></p>");
    } else {
      client.println("<p><a href=\"/"+name+"/open\"><button class=\"button\">"+openCaption+"</button></a>"
                     "<a href=\"/"+name+"/close\"><button disabled class=\"button button2\">"+closeCaption+"</button></a>"
                     "<a href=\"/"+name+"/action\"><button class=\"button\">Action</button></a></p>");
    }   
  }  

  void start(){
    StopAllMotors();
    if(state==OPEN){
      Serial.println(name+" is opening");
      digitalWrite(openPin, HIGH);
      digitalWrite(closePin, LOW);
    }
    else{
      Serial.println(name+" is closing");
      digitalWrite(openPin, LOW);
      digitalWrite(closePin, HIGH);      
    }
  }

  void stop(){
      digitalWrite(openPin, LOW);
      digitalWrite(closePin, LOW);    
  }
  
  void postAction(String header){
    if(header.indexOf("GET /"+name+"/")>=0){
      if(header.indexOf("GET /"+name+"/open")>=0){
        state=OPEN;
      }
      if(header.indexOf("GET /"+name+"/close")>=0){
        state=CLOSE;
      }
      if(header.indexOf("GET /"+name+"/action")>=0){
        start();
        timeout.attach(3, StopAllMotors);
      }      
    }
  }
};

MotorControl motorControls[]={MotorControl("shoulder",16,5,"Lift","Lower"), MotorControl("grip",9,4,"Open","Close"), MotorControl("wrist",10,14,"Raise","Lower"), MotorControl("elbow",12,13,"Lift","Lower"), MotorControl("rotate",0,3,"Clockwise","Counter clockwise")};
const int motorControlsLength=sizeof(motorControls)/sizeof(motorControls[0]);

void StopAllMotors(){
  for(int i=0;i<motorControlsLength;i++){
    motorControls[i].stop();
  }  
  timeout.detach();
}


// Replace with your network credentials
const char* ssid     = "fisknet";
const char* password = "sillsosse";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Auxiliar variables to store the current output state
String output2State = "off";

// Assign output variables to GPIO pins
const int output2 = 2;



// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

void setup() {
  Serial.begin(115200);
  // Initialize the output variables as outputs
  pinMode(output2, OUTPUT);
  // Set outputs to LOW
  digitalWrite(output2, LOW);

  for(int i=0;i<motorControlsLength;i++){
    motorControls[i].setup();
  }

  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void displayStopButton(WiFiClient& client){
  client.println("<p><a href=\"/control/stop\"><button class=\"button\">STOP!</button></a></p>");
}

void loop(){
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    currentTime = millis();
    previousTime = currentTime;
    while (client.connected() && currentTime - previousTime <= timeoutTime) { // loop while the client's connected
      currentTime = millis();         
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            if (header.indexOf("GET /control/stop") >= 0) {
              Serial.println("STOP!");
              StopAllMotors();
            }

            for(int i=0;i<motorControlsLength;i++){
              motorControls[i].postAction(header);
            }

            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons 
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #195B6A; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #77878A;}</style></head>");
            
            // Web Page Heading
            client.println("<body><h1>ROBOWars light</h1>");
                           
            displayStopButton(client);

            for(int i=0;i<motorControlsLength;i++){
              motorControls[i].display(client);
            }

            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
