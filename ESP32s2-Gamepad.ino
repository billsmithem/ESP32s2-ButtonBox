#define DEBUG
// #define SLOWDOWN          // add delay so I can see button presses on jstest
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

#include "types.h"          // get around weird arduino pre-processor

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

bool ssSwitchState[] { true, true, true, true, true, true};

encModal encoderMode[] = {
  { true, false },
  { true, false },
  { true, false },
  { true, false },
  { true, false },
  { true, false }
};

const int shiftSize = 18;

// keypad
const byte ROWS = 4;
const byte COLS = 6;
const int encoderOffset = ROWS * COLS;    // encoder "buttons" start after keypad
char bbkeys[ROWS][COLS] = {
  { 7, 8, 9, 10, 11, 12 },
  { 1, 2, 3, 4, 5, 6 },
  { 13, 14, 15, 16, 17, 18 },
  { 19, 20, 21, 22, 23, 24 }
};

byte rowPins[ROWS] = { 5, 6, 12, 14 };                      // row pinouts of the keypad
byte colPins[COLS] = { 10, 7, 3, 1, 38, 33 };  // column pinouts of the keypad

Keypad keypad = Keypad(makeKeymap(bbkeys), rowPins, colPins, ROWS, COLS);


myUSBHIDGamepad Gamepad;
USBHIDKeyboard Keyboard;

Config prefs = {
  "default.layout"
};

// default values for buttons
// Button buttons[42];

Button buttons[] = {
  // first row SPDT toggles
  { 0, false, "", false, false, false },
  { 1, false, "", false, false, false },
  { 2, false, "", false, false, false },
  { 3, false, "", false, false, false },
  { 4, false, "", false, false, false },
  { 5, false, "", false, false, false },

  { 6, false, "", false, false, false },
  { 7, false, "", false, false, false },
  { 8, false, "", false, false, false },
  { 9, false, "", false, false, false },
  { 10, false, "", false, false, false },
  { 11, false, "", false, false, false },
  // Second row push buttons
  { 12, false, "", false, false, false },
  { 13, false, "", false, false, false },
  { 14, true, "A", false, false, false },     // testing - set back to button when done
  { 15, false, "", false, false, false },
  { 16, false, "", false, false, false },
  { 17, false, "", false, false, false },
  // Third row push buttons
  { 18, false, "", false, false, false },
  { 19, false, "", false, false, false },
  { 20, false, "", false, false, false },
  { 21, false, "", false, false, false },
  { 22, false, "", false, false, false },
  { 23, false, "", false, false, false },
  // rotaries start here
  // one
  { 24, false, "", false, false, false },    // CCW
  { 25, false, "", false, false, false },    // CW
  { 26, false, "", false, false, false },    // push button
  // two
  { 27, false, "", false, false, false },    // CCW
  { 28, false, "", false, false, false },    // CW
  { 29, false, "", false, false, false },    // push button
  // three
  { 30, false, "", false, false, false },    // CCW
  { 31, false, "", false, false, false },    // CW
  { 32, false, "", false, false, false },    // push button
  // four
  { 33, false, "", false, false, false },    // CCW
  { 34, false, "", false, false, false },    // CW
  { 35, false, "", false, false, false },    // push button
  // five
  { 36, false, "", false, false, false },    // CCW
  { 37, false, "", false, false, false },    // CW
  { 38, false, "", false, false, false },    // push button
  // six
  { 39, false, "", false, false, false },    // CCW
  { 40, false, "", false, false, false },    // CW
  { 41, false, "", false, false, false },    // push button

  // shifted rotaries start here
  // one
  { 42, false, "", false, false, false },    // CCW
  { 43, false, "", false, false, false },    // CW
  { 44, false, "", false, false, false },    // push button
  // two
  { 45, false, "", false, false, false },    // CCW
  { 46, false, "", false, false, false },    // CW
  { 47, false, "", false, false, false },    // push button
  // three
  { 48, false, "", false, false, false },    // CCW
  { 49, false, "", false, false, false },    // CW
  { 50, false, "", false, false, false },    // push button
  // four
  { 51, false, "", false, false, false },    // CCW
  { 52, false, "", false, false, false },    // CW
  { 53, false, "", false, false, false },    // push button
  // five
  { 54, false, "", false, false, false },    // CCW
  { 55, false, "", false, false, false },    // CW
  { 56, false, "", false, false, false },    // push button
  // six
  { 57, false, "", false, false, false },    // CCW
  { 58, false, "", false, false, false },    // CW
  { 59, false, "", false, false, false },    // push button
};


