#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <AutoConnect.h>
#include <PubSubClient.h>
#include "LittleFS.h"
#include <NeoPixelBus.h>
#include <string>

#define SETTINGS_FILE              "/settings.json"
#define PORT_FILE                  "/ports.json"
#define COMMAND_TOPIC              "ampel/commandedState"
#define STATE_TOPIC_SUED           "ampel/stateSued"
#define STATE_TOPIC_NORD           "ampel/stateNord"
#define STATE_TOPIC_WINDE_WARNING  "winde/warningLight"

constexpr unsigned int str2int(const char* str, int h = 0)
{
    return !str[h] ? 5381 : (str2int(str, h+1) * 33) ^ str[h];
}

const static char* settingsPage PROGMEM = R"(
  {
   "title": "Settings",
   "uri": "/settings",
   "menu": false,
   "element": [
     {
       "name": "broker",
       "type": "ACInput",
       "value": "",
       "label": "MQTT Broker"
     },
     {
       "name": "user",
       "type": "ACSelect",
       "label": "MQTT User",
       "option": [
         "startwagen",
         "winde",
         "ampelSued",
         "ampelNord"
       ]
     },
     {
      "name": "password",
      "type": "ACInput",
      "label": "MQTT Password",
      "apply": "password"
     },
     {
      "name": "save",
      "type": "ACSubmit",
      "value": "Save",
      "uri": "/settings_save"
    },
    {
      "name": "discard",
      "type": "ACSubmit",
      "value": "Discard",
      "uri": "/_ac"
    }
   ]
  }
)";

const static char* windeSetupPage PROGMEM = R"(
  {
    "title": "Port Setup",
    "uri": "/settings_winde",
    "menu": false,
    "element": [
      {
        "name": "buttonPort",
        "type": "ACInput",
        "apply": "number",
        "label": "Button Port",
        "value": "4"
      },
      {
        "name": "ledPort",
        "type": "ACInput",
        "apply": "number",
        "label": "LED Port",
        "value": "2"
      },
      {
        "name": "ledLowOn",
        "type": "ACCheckbox",
        "label": "LED On LOW",
        "checked": true
      },
      {
        "name": "warningLightInputPort",
        "type": "ACInput",
        "apply": "number",
        "label": "Warning Light Input Port",
        "value": "3"
      },
      {
        "name": "warningLightInputLowOn",
        "type": "ACCheckbox",
        "label": "Warning Light Input On LOW",
        "checked": true
      },
      {
        "name": "save",
        "type": "ACSubmit",
        "value": "Save",
        "uri": "/ports_save"
      },
      {
        "name": "discard",
        "type": "ACSubmit",
        "value": "Discard",
        "uri": "/_ac"
      }
    ]
  }
)";

const static char* startwagenSetupPage PROGMEM = R"(
  {
    "title": "Port Setup",
    "uri": "/settings_startwagen",
    "menu": false,
    "element": [
      {
        "name": "buttonPort",
        "type": "ACInput",
        "apply": "number",
        "label": "Button Port",
        "value": "4"
      },
      {
        "name": "ledPort",
        "type": "ACInput",
        "apply": "number",
        "label": "LED Port",
        "value": "2"
      },
      {
        "name": "ledLowOn",
        "type": "ACCheckbox",
        "label": "LED On LOW",
        "checked": true
      },
      {
        "name": "warningLightOutputPort",
        "type": "ACInput",
        "apply": "number",
        "label": "Warning Light Output Port",
        "value": "3"
      },
      {
        "name": "warningLightOutputLowOn",
        "type": "ACCheckbox",
        "label": "Warning Light Output On LOW",
        "checked": true
      },
      {
        "name": "save",
        "type": "ACSubmit",
        "value": "Save",
        "uri": "/ports_save"
      },
      {
        "name": "discard",
        "type": "ACSubmit",
        "value": "Discard",
        "uri": "/_ac"
      }
    ]
  }
)";

