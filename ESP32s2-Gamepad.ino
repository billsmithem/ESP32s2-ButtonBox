#define DEBUG
#define SLOWDOWN          // add delay so I can see button presses on jstest

#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>

const char* ssid = "CPRAD1";
const char* password = "Elevated84wHite21worshiP";

AsyncWebServer server(80);
IPAddress local_IP(10, 1, 10, 34);     // buttonbox in hosts
IPAddress gateway(10, 1, 10, 1);
IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(1, 1, 1, 1);
IPAddress secondaryDNS(1, 0, 0, 1);


#include "USB.h"
#include "myUSBHIDGamepad.h"

#include <Keypad.h>

// rotaries
#include "Adafruit_seesaw.h"
#include <seesaw_neopixel.h>

#define SS_SWITCH        24      // this is the pin on the encoder connected to switch
#define SS_NEOPIX        6       // this is the pin on the encoder connected to neopixel

#define SEESAW_BASE_ADDR          0x36  // I2C address, starts with 0x36


// create 6 encoders!
#define NUM_ENCODERS  6
Adafruit_seesaw encoders[NUM_ENCODERS];
// create 4 encoder pixels
seesaw_NeoPixel encoder_pixels[NUM_ENCODERS] = {
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

// init USB Gamepad
myUSBHIDGamepad Gamepad;

#if defined(SLOWDOWN)
const int delayMillis = 500;
#endif

void sendButton(unsigned int button) {
  #if defined(DEBUG)
  Serial.print("sendButton: ");
  Serial.println(button);
  Serial.println("Pressing button");
  #endif
  Gamepad.pressButton(button);
  #if defined(SLOWDOWN)
  delay(delayMillis);
  #endif
  Gamepad.releaseButton(button);
}

void setup() {
  Serial.begin();
  delay(500); //let things init

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

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Hi! I am ESP32.");
  });

  AsyncElegantOTA.begin(&server);    // Start ElegantOTA
  server.begin();
  Serial.println("HTTP server started");


  Gamepad.begin();
  USB.begin();

  #if defined(DEBUG)
  Serial.println("Looking for encoders");
  #endif

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

void loop() {
  uint8_t key = keypad.getKey();

  if (key) {
    #if defined(DEBUG)
    Serial.println(key);
    #endif
    sendButton(key - 1);    // buttons are zero based
  }

  for (uint8_t enc = 0; enc < sizeof(found_encoders); enc++) { 
    if (found_encoders[enc] == false) continue;

    int32_t new_position = encoders[enc].getEncoderPosition();
    // did we move around?
    if (encoder_positions[enc] != new_position) {
      int32_t change = new_position - encoder_positions[enc];
      // initially I'm going to assume that change is never more than one
      // map encoders such that cw is base, ccw is base + 1, pushbutton is base +2
      if (change > 0)
        sendButton(enc + encoderOffset);
      else
        sendButton(enc + encoderOffset + 1);

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