#ifndef AMPEL_SETUP_PAGE_INCLUDED
#define AMPEL_SETUP_PAGE_INCLUDED
#include <Arduino.h>
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
        "label": "Blink Interval for the LEDs, 0=disabled",
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
#endif