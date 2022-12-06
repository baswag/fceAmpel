#ifndef CONFIG_INCLUDED
#define CONFIG_INCLUDED
#include <Arduino.h>
#include <string>
#include <AutoConnect.h>

struct MqttConfiguration {
  String broker;
  String user;
  String password;
};

struct AmpelConfiguration {
  int ledPort;
  int warningLightInputPort;
  int warningLightOutputPort;
  int buttonPort;
  bool ledLowOn;
  bool warningLightOutputLowOn;
  bool warningLightInputLowOn;
  bool useNeopixel;
  uint8 numLeds;
  uint32 blinkIntervalMs;
};

class ConfigurationLoader {
  protected:
    char* settingsFile;
    std::vector<String> settings;
    AutoConnectAux autoconnectPage;

  public:
    void saveConfiguration(char* settingsFileToDelete) {
      LittleFS.begin();
      File param = LittleFS.open(settingsFile, "w");
      autoconnectPage.saveElement(param, settings);
      param.close();

      if(settingsFileToDelete != "undefined") {
        LittleFS.remove(settingsFileToDelete);
      }
      LittleFS.end();
    }

    void loadConfiguration() {
      LittleFS.begin();
      File param = LittleFS.open(settingsFile, "r");
      bool result = false;
      if(param && autoconnectPage.loadElement(param)) {
        Serial.println(String(settingsFile) + " loaded");
      }else {
        Serial.println(String(settingsFile) + " failed to load");
      }
      LittleFS.end();
    }
 };

 class MqttConfigurationLoader : public ConfigurationLoader {

  public:
    MqttConfigurationLoader(char* file, std::vector<String> settingsToSave, AutoConnectAux settingsPage) {
      settingsFile = file;
      settings = settingsToSave;
      autoconnectPage = settingsPage;
    }

    MqttConfiguration getConfigFromInput() {
      MqttConfiguration result = {};
          result.broker = autoconnectPage.getElement<AutoConnectInput>("broker").value;
          result.user = autoconnectPage.getElement<AutoConnectSelect>("user").value();
          result.password = autoconnectPage.getElement<AutoConnectInput>("password").value;
      return result;
    }
 };

 class AmpelConfigurationLoader: public ConfigurationLoader {
  public:
    AmpelConfigurationLoader(char* file, std::vector<String> settingsToSave, AutoConnectAux settingsPage) {
      settingsFile = file;
      settings = settingsToSave;
      autoconnectPage = settingsPage;
    }

    AmpelConfiguration getConfigFromInput() {
      AmpelConfiguration result = {};
      result.buttonPort = atoi(autoconnectPage.getElement<AutoConnectInput>("buttonPort").value.c_str());
      result.warningLightInputPort = atoi(autoconnectPage.getElement<AutoConnectInput>("warningLightInputPort").value.c_str());
      result.ledPort = atoi(autoconnectPage.getElement<AutoConnectInput>("ledPort").value.c_str());
      result.ledLowOn =  autoconnectPage.getElement<AutoConnectCheckbox>("ledLowOn").checked;
      result.warningLightInputLowOn = autoconnectPage.getElement<AutoConnectCheckbox>("warningLightInputLowOn").checked;
      result.warningLightOutputPort = atoi(autoconnectPage.getElement<AutoConnectInput>("warningLightOutputPort").value.c_str());
      result.warningLightOutputLowOn = autoconnectPage.getElement<AutoConnectCheckbox>("warningLightOutputLowOn").checked;
      result.useNeopixel = autoconnectPage.getElement<AutoConnectCheckbox>("useNeopixel").checked;
      if(!autoconnectPage.getElement<AutoConnectInput>("numLeds").value.isEmpty()) {
        uint8 numValue = atoi(autoconnectPage.getElement<AutoConnectInput>("numLeds").value.c_str());
        if(numValue < 1) {
          result.numLeds = 1;
        }else {
          result.numLeds = numValue;
        }
      }
      if(!autoconnectPage.getElement<AutoConnectInput>("blinkIntervalMs").value.isEmpty()) {
        result.blinkIntervalMs = atoi(autoconnectPage.getElement<AutoConnectInput>("blinkIntervalMs").value.c_str());
      }
      
      return result;
    }
 };
#endif