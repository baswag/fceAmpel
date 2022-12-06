#ifndef SETTINGS_PAGE_INCLUDED
#define SETTINGS_PAGE_INCLUDED
#include <Arduino.h>
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
#endif