const static char* ampelSetupPage PROGMEM = R"(
  {
    "title": "Port Setup",
    "uri": "/settings_ampel",
    "menu": false,
    "element": [
      {
        "name": "ledPort",
        "type": "ACInput",
        "apply": "number",
        "label": "LED Port",
        "value": "2"
      },
      {
        "name": "ledLowOn",
        "type": "ACCheckbox",
        "label": "LED On LOW",
        "checked": true
      },
      {
        "name": "useNeopixel",
        "type": "ACCheckbox",
        "label": "Use Neopixel?",
        "checked": false
      },
      {
        "name": "numLeds",
        "type": "ACInput",
        "apply": "number",
        "label": "Number of Neopixel LEDs",
        "value": "2"
      },
      {
        "name": "blinkIntervalMs",
        "type": "ACInput",
        "apply": "number",
        "label": "Blink Interval for the LEDs (0=disabled),
        "value": "0"
      },
      {
        "name": "save",
        "type": "ACSubmit",
        "value": "Save",
        "uri": "/ports_save"
      },
      {
        "name": "discard",
        "type": "ACSubmit",
        "value": "Discard",
        "uri": "/_ac"
      }
    ]
  }
)";

const static char* portSavePage PROGMEM = R"(
  {
  "title": "Port Setup",
  "uri": "/ports_save",
  "menu": false,
  "element": [
    {
      "name": "Result",
      "type": "ACText"
    },
    {
      "name": "OK",
      "type": "ACSubmit",
      "value": "OK",
      "uri": "/_ac/reset"
    }
  ]
}
)";

const static char* settingsSavePage PROGMEM = R"(
{
  "title": "Settings",
  "uri": "/settings_save",
  "menu": false,
  "element": [
    {
      "name": "Result",
      "type": "ACText"
    },
    {
      "name": "OK",
      "type": "ACSubmit",
      "value": "OK. Reset Device",
      "uri": "/_ac/reset"
    }
  ]
}
)";

ESP8266WebServer Server;
AutoConnect Portal(Server);
AutoConnectAux settings;
AutoConnectAux settingsSave;
AutoConnectAux windeSetup;
AutoConnectAux startwagenSetup;
AutoConnectAux ampelSetup;
AutoConnectAux portSave;
WiFiClient espClient;
PubSubClient client(espClient);
AutoConnectConfig config;

String MQTT_USER;
String MQTT_PASS;
String MQTT_BROKER;

uint32 AMPEL_BLINK_INTERVAL_MS;
int LED_PORT = 2;
int WARNING_LIGHT_INPUT_PORT = 3;
int BUTTON_PORT = 4;
int WARNING_LIGHT_PORT = 3;
bool LED_LOW_ON = true;
bool WARNING_LIGHT_OUTPUT_LOW_ON = true;
bool WARNING_LIGHT_INPUT_LOW_ON = true;
bool USE_NEOPIXEL = false;
uint8 NUM_LED = 1;

NeoPixelBus<NeoRgbFeature, NeoEsp8266Dma400KbpsMethod>* strip;
RgbColor neopixelOn(128,0,0);
RgbColor neopixelOff(0);

const int windeInt = str2int("winde");
const int startwagenInt = str2int("startwagen");
const int ampelSuedInt = str2int("ampelSued");
const int ampelNordInt = str2int("ampelNord");
int mqttUserInt;

bool stateSued = false;
bool stateNord = false;
bool stateWindeMqtt = false;
bool stateWinde = false;
bool commandedState = false;
bool isLedOn = false;
char* state = "off";
char* lastPublished = "";
unsigned long lastTrigger = 0;

IRAM_ATTR void buttonPressed() {
  if(MQTT_USER != "winde" && MQTT_USER != "startwagen"){
    return;
  }
  unsigned long time = millis();
  if (time > lastTrigger + 1000) {
    lastTrigger = time;
    if (commandedState) {
      client.publish(COMMAND_TOPIC, "off", true);
      commandedState = false;
    } else {
      client.publish(COMMAND_TOPIC, "on", true);
      commandedState = true;
    }
  }
}

