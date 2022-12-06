#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <AutoConnect.h>
#include <PubSubClient.h>
#include "LittleFS.h"
#include <NeoPixelBus.h>
#include <string>
#include "ampelSetupPage.h"
#include "portSavePage.h"
#include "settingsPage.h"
#include "settingsSavePage.h"
#include "startwagenSetupPage.h"
#include "windeSetupPage.h"
#include "tools.h"
#include "consts.h"
#include "config.h"

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

MqttConfiguration mqttConfig;
AmpelConfiguration ampelConfig;
AmpelConfigurationLoader* configLoader;
MqttConfigurationLoader* mqttConfigLoader;

NeoPixelBus<NeoRgbFeature, NeoEsp8266Dma400KbpsMethod>* strip;
RgbColor neopixelOn(128,0,0);
RgbColor neopixelOff(0);

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
  if(mqttConfig.user != "winde" && mqttConfig.user != "startwagen"){
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

String savePorts(AutoConnectAux& aux, PageArgument& args) {
  AutoConnectText& result = aux.getElement<AutoConnectText>("Result");
  configLoader->saveConfiguration("undefined");
  
  result.value = "Saved. Click OK to restart Device";
  return String();
}

void reconnectMqtt() {
  if(mqttConfig.user.length() == 0) {
    return;
  }
  mqttUserInt = str2int(mqttConfig.user.c_str());
  client.setServer(mqttConfig.broker.c_str(), 1883);
  while(!client.connected()){
    Serial.print("Reconnecting mqtt...");
    if(!client.connect(mqttConfig.user.c_str(), mqttConfig.user.c_str(), mqttConfig.password.c_str())) {
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
  mqttConfig = mqttConfigLoader->getConfigFromInput();

  reconnectMqtt();
  AutoConnectText& result = aux.getElement<AutoConnectText>("Result");
  if(client.connected()) {
    mqttConfigLoader->saveConfiguration(PORT_FILE);
    result.value = "Connected";
  } else {
    result.value = "Error";
  }
  return String();
}

void turnLedOn() {
  isLedOn = true;
  if(ampelConfig.useNeopixel){
    digitalWrite(13, HIGH);
    strip->ClearTo(neopixelOn);
    while(!strip->CanShow()) {
      
    }
    strip->Show();
    return;
  } else if(ampelConfig.ledLowOn) {
    digitalWrite(ampelConfig.ledPort, LOW);
  } else {
    digitalWrite(ampelConfig.ledPort, HIGH);
  }
}

void turnLedOff() {
  isLedOn = false;
  if(ampelConfig.useNeopixel){
    digitalWrite(13, LOW);
    strip->ClearTo(neopixelOff);
    while(!strip->CanShow()) {

    }
    strip->Show();
    return;
  }else if(ampelConfig.ledLowOn) {
    digitalWrite(ampelConfig.ledPort, HIGH);
  } else {
    digitalWrite(ampelConfig.ledPort, LOW);
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
  mqttConfigLoader = new MqttConfigurationLoader(SETTINGS_FILE, { "broker", "user", "password" }, settings);
  mqttConfigLoader->loadConfiguration();
  mqttConfig = mqttConfigLoader->getConfigFromInput();
  pinMode(13, OUTPUT);
  digitalWrite(13, HIGH);
  if(mqttConfig.user == "winde") {
    windeSetup.load(windeSetupPage);
    windeSetup.menu(true);
    configLoader = new AmpelConfigurationLoader(PORT_FILE, {"buttonPort", "ledPort", "warningLightInputPort", "ledLowOn", "warningLightInputLowOn" }, windeSetup);
    configLoader->loadConfiguration();
    ampelConfig = configLoader->getConfigFromInput();
    pinMode(ampelConfig.buttonPort, INPUT_PULLUP);
    pinMode(ampelConfig.warningLightInputPort, INPUT);
    pinMode(ampelConfig.ledPort, OUTPUT);
    attachInterrupt(digitalPinToInterrupt(ampelConfig.buttonPort), buttonPressed, FALLING);
    Portal.join({windeSetup});
  } else if(mqttConfig.user == "startwagen") {
    startwagenSetup.load(startwagenSetupPage);
    startwagenSetup.menu(true);
    configLoader = new AmpelConfigurationLoader(PORT_FILE, {"buttonPort", "ledPort", "warningLightOutputPort", "ledLowOn",  "warningLightOutputLowOn" }, startwagenSetup);
    configLoader->loadConfiguration();
    ampelConfig = configLoader->getConfigFromInput();
    pinMode(ampelConfig.buttonPort, INPUT_PULLUP);
    pinMode(ampelConfig.warningLightOutputPort, OUTPUT);
    pinMode(ampelConfig.ledPort, OUTPUT);
    attachInterrupt(digitalPinToInterrupt(ampelConfig.buttonPort), buttonPressed, FALLING);
    Portal.join({startwagenSetup});
  } else if(mqttConfig.user == "ampelSued" || mqttConfig.user == "ampelNord") {
    ampelSetup.load(ampelSetupPage);
    ampelSetup.menu(true);
    configLoader = new AmpelConfigurationLoader(PORT_FILE, {"ledPort", "ledLowOn", "useNeopixel", "numLeds", "blinkIntervalMs"}, ampelSetup);
    configLoader->loadConfiguration();
    ampelConfig = configLoader->getConfigFromInput();
    Portal.join({ampelSetup});

    if(ampelConfig.useNeopixel) {
      strip = new NeoPixelBus<NeoRgbFeature, NeoEsp8266Dma400KbpsMethod>(ampelConfig.numLeds, 3);
      strip->Begin();
      strip->ClearTo(neopixelOff);
      strip->Show();
    }else {
      pinMode(ampelConfig.ledPort, OUTPUT);
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
    if(ampelConfig.blinkIntervalMs == 0) {
      if(!isLedOn){
        turnLedOn();
      }
    }else {
      unsigned long currentMillis = millis();
      if(currentMillis - previousMillis > ampelConfig.blinkIntervalMs) {
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
        if(ampelConfig.warningLightOutputLowOn) {
          digitalWrite(ampelConfig.warningLightOutputPort, LOW);
        }else {
          digitalWrite(ampelConfig.warningLightOutputPort, HIGH);
        }
      }else {
        if(ampelConfig.warningLightOutputLowOn) {
          digitalWrite(ampelConfig.warningLightOutputPort, HIGH);
        }else {
          digitalWrite(ampelConfig.warningLightOutputPort, LOW);
        }
      }
      setLed();
      break;
    }
    case windeInt: {
      bool stateWinde;
      if(ampelConfig.warningLightInputLowOn) {
        stateWinde = digitalRead(ampelConfig.warningLightInputPort) == LOW;
      } else {
        stateWinde = digitalRead(ampelConfig.warningLightInputPort) == HIGH;
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