void sendButton(unsigned int buttonIndex, ButtonMode mode) {
  #if defined(DEBUG)
  Serial.print("sendButton: ");
  Serial.println(buttonIndex);
  switch (mode) {
    case On:
      Serial.println("Pressing button");
      break;

    case Off:
      Serial.println("Releasing button");
      break;

    case Pulse:
      Serial.println("Pulsing button");
      break;
  }
  #endif
  if (buttons[buttonIndex].isChars == false) {
    switch (mode)
    {
      case On:
        Gamepad.pressButton(buttons[buttonIndex].buttonVal);
        #if defined(SLOWDOWN)
        delay(delayMillis);
        #endif
        break;

      case Off:
        Gamepad.releaseButton(buttons[buttonIndex].buttonVal);
        break;

      case Pulse:
        Gamepad.pressButton(buttons[buttonIndex].buttonVal);
        delay(50);
        Gamepad.releaseButton(buttons[buttonIndex].buttonVal);
        break;
    }
  }
  else {      // it's keys
    switch (mode)
    {
      case On:
      {
        Button button = buttons[buttonIndex];
        // Keyboard.press(buttons[buttonIndex].keys);
        if (button.control)
          Keyboard.press(KEY_LEFT_CTRL);
        if (button.shift)
          Keyboard.press(KEY_LEFT_SHIFT);
        if (button.alt)
          Keyboard.press(KEY_LEFT_ALT);
        for (int index = 0; index < sizeof(Button().keys); index++ ){
          Keyboard.press(button.keys[index]);
          delay(10);
          Keyboard.release(button.keys[index]);
          delay(10);
        }
        // delay(100);
        Keyboard.releaseAll();
        break;
      }

      case Off:
        Keyboard.releaseAll();
        break;

      case Pulse:
        break;
    }
  }
}

void getConfig() {
  StaticJsonDocument<100> doc;

  // load current configuration
  File config = LittleFS.open("/config.json", "r");
  if (!config) {
    // file missing, write default
    Serial.println("Config not found");
    config = LittleFS.open("/config.json", "w");
    //    create serialized buffer
    doc["layout"] = "/default.layout";
    //    write buffer to file
    serializeJsonPretty(doc, config);
  }
  else {  // file found and open for reading
    // DeserializationError error = deserializeJson(doc, config.readString());
    DeserializationError error = deserializeJson(doc, config);
    // Serial.println((char *)doc["layout"]);
    if (error) {
      Serial.print("deserializeJson of config file failed:");
      Serial.println(error.f_str());
    }
    else {    // fetch name of layout file
      strncpy(prefs.layout, doc["layout"], sizeof(prefs.layout));
      Serial.println("prefs.layout = ");
      Serial.println(prefs.layout);
    }
  }
}