void rootPage() {
  Server.send(200, "text/plain", "Hello World!");
}

void getWindeParams() {
  BUTTON_PORT = atoi(windeSetup.getElement<AutoConnectInput>("buttonPort").value.c_str());
  WARNING_LIGHT_INPUT_PORT = atoi(windeSetup.getElement<AutoConnectInput>("warningLightInputPort").value.c_str());
  LED_PORT = atoi(windeSetup.getElement<AutoConnectInput>("ledPort").value.c_str());
  LED_LOW_ON = windeSetup.getElement<AutoConnectCheckbox>("ledLowOn").checked;
  WARNING_LIGHT_INPUT_LOW_ON = windeSetup.getElement<AutoConnectCheckbox>("warningLightInputLowOn").checked;
}

void getStartwagenParams() {
  BUTTON_PORT = atoi(startwagenSetup.getElement<AutoConnectInput>("buttonPort").value.c_str());
  WARNING_LIGHT_PORT = atoi(startwagenSetup.getElement<AutoConnectInput>("warningLightOutputPort").value.c_str());
  LED_PORT = atoi(startwagenSetup.getElement<AutoConnectInput>("ledPort").value.c_str());
  LED_LOW_ON = startwagenSetup.getElement<AutoConnectCheckbox>("ledLowOn").checked;
  WARNING_LIGHT_OUTPUT_LOW_ON = startwagenSetup.getElement<AutoConnectCheckbox>("warningLightOutputLowOn").checked;
}

void getAmpelParams() {
  LED_PORT = atoi(ampelSetup.getElement<AutoConnectInput>("ledPort").value.c_str());
  LED_LOW_ON = ampelSetup.getElement<AutoConnectCheckbox>("ledLowOn").checked;
  USE_NEOPIXEL = ampelSetup.getElement<AutoConnectCheckbox>("useNeopixel").checked;
  uint8 NUM_VALUE = atoi(ampelSetup.getElement<AutoConnectInput>("numLeds").value.c_str());
  if(NUM_VALUE < 1) {
    NUM_LED = 1;
  }else {
    NUM_LED = NUM_VALUE;
  }
  AMPEL_BLINK_INTERVAL_MS = atoi(ampelSetup.getElement<AutoConnectInput>("blinkIntervalMs").value.c_str());
}

void getParams() {
  MQTT_BROKER = settings.getElement<AutoConnectInput>("broker").value;
  MQTT_BROKER.trim();
  MQTT_USER = settings.getElement<AutoConnectSelect>("user").value();
  MQTT_USER.trim();
  MQTT_PASS = settings.getElement<AutoConnectInput>("password").value;
  MQTT_PASS.trim();
}

void loadWindeSettings(const char* paramFile) {
  LittleFS.begin();
  File param = LittleFS.open(paramFile, "r");
  if (param) {
    bool rc = windeSetup.loadElement(param);    // Load the elements with parameters
    if (rc){
      getWindeParams();
      Serial.println(String(paramFile) + " loaded");
    }
    else{
      Serial.println(String(paramFile) + " failed to load");
    }
    param.close();
  }
  LittleFS.end();
}

void loadStartwagenSettings(const char* paramFile) {
  LittleFS.begin();
  File param = LittleFS.open(paramFile, "r");
  if (param) {
    bool rc = startwagenSetup.loadElement(param);    // Load the elements with parameters
    if (rc){
      getStartwagenParams();
      Serial.println(String(paramFile) + " loaded");
    }
    else{
      Serial.println(String(paramFile) + " failed to load");
    }
    param.close();
  }
  LittleFS.end();
}

