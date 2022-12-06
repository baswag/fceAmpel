#ifndef STARTWAGEN_SETUP_PAGE_INCLUDED
#define STARTWAGEN_SETUP_PAGE_INCLUDED
#include <Arduino.h>
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
#endif