void getLayout(String file = prefs.layout) {
  Serial.println("Opening:");
  Serial.println(file);

  // load current configuration
  File lof = LittleFS.open(file, "r");
  if (!lof) {
    // file missing, write default
    Serial.println("Config not found");
  }
  else {  // file found and open for reading
    DynamicJsonDocument layout(6200);
    DeserializationError error = deserializeJson(layout, lof);

    JsonArray arr = layout.as<JsonArray>();
    int count = arr.size();
    if (count == 42) {
        Button* bPtr = buttons;
        for (JsonObject repo : arr) {
          bPtr->buttonVal = repo["buttonVal"];
          bPtr->isChars = repo["isChars"];
          strncpy(bPtr->keys, repo["keys"], sizeof(Button().keys));
          bPtr->control = repo["control"];
          bPtr->shift = repo["shift"];
          bPtr->alt = repo["alt"];
          bPtr++;
        }
    }
    else {    // houston, we have a problem
      Serial.println("layout array wrong size");
      Serial.println(count);
    }

    for (int index = 0; index < 42; index++) {
      Serial.print("buttonVal: ");
      Serial.print(buttons[index].buttonVal);
      Serial.print(", ");
      Serial.print("isChars: ");
      Serial.print(buttons[index].isChars);
      Serial.print(", ");
      Serial.print("keys: ");
      Serial.print(buttons[index].keys);
      Serial.print(", ");
      Serial.print("control: ");
      Serial.print(buttons[index].control);
      Serial.print(", ");
      Serial.print("shift: ");
      Serial.print(buttons[index].shift);
      Serial.print(", ");
      Serial.print("alt: ");
      Serial.println(buttons[index].alt);
    }
    /*
    if (error) {
      Serial.print("deserializeJson of layout file failed:");
      Serial.println(error.f_str());
    }
    else {    // fetch layout file contents
      Serial.println("read layout file goes here");
    }
    */
  }
}

void putLayout(String file = "/first.layout") {
  Serial.println("Saving:");
  Serial.println(file);

  // load current configuration
  File lof = LittleFS.open(file, "w");
  if (!lof) {
    // file missing, write default
    Serial.print("Error opening ");
    Serial.println(file);
  }
  else {  // file found and open for reading
    DynamicJsonDocument layout(6200);

    for (int index = 0; index < 42; index++) {
      layout[index]["buttonVal"] = buttons[index].buttonVal;
      layout[index]["isChars"] = buttons[index].isChars;
      layout[index]["keys"] = buttons[index].keys;
      layout[index]["control"] = buttons[index].control;
      layout[index]["shift"] = buttons[index].shift;
      layout[index]["alt"] = buttons[index].alt;
    }
    serializeJsonPretty(layout, lof);
  }
}

void readFile(const char * path){
    Serial.printf("Reading file: %s\r\n", path);

    File file = LittleFS.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return;
    }

    Serial.println("- read from file:");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
}

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

// init web server
void initWebServer() {
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
      inputMessage1 = "No messge sent";
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
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels){
    Serial.printf("Listing directory: %s\r\n", dirname);

    File root = LittleFS.open(dirname);
    if(!root){
        Serial.println("- failed to open directory");
        return;
    }
    if(!root.isDirectory()){
        Serial.println(" - not a directory");
        return;
    }

    File file = root.openNextFile();
    while(file){
        if(file.isDirectory()){
            Serial.print("  DIR : ");
            Serial.println(file.name());
            if(levels){
                listDir(fs, file.path(), levels -1);
            }
        } else {
            Serial.print("  FILE: ");
            Serial.print(file.name());
            Serial.print("\tSIZE: ");
            Serial.println(file.size());
        }
        file = root.openNextFile();
    }
}

bool readFile(fs::FS &fs, const char * path){
    Serial.printf("Reading file: %s\r\n", path);

    File file = LittleFS.open(path);
    if(!file || file.isDirectory()){
        Serial.println("- failed to open file for reading");
        return false;
    }

    Serial.println("- read from file:");
    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
    return true;
}

void writeFile(fs::FS &fs, const char * path, const char * message){
    Serial.printf("Writing file: %s\r\n", path);

    File file = LittleFS.open(path, FILE_WRITE);
    if(!file){
        Serial.println("- failed to open file for writing");
        return;
    }
    if(file.print(message)){
        Serial.println("- file written");
    } else {
        Serial.println("- write failed");
    }
    file.close();
}

void deleteFile(fs::FS &fs, const char * path){
    Serial.printf("Deleting file: %s\r\n", path);
    if(LittleFS.remove(path)){
        Serial.println("- file deleted");
    } else {
        Serial.println("- delete failed");
    }
}