void loadAmpelSettings(const char* paramFile) {
  LittleFS.begin();
  File param = LittleFS.open(paramFile, "r");
  if (param) {
    bool rc = ampelSetup.loadElement(param);    // Load the elements with parameters
    if (rc){
      getAmpelParams();
      Serial.println(String(paramFile) + " loaded");
    }
    else{
      Serial.println(String(paramFile) + " failed to load");
    }
    param.close();
  }
  LittleFS.end();
}

void loadSettings(const char* paramFile) {
  LittleFS.begin();
  File param = LittleFS.open(paramFile, "r");
  if (param) {
    bool rc = settings.loadElement(param);    // Load the elements with parameters
    if (rc){
      getParams();
      Serial.println(String(paramFile) + " loaded");
    }
    else{
      Serial.println(String(paramFile) + " failed to load");
    }
    param.close();
  }
  LittleFS.end();
}

void saveSettings(const char* paramFile) {
  LittleFS.begin();
  File param = LittleFS.open(paramFile, "w");
  settings.saveElement(param, { "broker", "user", "password" });
  param.close();
  LittleFS.remove(PORT_FILE);
  LittleFS.end();
}

void saveWindeSettings(const char* paramFile) {
  LittleFS.begin();
  File param = LittleFS.open(paramFile, "w");
  windeSetup.saveElement(param, {"buttonPort", "ledPort", "warningLightInputPort", "ledLowOn", "warningLightInputLowOn" });
  param.close();
  LittleFS.end();
}

void saveStartwagenSettings(const char* paramFile) {
  LittleFS.begin();
  File param = LittleFS.open(paramFile, "w");
  startwagenSetup.saveElement(param, {"buttonPort", "ledPort", "warningLightOutputPort", "ledLowOn",  "warningLightOutputLowOn" });
  param.close();
  LittleFS.end();
}

void saveAmpelSettings(const char* paramFile) {
  LittleFS.begin();
  File param = LittleFS.open(paramFile, "w");
  ampelSetup.saveElement(param, {"ledPort", "ledLowOn", "useNeopixel", "numLeds", "blinkIntervalMs"});
  param.close();
  LittleFS.end();
}

String savePorts(AutoConnectAux& aux, PageArgument& args) {
  AutoConnectText& result = aux.getElement<AutoConnectText>("Result");
  if(MQTT_USER == "winde") {
    getWindeParams();
    saveWindeSettings(PORT_FILE);
  } else if (MQTT_USER == "startwagen") {
    getStartwagenParams();
    saveStartwagenSettings(PORT_FILE);
  } else if (MQTT_USER == "ampelSued" || MQTT_USER == "ampelNord") {
    getAmpelParams();
    saveAmpelSettings(PORT_FILE);
  }
  
  result.value = "Saved. Click OK to restart Device";
  return String();
}

void reconnectMqtt() {
  if(MQTT_USER.length() == 0) {
    return;
  }
  mqttUserInt = str2int(MQTT_USER.c_str());
  client.setServer(MQTT_BROKER.c_str(), 1883);
  while(!client.connected()){
    Serial.print("Reconnecting mqtt...");
    if(!client.connect(MQTT_USER.c_str(), MQTT_USER.c_str(), MQTT_PASS.c_str())) {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 5 seconds");
      Portal.handleClient();
      delay(5000);
    } else {
      Serial.println(" connected");
      client.subscribe(COMMAND_TOPIC);

      switch(mqttUserInt) {
        case startwagenInt:
          client.subscribe(STATE_TOPIC_SUED);
          client.subscribe(STATE_TOPIC_NORD);
          client.subscribe(STATE_TOPIC_WINDE_WARNING);
          break;
        case windeInt:
          client.subscribe(STATE_TOPIC_SUED);
          client.subscribe(STATE_TOPIC_NORD);
          break;
      }
    }
  }
}

String onConnect(AutoConnectAux& aux, PageArgument& args){
  getParams();

  reconnectMqtt();
  AutoConnectText& result = aux.getElement<AutoConnectText>("Result");
  if(client.connected()) {
    saveSettings(SETTINGS_FILE);
    result.value = "Connected";
  } else {
    result.value = "Error";
  }
  return String();
}

