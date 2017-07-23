/*
 * ERM Control box and game win trigger
 * By Matt Simmons, Operation: Escape Room
 * Hacked together using example code from Nate Shane/Escape Room Master
 */

// Include Ethernet and deBounce Library
#include <Ethernet.h>
#include <Bounce2.h>

// Setup Pins
#define START_PIN 2
#define WIN_PIN 8
#define RESET_PIN 9
int gamepin = 3;
int readypin = 7; 
int wonlight = 5; 

// Initiate deBounce
Bounce debouncer1 = Bounce(); 

bool useDHCP = false;

// Enter a MAC address and IP address for your controller below.
// The IP address will be dependent on your local network:
byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xEF
};
IPAddress ip(10, 1, 10, 42);

// Initialize the Ethernet server library
// with the IP address and port you want to use
// (port 80 is default for HTTP):
EthernetServer server(80);

// Setup Room State Variables
bool gameStarted = false; //has the game been started?
bool gameWon = false;  // has the game been won?
bool gameFailed = false; // has the game been failed?

// Setup Variables for loss blink
long previousMillis = 0;  
long interval = 500;
int ledState = LOW; 



void setup() {
  // start serial connection (debugging)
  Serial.begin(9600);

  // NOTE: pin 1 & 2 are used for Serial
  // 4 & 10 - 13 are used for ethernet

// Sets Pin Modes
  pinMode(START_PIN, INPUT_PULLUP);   // Start Game Button
  pinMode(WIN_PIN, INPUT_PULLUP);   // Game Win Button
  pinMode(RESET_PIN, INPUT_PULLUP);   // Game Win Button
  pinMode(gamepin, OUTPUT);
  pinMode(readypin, OUTPUT);
  pinMode(wonlight, OUTPUT);

// Attaches Win Pin to debouncer variable
  debouncer1.attach(WIN_PIN);
  debouncer1.interval(300);

  // start the Ethernet connection and the server:
  if (useDHCP) {
    Serial.println("Using DHCP");
    Ethernet.begin(mac); // Use DHCP instead of ip
  }
  else {
    Serial.println("Not using DHCP");
    Ethernet.begin(mac, ip);
  }
  server.begin();
  Serial.print("server is at ");
  Serial.println(Ethernet.localIP());


  // Initial game state
  gameStarted = false;
  gameWon = false;
  gameFailed = false;
}



void loop() { 
  //read the button value into a variable
  int startBtnVal = digitalRead(START_PIN);
  int resetBtnVal = digitalRead(RESET_PIN);
  
// Poll deBounced win pin and read to variable wonbounce 
  debouncer1.update();
  int wonbounce = debouncer1.read();

// Monitor for Start Button and call startgame function.  Ready light MUST be on
  if (startBtnVal == LOW && digitalRead(readypin) == HIGH) {
    startgame();
  }

// Monitor for signal from FX50 to show final prop has been found and call wongame function.  Game must be running.
  if (gameStarted == true && wonbounce == LOW) {
    wongame();
  }

// Monitor for reset button to reset all variables back to initial values.  Reset button and start button must be pressed at same time.
  if (resetBtnVal == LOW && startBtnVal== LOW) {
    reset();
  }

// Monitor for game failure & blink game pin on fail
  if (gameStarted == true && gameWon == false && gameFailed == true) {
     unsigned long currentMillis = millis();
 
  if(currentMillis - previousMillis > interval) {
    // save the last time you blinked the LED 
    previousMillis = currentMillis;   
 
    // if the LED is off turn it on and vice-versa:
    if (ledState == LOW)
      ledState = HIGH;
    else
      ledState = LOW;
 
    // set the LED with the ledState of the variable:
    digitalWrite(3, ledState);
  }
  }

// Monitors variables to ensure game conditions are reset and that the final prop is placed correctly before illuminating the ready indicatior.
  if (gameStarted == false && gameWon == false && gameFailed == false && wonbounce == HIGH) {
  digitalWrite(readypin, HIGH);
  digitalWrite(gamepin, LOW);
  digitalWrite(wonlight, LOW);
  }
  else {
  digitalWrite(readypin, LOW);
  }


  // listen for incoming Ethernet connections:
  listenForEthernetClients();

  // Maintain DHCP lease
  if (useDHCP) {
    Ethernet.maintain();
  }


  // Use serial for debugging to bypass web server
  if (Serial.available()) {
    //read serial as a character
    char ser = Serial.read();

    switch (ser) {
      case 'r': // reset prop
        reset(); 
        break;

      case 's': // button press
        startgame();
        break;  

       case 'w': // button press
        wongame();
        break;  
    }
  }
}


