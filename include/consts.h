#ifndef CONSTS_INCLUDED
#define CONSTS_INCLUDED
#define SETTINGS_FILE              "/settings.json"
#define PORT_FILE                  "/ports.json"
#define COMMAND_TOPIC              "ampel/commandedState"
#define STATE_TOPIC_SUED           "ampel/stateSued"
#define STATE_TOPIC_NORD           "ampel/stateNord"
#define STATE_TOPIC_WINDE_WARNING  "winde/warningLight"
#include "tools.h"
const int windeInt = str2int("winde");
const int startwagenInt = str2int("startwagen");
const int ampelSuedInt = str2int("ampelSued");
const int ampelNordInt = str2int("ampelNord");
#endif