void initLittleFS() {
  if (!LittleFS.begin(true)) {
    Serial.println("Error mounting LittleFS");
    return;
  } else {
    Serial.println("LittleFS mounted");

    #if defined(DEBUG)
    // There should be a config file and at least one buttons map file
    listDir(LittleFS, "/", 0);
    #endif
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
  bool switchState;

  for (uint8_t enc = 0; enc < sizeof(found_encoders); enc++) { 
    if (found_encoders[enc] == false) continue;

    int32_t new_position = encoders[enc].getEncoderPosition();
    // did we move around?
    if (encoder_positions[enc] != new_position) {
      int32_t change = new_position - encoder_positions[enc];
      // map encoders such that cw is base, ccw is base + 1, pushbutton is base +2
      if (change > 0)         // which way did he go...
        button = enc * 3 + encoderOffset;
      else
        button = enc * 3 + encoderOffset + 1;

      if (encoderMode[enc].shifted)
        button += shiftSize;        // add offset for additional inputs shifted
      // send press for each step
      for (int count = 0; count < abs(change); count++) {
        sendButton(button, Pulse);
      }

      #if defined(DEBUG)
      Serial.print("Encoder #");
      Serial.print(enc);
      Serial.print(" -> ");
      Serial.println(new_position);         // display new position
      #endif
      encoder_positions[enc] = new_position;

      // change the neopixel color, mulitply the new positiion by 4 to speed it up
      // encoder_pixels[enc].setPixelColor(0, Wheel((new_position*4) & 0xFF));
      // encoder_pixels[enc].show();
    }

    if (ssSwitchState[enc] != (switchState = encoders[enc].digitalRead(SS_SWITCH))) {     // switch pressed or released
      #if defined(DEBUG)
      Serial.print("Encoder #");
      Serial.print(enc);
      Serial.print(" value is: ");
      Serial.println(switchState);
      #endif
      // modal && switch state is true
      if (encoderMode[enc].modal && ssSwitchState[enc]) {
        encoderMode[enc].shifted = !encoderMode[enc].shifted;
        if (encoderMode[enc].shifted)
          encoder_pixels[enc].setPixelColor(0, 100, 0, 0);
        else
          encoder_pixels[enc].setPixelColor(0, 0, 100, 0);
        encoder_pixels[enc].show();

      } else {              // if not modal, send button press/release
        if (switchState)
          sendButton(enc * 3 + encoderOffset, Off);    // pushbutton is base + 2
        else
          sendButton(enc * 3 + encoderOffset, On);
      }
      ssSwitchState[enc] = switchState;
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
      if (encoderMode[enc].modal)
        encoder_pixels[enc].setPixelColor(0, 0, 100, 0);
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

  initWebServer();

  getConfig();
  getLayout();
  putLayout("/first.layout");
  getLayout("/first.layout");
  // readFile("/first.layout");

  Gamepad.begin();
  Keyboard.begin();
  USB.begin();

  #if defined(DEBUG)
  Serial.println("Looking for encoders");
  #endif

  initEncoders();
}

void CheckAllButtons(void) {
  if (keypad.getKeys())
  {
    for (int i = 0; i < LIST_MAX; i++)
    {
      if ( keypad.key[i].stateChanged )
      {
        switch (keypad.key[i].kstate) {
          case PRESSED:
            sendButton(keypad.key[i].kchar - 1, On);      // change to zero based
            break;
          case RELEASED:
            sendButton(keypad.key[i].kchar - 1, Off);
            break;
          // case HOLD:
          // case IDLE:
          //   break;
        }
      }
    }
  }
}

void loop() {
  #if defined(TESTCODE)
  uint8_t key = keypad.getKey();

  if (key) {
    #if defined(DEBUG)
    Serial.println(key);
    #endif
    sendButton(key - 1, true);    // buttons are zero based
  }
  #else
    CheckAllButtons();
  #endif

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