void turnLedOn() {
  isLedOn = true;
  if(USE_NEOPIXEL){
    digitalWrite(13, HIGH);
    strip->ClearTo(neopixelOn);
    while(!strip->CanShow()) {
      
    }
    strip->Show();
    return;
  } else if(LED_LOW_ON) {
    digitalWrite(LED_PORT, LOW);
  } else {
    digitalWrite(LED_PORT, HIGH);
  }
}

void turnLedOff() {
  isLedOn = false;
  if(USE_NEOPIXEL){
    digitalWrite(13, LOW);
    strip->ClearTo(neopixelOff);
    while(!strip->CanShow()) {

    }
    strip->Show();
    return;
  }else if(LED_LOW_ON) {
    digitalWrite(LED_PORT, HIGH);
  } else {
    digitalWrite(LED_PORT, LOW);
  }
}

void mqttCallback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  String messageTemp;
  for (int i = 0; i < length; i++) {
    messageTemp += (char)message[i];
  }
  Serial.println();
  if (String(topic) == COMMAND_TOPIC) {
    commandedState = messageTemp == "on";
    if(ampelNordInt == mqttUserInt || ampelSuedInt == mqttUserInt) {
      if(commandedState) {
        turnLedOn();
        state = "on";
      } else {
        turnLedOff();
        state = "off";
      }
    }
  } else if (String(topic) == STATE_TOPIC_SUED) {
    stateSued = messageTemp == "on";
  } else if (String(topic) == STATE_TOPIC_NORD) {
    stateNord = messageTemp == "on";
  } else if(String(topic) == STATE_TOPIC_WINDE_WARNING){
    stateWinde = messageTemp == "on";
  }
}

void setup() {
  // ESP.eraseConfig();
  Serial.begin(115200);
  Serial.println();
  Server.on("/", rootPage);
  settings.load(settingsPage);
  settingsSave.load(settingsSavePage);
  settingsSave.menu(false);
  settingsSave.on(onConnect);
  config.auth = AC_AUTH_BASIC;
  config.authScope = AC_AUTHSCOPE_PORTAL;
  config.username = WEB_USER;
  config.password = WEB_PASS;
  config.psk = PSK;
  config.autoReconnect = true;
  config.ota = AC_OTA_BUILTIN;
  Portal.config(config);
  portSave.load(portSavePage);
  portSave.menu(false);
  portSave.on(savePorts);
  loadSettings(SETTINGS_FILE);
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  if(MQTT_USER == "winde") {
    windeSetup.load(windeSetupPage);
    windeSetup.menu(true);
    loadWindeSettings(PORT_FILE);
    pinMode(BUTTON_PORT, INPUT_PULLUP);
    pinMode(WARNING_LIGHT_INPUT_PORT, INPUT);
    pinMode(LED_PORT, OUTPUT);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PORT), buttonPressed, FALLING);
    Portal.join({windeSetup});
  } else if(MQTT_USER == "startwagen") {
    startwagenSetup.load(startwagenSetupPage);
    startwagenSetup.menu(true);
    loadStartwagenSettings(PORT_FILE);
    pinMode(BUTTON_PORT, INPUT_PULLUP);
    pinMode(WARNING_LIGHT_PORT, OUTPUT);
    pinMode(LED_PORT, OUTPUT);
    attachInterrupt(digitalPinToInterrupt(BUTTON_PORT), buttonPressed, FALLING);
    Portal.join({startwagenSetup});
  } else if(MQTT_USER == "ampelSued" || MQTT_USER == "ampelNord") {
    ampelSetup.load(ampelSetupPage);
    ampelSetup.menu(true);
    loadAmpelSettings(PORT_FILE);
    Portal.join({ampelSetup});

    if(USE_NEOPIXEL) {
      strip = new NeoPixelBus<NeoRgbFeature, NeoEsp8266Dma400KbpsMethod>(NUM_LED, 3);
      strip->Begin();
      for(int i=0; i<NUM_LED; i++){
        strip->SetPixelColor(i, neopixelOff);
      }
      strip->Show();
    }else {
      pinMode(LED_PORT, OUTPUT);
    }
  }


  Portal.join({settings, settingsSave, portSave});
  Portal.begin();
  Serial.println("Web Server started: " + WiFi.localIP().toString());
  client.setCallback(mqttCallback);
}

