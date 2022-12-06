#ifndef WINDE_SETUP_PAGE_INCLUDED
#define WINDE__SETUP_PAGE_INCLUDED
#include <Arduino.h>
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
#endif