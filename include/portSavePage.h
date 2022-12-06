#ifndef PORT_SAVE_PAGE_INCLUDED
#define PORT_SAVE_PAGE_INCLUDED
#include <Arduino.h>
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
#endif