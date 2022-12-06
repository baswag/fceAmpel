#ifndef SETTINGS_SAVE_PAGE_INCLUDED
#define SETTINGS_SAVE_PAGE_INCLUDED
#include <Arduino.h>
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
#endif