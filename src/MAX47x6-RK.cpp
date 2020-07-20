// Repository: https://github.com/rickkas7/MAX47x6-RK
// License: MIT

#include "MAX47x6-RK.h"

static_assert(sizeof(MAX47x6ConfigurationStatus) == 1, "MAX47x6ConfigurationStatus should be 1 byte");
static_assert(sizeof(MAX47x6Status) == 6, "MAX47x6Status should be 6 bytes");
static_assert(sizeof(MAX47x6Status8) == 4, "MAX47x6Status8 should be 4 bytes");


MAX47x6::MAX47x6(Model model, uint8_t addr, TwoWire &wire) : model(model), addr(addr), wire(wire) {
	if (addr < 0x8) {
		// Just passed in 0 - 3, add in the 0x30 automatically to make addresses 0x60 - 0x67
		addr |= 0x60;
	}
}

MAX47x6::~MAX47x6() {

}


bool MAX47x6::begin() {
	// Initialize the I2C bus in standard master mode.
	wire.begin();

	return true;
}

bool MAX47x6::updateValue(uint16_t value) {
    return true; 
}



bool MAX47x6::updateEepromIfChanged(uint8_t vref, uint8_t gain, uint16_t value) {
    MAX47x6Status status = readStatus();
    if (status.nonVolatileConfig.vref != vref || 
        status.nonVolatileConfig.gain != gain || 
        status.nonVolatileValue != value) {
        // Value changed
        return updateSettings(vref, gain, value, true);
    }
    return true;
}


bool MAX47x6::updateSettings(uint8_t vref, uint8_t gain, uint16_t value, bool saveToEeprom) {
    uint8_t buf[3];
    buf[0] = buf[1] = buf[2] = 0;

    if (saveToEeprom) {
        buf[0] |= CMD_WRITE_ALL_MEMORY << 5; // 0b011
    } 
    else {
        buf[0] |= CMD_WRITE_VOLATILE_MEMORY << 5; // 0b010
    }

    buf[0]= vref << 3;

    if (gain) {
        buf[0] |= GAIN_2X;
    }

    switch(model) {
    case Model::MAX4706:
        // 8 bits fit in buf[1]
        buf[1] = (uint8_t) value;
        break;

    case Model::MAX4716:
        // 10 bits total
        // 8 bits in buf[1] D9 - D2
        // 2 bints in buf[2] D1 - D0
        buf[1] = (uint8_t) (value >> 2);
        buf[2] = (uint8_t) (value << 6);
        break;

    case Model::MAX4726:
        // 12 bits total
        // 8 bits in buf[1] D11 - D4
        // 4 bints in buf[2] D3 - D0
        buf[1] = (uint8_t) (value >> 4);
        buf[2] = (uint8_t) (value << 4);
        break;
    }

    if (!writeDevice(buf, sizeof(buf))) {
        return false;
    }

    // Wait until EEPROM write completes or 100 ms, whichever is shorter
    unsigned long start = millis();
    while(millis() - start < MAX_EEPROM_WAIT_MS && !isReady()) {
    }

    return true;
}




MAX47x6Status MAX47x6::readStatus() {
    MAX47x6Status result;

    if (model == Model::MAX4706) {
        // Status is 4 bytes, but convert to 6-byte format for consistency before return
        MAX47x6Status8 status8;
        readDevice((uint8_t *)&status8, 4);

        convert(&status8, &result);
    }
    else {
        // Status is 6 bytes
        readDevice((uint8_t *)&result, 6);
    }

    return result;
}




void MAX47x6::convert(const MAX47x6Status8 *from, MAX47x6Status *to) {
    to->volatileConfig = from->volatileConfig;
    to->volatileValue = from->volatileValue;  // uint8_t -> uint16_t
    to->nonVolatileConfig = from->nonVolatileConfig;
    to->nonVolatileValue = from->nonVolatileValue;  // uint8_t -> uint16_t
}
    


void MAX47x6::readDevice(uint8_t *buf, size_t bufLen) {

	wire.requestFrom(addr, (uint8_t) bufLen, (uint8_t) true);
    for(size_t ii = 0; ii < bufLen; ii++) {
        buf[ii] = (uint8_t) wire.read();
    }
}

bool MAX47x6::writeDevice(const uint8_t *buf, size_t bufLen) {

	wire.beginTransmission(addr);
    for(size_t ii = 0; ii < bufLen; ii++) {
        wire.write(buf[ii]);
    }

	int stat = wire.endTransmission(true);

	return (stat == 0);
}

