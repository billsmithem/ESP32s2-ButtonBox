#define DEBUG
#define SLOWDOWN          // add delay so I can see button presses on jstest
#if defined(SLOWDOWN)
const int delayMillis = 500;
#endif


#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include "FS.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <AsyncElegantOTA.h>
#include "USB.h"
#include "myUSBHIDGamepad.h"
#include "myUSBHIDKeyboard.h"
#include <Keypad.h>
#include "Adafruit_seesaw.h"      // rotaries
#include <seesaw_neopixel.h>

const char* ssid = "CPRAD1";
const char* password = "Elevated84wHite21worshiP";

IPAddress local_IP(10, 1, 10, 34);     // buttonbox in hosts
IPAddress gateway(10, 1, 10, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(1, 1, 1, 1);
IPAddress secondaryDNS(1, 0, 0, 1);

const char* input_parameter1 = "output";
const char* input_parameter2 = "state";

AsyncWebServer server(80);

String ledState;

// Replaces placeholder with LED state value
String processor(const String& var){
  Serial.println(var);
  // if(var == "STATE"){
  //   if(digitalRead(ledPin)){
  //     ledState = "ON";
  //   }
  //   else{
  //     ledState = "OFF";
  //   }
  //   Serial.print(ledState);
  //   return ledState;
  // }
  ledState = "unused???";
  return String();
}

String outputState(int output){
  if(digitalRead(output)){
    return "checked";
  }
  else {
    return "";
  }
}


// rotary encoders
#define SS_SWITCH        24      // this is the pin on the encoder connected to switch
#define SS_NEOPIX        6       // this is the pin on the encoder connected to neopixel

#define SEESAW_BASE_ADDR          0x36  // I2C address, starts with 0x36


// create 6 encoders!
#define NUM_ENCODERS  6
Adafruit_seesaw encoders[NUM_ENCODERS];
// create 6 encoder pixels
seesaw_NeoPixel encoder_pixels[NUM_ENCODERS] = {
  seesaw_NeoPixel(1, SS_NEOPIX, NEO_GRB + NEO_KHZ800),
  seesaw_NeoPixel(1, SS_NEOPIX, NEO_GRB + NEO_KHZ800),
  seesaw_NeoPixel(1, SS_NEOPIX, NEO_GRB + NEO_KHZ800),
  seesaw_NeoPixel(1, SS_NEOPIX, NEO_GRB + NEO_KHZ800),
  seesaw_NeoPixel(1, SS_NEOPIX, NEO_GRB + NEO_KHZ800),
  seesaw_NeoPixel(1, SS_NEOPIX, NEO_GRB + NEO_KHZ800)};

int32_t encoder_positions[] = {0, 0, 0, 0, 0, 0};
bool found_encoders[] = {false, false, false, false, false, false};

// keypad
const byte ROWS = 2;
const byte COLS = 6;
const int encoderOffset = ROWS * COLS;    // encoder "buttons" start after keypad
char keys[ROWS][COLS] = {
  { 1, 2, 3, 4, 5, 6 },
  { 7, 8, 9, 10, 11, 12 },
  // { 13, 14, 15, 16, 17, 18 }
};

byte rowPins[ROWS] = { 5, 6, /*12*/ };                      // row pinouts of the keypad
byte colPins[COLS] = { 10, 7, 3, 1, 38, 33 };  // column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);


myUSBHIDGamepad Gamepad;
USBHIDKeyboard Keyboard;

typedef struct {
  int buttonVal;
  bool isKey;
  char key;
  bool control;
  bool shift;
  bool alt;
} Button;

// default values for buttons
Button buttons[] = {
  // first row SPDT toggles
  { 0, false, 0, false, false, false },
  { 1, false, 0, false, false, false },
  { 2, false, 0, false, false, false },
  { 3, false, 0, false, false, false },
  { 4, false, 0, false, false, false },
  { 5, false, 0, false, false, false },
  { 6, false, 0, false, false, false },
  { 7, false, 0, false, false, false },
  { 8, false, 0, false, false, false },
  { 9, false, 0, false, false, false },
  { 10, false, 0, false, false, false },
  { 11, false, 0, false, false, false },
  // Second row push buttons
  { 12, false, 0, false, false, false },
  { 13, false, 0, false, false, false },
  { 14, true, 'A', false, false, false },     // testing - set back to button when done
  { 15, false, 0, false, false, false },
  { 16, false, 0, false, false, false },
  { 17, false, 0, false, false, false },
  // Third row push buttons
  { 18, false, 0, false, false, false },
  { 19, false, 0, false, false, false },
  { 20, false, 0, false, false, false },
  { 21, false, 0, false, false, false },
  { 22, false, 0, false, false, false },
  { 23, false, 0, false, false, false },
  // rotaries start here
  // one
  { 24, false, 0, false, false, false },    // CCW
  { 25, false, 0, false, false, false },    // CW
  { 26, false, 0, false, false, false },    // push button
  // two
  { 27, false, 0, false, false, false },    // CCW
  { 28, false, 0, false, false, false },    // CW
  { 29, false, 0, false, false, false },    // push button
  // three
  { 30, false, 0, false, false, false },    // CCW
  { 31, false, 0, false, false, false },    // CW
  { 32, false, 0, false, false, false },    // push button
  // four
  { 33, false, 0, false, false, false },    // CCW
  { 34, false, 0, false, false, false },    // CW
  { 35, false, 0, false, false, false },    // push button
  // five
  { 36, false, 0, false, false, false },    // CCW
  { 37, false, 0, false, false, false },    // CW
  { 38, false, 0, false, false, false },    // push button
  // six
  { 39, false, 0, false, false, false },    // CCW
  { 40, false, 0, false, false, false },    // CW
  { 41, false, 0, false, false, false },    // push button
};

void sendButton(unsigned int buttonIndex) {
  #if defined(DEBUG)
  Serial.print("sendButton: ");
  Serial.println(buttonIndex);
  Serial.println("Pressing button");
  #endif
  if (buttons[buttonIndex].isKey == false) {
    Gamepad.pressButton(buttons[buttonIndex].buttonVal);
    #if defined(SLOWDOWN)
    delay(delayMillis);
    #endif
    Gamepad.releaseButton(buttons[buttonIndex].buttonVal);
  }
  else {      // it's a key
    Button button = buttons[buttonIndex];
    // Keyboard.press(buttons[buttonIndex].key);
    if (button.control)
      Keyboard.press(KEY_LEFT_CTRL);
    if (button.shift)
      Keyboard.press(KEY_LEFT_SHIFT);
    if (button.alt)
      Keyboard.press(KEY_LEFT_ALT);
    Keyboard.press(button.key);
    delay(100);
    Keyboard.releaseAll();
  }
}

void initLittleFS() {
  if (!LittleFS.begin(true)) {
    Serial.println("Error mounting LittleFS");
  } else {
    Serial.println("LittleFS mounted");
  }
}

void initWiFi() {
  // configure static IP address
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA failed to configure");
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void doEncoders() {
  int button;

  for (uint8_t enc = 0; enc < sizeof(found_encoders); enc++) { 
    if (found_encoders[enc] == false) continue;

    int32_t new_position = encoders[enc].getEncoderPosition();
    // did we move around?
    if (encoder_positions[enc] != new_position) {
      int32_t change = new_position - encoder_positions[enc];
      // map encoders such that cw is base, ccw is base + 1, pushbutton is base +2
      if (change > 0)         // which way did he go...
        button = enc + encoderOffset;
      else
        button = enc + encoderOffset + 1;
      // send press for each step
      for (int count = 0; count < abs(change); count++)
        sendButton(button);

      #if defined(DEBUG)
      Serial.print("Encoder #");
      Serial.print(enc);
      Serial.print(" -> ");
      Serial.println(new_position);         // display new position
      #endif
      encoder_positions[enc] = new_position;

      // change the neopixel color, mulitply the new positiion by 4 to speed it up
      encoder_pixels[enc].setPixelColor(0, Wheel((new_position*4) & 0xFF));
      encoder_pixels[enc].show();
    }

    if (! encoders[enc].digitalRead(SS_SWITCH)) {
      #if defined(DEBUG)
      Serial.print("Encoder #");
      Serial.print(enc);
      Serial.println(" pressed");
      #endif
      sendButton(enc + encoderOffset + 2);    // pushbutton is base + 2
    }
  }
}

void initEncoders() {
  for (uint8_t enc = 0; enc < sizeof(found_encoders); enc++) {
    // See if we can find encoders on this address 
    if (! encoders[enc].begin(SEESAW_BASE_ADDR + enc) ||
        ! encoder_pixels[enc].begin(SEESAW_BASE_ADDR + enc)) {
      Serial.print("Couldn't find encoder #");
      Serial.println(enc);
    } else {
      Serial.print("Found encoder + pixel #");
      Serial.println(enc);
      
      uint32_t version = ((encoders[enc].getVersion() >> 16) & 0xFFFF);
      if (version != 4991){
        Serial.print("Wrong firmware loaded? ");
        Serial.println(version);
        while(1) delay(10);
      }
      Serial.println("Found Product 4991");
  
      // use a pin for the built in encoder switch
      encoders[enc].pinMode(SS_SWITCH, INPUT_PULLUP);

      // get starting position
      encoder_positions[enc] = encoders[enc].getEncoderPosition();
  
      Serial.println("Turning on interrupts");
      delay(10);
      encoders[enc].setGPIOInterrupts((uint32_t)1 << SS_SWITCH, 1);
      encoders[enc].enableEncoderInterrupt();
      
      // set not so bright!
      encoder_pixels[enc].setBrightness(30);
      encoder_pixels[enc].show();

      found_encoders[enc] = true;
    }
  }
 
  Serial.println("Encoders started");
}

void setup() {
  Serial.begin();
  delay(500); //let things init
  Serial.println("Booting...");

  initLittleFS();

  initWiFi();

  AsyncElegantOTA.begin(&server);    // Start ElegantOTA

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/index.html", String(), false, processor);
  });

  // route for style.css
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(LittleFS, "/style.css", "text/css");
  });

  // Send a GET request to <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
  server.on("/update", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage1;
    String inputMessage2;
    // GET input1 value on <ESP_IP>/update?output=<inputMessage1>&state=<inputMessage2>
    if (request->hasParam(input_parameter1) && request->hasParam(input_parameter2)) {
      inputMessage1 = request->getParam(input_parameter1)->value();
      inputMessage2 = request->getParam(input_parameter2)->value();
      // digitalWrite(inputMessage1.toInt(), inputMessage2.toInt());
    }
    else {
      inputMessage1 = "No message sent";
      inputMessage2 = "No message sent";
    }
    Serial.print("GPIO: ");
    Serial.print(inputMessage1);
    Serial.print(" - Set to: ");
    Serial.println(inputMessage2);
    request->send(200, "text/plain", "OK");
  });

  server.begin();
  Serial.println("HTTP server started");


  Gamepad.begin();
  Keyboard.begin();
  USB.begin();

  #if defined(DEBUG)
  Serial.println("Looking for encoders");
  #endif

  initEncoders();
  // for (uint8_t enc = 0; enc < sizeof(found_encoders); enc++) {
  //   // See if we can find encoders on this address 
  //   if (! encoders[enc].begin(SEESAW_BASE_ADDR + enc) ||
  //       ! encoder_pixels[enc].begin(SEESAW_BASE_ADDR + enc)) {
  //     Serial.print("Couldn't find encoder #");
  //     Serial.println(enc);
  //   } else {
  //     Serial.print("Found encoder + pixel #");
  //     Serial.println(enc);
      
  //     uint32_t version = ((encoders[enc].getVersion() >> 16) & 0xFFFF);
  //     if (version != 4991){
  //       Serial.print("Wrong firmware loaded? ");
  //       Serial.println(version);
  //       while(1) delay(10);
  //     }
  //     Serial.println("Found Product 4991");
  
  //     // use a pin for the built in encoder switch
  //     encoders[enc].pinMode(SS_SWITCH, INPUT_PULLUP);

  //     // get starting position
  //     encoder_positions[enc] = encoders[enc].getEncoderPosition();
  
  //     Serial.println("Turning on interrupts");
  //     delay(10);
  //     encoders[enc].setGPIOInterrupts((uint32_t)1 << SS_SWITCH, 1);
  //     encoders[enc].enableEncoderInterrupt();
      
  //     // set not so bright!
  //     encoder_pixels[enc].setBrightness(30);
  //     encoder_pixels[enc].show();

  //     found_encoders[enc] = true;
  //   }
  // }
 
  // Serial.println("Encoders started");
}

void loop() {
  uint8_t key = keypad.getKey();

  if (key) {
    #if defined(DEBUG)
    Serial.println(key);
    #endif
    sendButton(key - 1);    // buttons are zero based
  }

  doEncoders();
  
  // don't overwhelm serial port
  yield();
  delay(10);
}


uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if (WheelPos < 85) {
    return seesaw_NeoPixel::Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if (WheelPos < 170) {
    WheelPos -= 85;
    return seesaw_NeoPixel::Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return seesaw_NeoPixel::Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}