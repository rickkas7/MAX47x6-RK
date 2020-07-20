#include "MAX47x6-RK.h"

SYSTEM_THREAD(ENABLED);
  
// SYSTEM_MODE(MANUAL);

SerialLogHandler logHandler;

MAX47x6 dac(MAX47x6::Model::MAX4706, 0x60, Wire);


void setup() {
    waitFor(Serial.isConnected, 15000);
    delay(1000);

	dac.begin();
	dac.updateSettings(MAX47x6::VREF_VDD, MAX47x6::GAIN_1X, 24, false);

	Log.info("setup complete");
}

void loop() {
}