/*
 * Game states
 */


// Check if game has been started and if not, set gameStarted variable to true.
void startgame() {
  if (gameStarted) {
    return;
  }
  
  gameStarted = true;
}

// Check if game has been won and if not, set gameWon variable to true
void wongame() {
  if (gameWon) {
    return;
  }
  
  gameWon = true;
}

// Reset the controller
void reset() {
  gameStarted = false;
  gameWon = false;
  gameFailed = false;
  digitalWrite(wonlight, LOW);
}


/*
 * URL routing for prop commands and polling
 */

// This function dictates what text is returned when the prop is polled over the network
String startString() {
  return gameStarted ? "started" : "not started";
}

String wonString() {
  return gameWon ? "won" : "not won";
}

// Actual request handler
void processRequest(EthernetClient& client, String requestStr) {
  Serial.println(requestStr);

  //ERM Polls this for game started status
  if (requestStr.startsWith("GET /started")) {
    Serial.println("polled for status!");
    writeClientResponse(client, startString());
  //ERM Polls this for game won status
  } else if (requestStr.startsWith("GET /won")) {
    Serial.println("polled for won!");
    writeClientResponse(client, wonString());
  //Reset call from ERM
  } else if (requestStr.startsWith("GET /reset")) {
    Serial.println("Room reset");
    reset();
    writeClientResponse(client, "Room Reset");
  //Game failed call from ERM  
  } else if (requestStr.startsWith("GET /gamefailed")) {
    Serial.println("Room failed");
    gameFailed = true;
    writeClientResponse(client, "Room Failed");
  //Game started confirmation from ERM.  Only way the gamestarted LED can be lit.
  } else if (requestStr.startsWith("GET /confirmstart")) {
    gameStarted = true;
    digitalWrite(gamepin, HIGH);
    writeClientResponse(client, "Start Confirmed");
  } 
  //Room Completion call from ERM.
    else if (requestStr.startsWith("GET /roomwon")) {
    gameWon = true;
    digitalWrite(gamepin, LOW);
    digitalWrite(wonlight, HIGH);
    writeClientResponse(client, "Room Won Confirmed");
  }else {
    writeClientResponseNotFound(client);
  }
}


/*
 * HTTP helper functions
 */

void listenForEthernetClients() {
  // listen for incoming clients
  EthernetClient client = server.available();
  if (client) {
    Serial.println("Got a client");
    // Grab the first HTTP header (GET /status HTTP/1.1)
    String requestStr;
    boolean firstLine = true;
    // an http request ends with a blank line
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        // if you've gotten to the end of the line (received a newline
        // character) and the line is blank, the http request has ended,
        // so you can send a reply
        if (c == '\n' && currentLineIsBlank) {
          processRequest(client, requestStr);
          break;
        }
        if (c == '\n') {
          // you're starting a new line
          currentLineIsBlank = true;
          firstLine = false;
        } else if (c != '\r') {
          // you've gotten a character on the current line
          currentLineIsBlank = false;

          if (firstLine) {
            requestStr.concat(c);
          }
        }
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
  }
}


void writeClientResponse(EthernetClient& client, String bodyStr) {
  // send a standard http response header
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/plain");
  client.println("Access-Control-Allow-Origin: *");  // ERM will not be able to connect without this header!
  client.println();
  client.print(bodyStr);
}


void writeClientResponseNotFound(EthernetClient& client) {
  // send a standard http response header
  client.println("HTTP/1.1 404 Not Found");
  client.println("Access-Control-Allow-Origin: *");  // ERM will not be able to connect without this header!
  client.println();
}