void flashNord(){
  turnLedOff();
  delay(500);
  turnLedOn();
  delay(750);
  turnLedOff();
}

void flashSued(){
  turnLedOff();
  delay(500);
  turnLedOn();
  delay(500);
  turnLedOff();
}

void flashBoth(){
  turnLedOff();
  delay(250);
  turnLedOn();
  delay(500);
  turnLedOff();
}

void setLed(){
  if(stateSued == stateNord && stateSued != commandedState){
    flashBoth();
  }else if(stateSued == commandedState && stateNord != stateSued){
    flashNord();
  }
  else if(stateNord == commandedState && stateNord != stateSued){
    flashSued();
  }else if(stateNord == stateSued && stateSued == commandedState){
    if(commandedState){
      turnLedOn();
    }else {
      turnLedOff();
    }
  }
}

unsigned long previousMillis = 0;

void doAmpel(char *topic) {
  if(commandedState) {
    if(AMPEL_BLINK_INTERVAL_MS == 0) {
      if(!isLedOn){
        turnLedOn();
      }
    }else {
      unsigned long currentMillis = millis();
      if(currentMillis - previousMillis > AMPEL_BLINK_INTERVAL_MS) {
        if(isLedOn) {
          turnLedOff();
        }else {
          turnLedOn();
        }
        previousMillis = millis();
      }
    }
  } else if (isLedOn) {
    turnLedOff();
  }
  if(state != lastPublished) {
    client.publish(topic, state, true);
    lastPublished = state;
  }
}

void loop() {
  bool wifiConnected = WiFi.status() == WL_CONNECTED;
  settings.menu(wifiConnected);

  if(!wifiConnected){
    Portal.handleClient();
  }
  if(!client.connected() && wifiConnected){
    reconnectMqtt();
    Portal.handleClient();
  }
  client.loop();
  startwagenSetup.menu(mqttUserInt == startwagenInt);
  windeSetup.menu(mqttUserInt == windeInt);
  switch(mqttUserInt) {
    case ampelSuedInt: {
      doAmpel(STATE_TOPIC_SUED);
      break;
    }
    case ampelNordInt: {
      doAmpel(STATE_TOPIC_NORD);
      break;
    }
    case startwagenInt: {
      if(stateWinde) {
        if(WARNING_LIGHT_OUTPUT_LOW_ON) {
          digitalWrite(WARNING_LIGHT_PORT, LOW);
        }else {
          digitalWrite(WARNING_LIGHT_PORT, HIGH);
        }
      }else {
        if(WARNING_LIGHT_OUTPUT_LOW_ON) {
          digitalWrite(WARNING_LIGHT_PORT, HIGH);
        }else {
          digitalWrite(WARNING_LIGHT_PORT, LOW);
        }
      }
      setLed();
      break;
    }
    case windeInt: {
      bool stateWinde;
      if(WARNING_LIGHT_INPUT_LOW_ON) {
        stateWinde = digitalRead(WARNING_LIGHT_INPUT_PORT) == LOW;
      } else {
        stateWinde = digitalRead(WARNING_LIGHT_INPUT_PORT) == HIGH;
      }

      if (stateWinde != stateWindeMqtt) {
        stateWindeMqtt = stateWinde;
        if (stateWinde) {
          client.publish(STATE_TOPIC_WINDE_WARNING, "on", true);
        } else {
          client.publish(STATE_TOPIC_WINDE_WARNING, "off", true);
        }
      }
      setLed();
      break;
    }
  }
  Portal.handleClient();
}
