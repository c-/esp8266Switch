#include <Bounce2.h>
#include <Homie.h>
#include <Button.h>

const int PIN_RELAY = 15;
const int PIN_LED = 2;
const int PIN_BUTTON = 13;


HomieNode switchNode("plug", "switch");
Button button1(PIN_BUTTON); // Connect your button between pin 2 and GND

HomieSetting<bool> poweronState("plugon", "Default poweron state");

static void setRelay(int state) {
  digitalWrite(PIN_RELAY, state ? HIGH : LOW);
  switchNode.setProperty("on").send(state ? "true" : "false");
  Serial.print("Plug is ");
  Serial.println(state ? "on" : "off");
}

bool lightOnHandler(HomieRange range, String value) {
  if (value == "true") {
    setRelay(1);
  } else if (value == "false") {
    setRelay(0);
  } else {
    Serial.print("Error Got: ");
    Serial.println(value);
    return false;
  }

  return true;
}

void setup() {
  Serial.begin(115200);
  Serial.println("ecoplug booting");
  Serial.println();
  //pinMode(PIN_BUTTON,INPUT_PULLUP);
  pinMode(PIN_RELAY, OUTPUT);

  Homie.setLedPin(PIN_LED, LOW);
  //Homie.setResetTrigger(PIN_BUTTON, LOW, 5000);
  Homie_setFirmware("ecoplug", "1.0.0");
  switchNode.advertise("on").settable(lightOnHandler);

  poweronState.setDefaultValue(false);
  
  button1.begin();
  Homie.setup();

  setRelay(poweronState.get());
}

void loop() {
  Homie.loop();
  if (button1.pressed())
  {
    setRelay( !digitalRead(PIN_RELAY) );
  }
}
