#include <Bounce2.h>
#include <Homie.h>
#include <Button.h>
#include <Wire.h>

const int PIN_RELAY = 15;
const int PIN_LED = 2;
const int PIN_BUTTON = 13;

// WiOn 50055 with dual USB, backlight, power monitoring
#define PIN_BACKLIGHT 14
#define PIN_SDA 12
#define PIN_SCK 0

HomieNode switchNode("plug", "switch");
Button button1(PIN_BUTTON);

HomieSetting<bool> poweronState("plugon", "Default poweron state");

#ifdef PIN_BACKLIGHT
HomieNode lightNode("backlight", "switch");
static void setBacklight(int state) {
  digitalWrite(PIN_BACKLIGHT, state ? HIGH : LOW);
  lightNode.setProperty("on").send(state ? "true" : "false");
  Serial.print("Backlight is ");
  Serial.println(state ? "on" : "off");
}

bool backlightOnHandler(HomieRange range, String value) {
  if (value == "true") {
    setBacklight(1);
  } else if (value == "false") {
    setBacklight(0);
  } else {
    Serial.print("Error Got: ");
    Serial.println(value);
    return false;
  }

  return true;
}
#endif

#ifdef PIN_SDA
const int DEFAULT_WATTS_INTERVAL = 15;

HomieNode wattsNode("watts", "watts");

HomieSetting<long> wattsIntervalSetting("wattsInterval",
	"The current measurement interval in seconds");

static void setupHandler() {
  wattsNode.setProperty("unit").send("watts");
}

static void loopHandler() {
  static unsigned long lastWattsSent = 0;
  static unsigned long lastWatts = 0;
  static byte d[16],count = 0;
  static int reading = 0;

  if( reading ) {
    while( Wire.available() && count<16 ) {
      d[count++] = Wire.read();
    }
    if( count == 16 ) {
      unsigned long value=(d[0]<<24) + (d[1]<<16) + (d[2]<<8) + d[3];
      unsigned long watts = (value/(400-(value/1800)));

      if( watts != lastWatts ) {
        Homie.getLogger() << "Watts: " << watts << " e" << endl;
        wattsNode.setProperty("watts").send(String(watts));
        lastWatts = watts;
      }
      reading = 0;
      lastWattsSent = millis();
    }
  } else if (millis() - lastWattsSent >= wattsIntervalSetting.get() * 1000UL
  	|| lastWattsSent == 0
  ) {
    count = 0;
    Wire.requestFrom(0, 16);
    reading = 1;
  }
}

#endif

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
  pinMode(PIN_BUTTON,INPUT_PULLUP);
  pinMode(PIN_RELAY, OUTPUT);
#ifdef PIN_SDA
  pinMode(PIN_SDA, INPUT_PULLUP);
  Wire.begin(PIN_SDA, PIN_SCK);
#endif

  Homie.setLedPin(PIN_LED, LOW);
  Homie.setResetTrigger(PIN_BUTTON, LOW, 5000);
  Homie_setFirmware("ecoplug", "1.0.1");

#ifdef PIN_SDA
  Homie.setSetupFunction(setupHandler).setLoopFunction(loopHandler);

  wattsNode.advertise("unit");
  wattsNode.advertise("watts");

  wattsIntervalSetting.setDefaultValue(DEFAULT_WATTS_INTERVAL).setValidator([] (long candidate) {
    return candidate > 0;
  });
#endif

  switchNode.advertise("on").settable(lightOnHandler);

#ifdef PIN_BACKLIGHT
  pinMode(PIN_BACKLIGHT, OUTPUT);
  lightNode.advertise("on").settable(backlightOnHandler);
#endif

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
