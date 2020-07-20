#ifndef __MAX47X6_RK_H
#define __MAX47X6_RK_H

// Repository: https://github.com/rickkas7/MAX47x6-RK
// License: MIT

#include "Particle.h"


typedef struct {    // 1 bytes (8 bits)
    int         ready:1;        //!< 1=ready, 0=EEPROM write in progress
    int         por:1;          //!< 1=ready, 0=power-on-reset, device unstable
    int         filler:1;       //!< bit not used
    int         vref:2;         //!< voltage reference setting
    int         powerdown:2;    //!< power-down bits
    int         gain:1;         //!< gain control bit (0=1x, 1=2x)
} __attribute__((packed)) MAX47x6ConfigurationStatus;

typedef struct {    // 6 bytes
    MAX47x6ConfigurationStatus  volatileConfig;
    uint16_t                    volatileValue;
    MAX47x6ConfigurationStatus  nonVolatileConfig;
    uint16_t                    nonVolatileValue;
} __attribute__((packed)) MAX47x6Status;

/**
 * @brief Structure for the data read from the MAX4706
 * 
 * This structure is used internally and converted into a MAX47x6Status before
 * returning to the user to make the calling code more flexible.
 */
typedef struct {    // 4 bytes
    MAX47x6ConfigurationStatus  volatileConfig;
    uint8_t                     volatileValue;
    MAX47x6ConfigurationStatus  nonVolatileConfig;
    uint8_t                     nonVolatileValue;
} __attribute__((packed)) MAX47x6Status8;

/**
 * @brief Class for the MAX47x6 I2C DAC
 *
 * - MAX4706 - 8 bit
 * - MAX4716 - 10 bit
 * - MAX4726 - 12 bit
 * 
 * Normally you create one of these as a global variable.
 *
 * Be sure to call the begin() method from setup(). 
 */
class MAX47x6 {
public:
    enum class Model {
        MAX4706,
        MAX4716,
        MAX4726,
    };

	/**
	 * @brief Construct the object
	 *
	 * @param addr The address. Can be 0 - 7 based on the address select pin and the normal base of60
	 *
	 * @param wire The I2C interface to use. Normally Wire, the primary I2C interface. Can be a
	 * different one on devices with more than one I2C interface.
	 * 
     * The MAX47x6 does not have an address selection bit and the address is determined by the SKU
     * that you purchase. 
     * - MCP47x6A0T-E/xx I2C address 0b1100000 = 0x60
     * - MCP47x6A1T-E/xx I2C address 0b1100001 = 0x61 
     * - MCP47x6A1T-E/xx I2C address 0b1100010 = 0x62 
     * ...
     * - MCP47x6A7T-E/xx I2C address 0b1100111 = 0x67
	 */
	MAX47x6(Model model, uint8_t addr = 0x38, TwoWire &wire = Wire);

	/**
	 * @brief Destructor. Not normally used as this is typically a globally instantiated object.
	 */
	virtual ~MAX47x6();


	/**
	 * @brief Set up the I2C device and begin running.
	 *
	 * You cannot do this from STARTUP or global object construction time. It should only be done from setup
	 * or loop (once).
	 *
	 * Make sure you set the LED current using withLEDCurrent() before calling begin if your LED has a
	 * current other than the default of 5 mA.
	 */
	bool begin();

    /**
     * @brief Update the DAC value 
     * 
     * Only affects the volatile value, does not affect the EEPROM value. Use updateEepromIfChanged()
     * if you want the DAC value saved and restore on power-on or reset.
     */
    bool updateValue(uint16_t value);

    /**
     * @brief Powers down this device
     * 
     * @param powerMode Normally one of POWERDOWN_1K, POWERDOWN_125K, or POWERDOWN_640K
     */
    bool powerdown(uint8_t powerMode);
    

    /**
     * @brief Wake all devices from sleep mode
     * 
     * This affects all MAX47x6 devices, not just the specific one addressed as it uses
     * the general call address, not the specific device address!
     */
    bool wakeup();


    /**
     * @brief Update settings and value in the EEPROM, only if the values change
     * 
     * @param vref voltage reference: VREF_VDD, VREF_VREF_UNBUFFERED, or VREF_VREF_BUFFERED
     * 
     * @param gain gain: GAIN_1X, GAIN_2X (only with VREF_VREF_UNBUFFERED, or VREF_VREF_BUFFERED)
     * 
     * @param value The value to set the DAC to (maximum value depends on which MAX47x6)
     * 
     * If the non-volatile values are the same, the operation completely quickly without wear 
     * or EEPROM write delays (normally 25 ms., could be up to 50 ms.).
     */
    bool updateEepromIfChanged(uint8_t vref, uint8_t gain, uint16_t value);

    /**
     * @brief Update settings and value, either in voltile settings, or both volatile and EEPROM
     * 
     * @param vref voltage reference: VREF_VDD, VREF_VREF_UNBUFFERED, or VREF_VREF_BUFFERED
     * 
     * @param gain gain: GAIN_1X, GAIN_2X (only with VREF_VREF_UNBUFFERED, or VREF_VREF_BUFFERED)
     * 
     * @param value The value to set the DAC to (maximum value depends on device)
     */
    bool updateSettings(uint8_t vref, uint8_t gain, uint16_t value, bool saveToEeprom);

    /**
     * @brief Read the device status
     * 
     * This is the only call that can be made to the MAX47x6 during an EEPROM write. Check
     * the ready flag to see if the write has completed.
     */
    MAX47x6Status readStatus();

    /**
     * @brief Checks to see if the device is ready (an EEPROM operation is not in progress)
     * 
     * Calls readStatus() internally.
     */
    bool isReady() { return readStatus().volatileConfig.ready; };

	
    static const uint8_t VREF_VDD                           = 0b00;     //!< VREF is VDD (gain is ignored)
    static const uint8_t VREF_VREF_UNBUFFERED               = 0b10;     //!< VREF is VREF pin, buffered
    static const uint8_t VREF_VREF_BUFFERED                 = 0b11;     //!< VREF is VREF pin, unbuffered

    static const uint8_t POWERDOWN_NORMAL                   = 0b00;     //!< Normal operation (not powered down)
    static const uint8_t POWERDOWN_1K                       = 0b01;     //!< Powered down, VOUT 1K resistor to ground
    static const uint8_t POWERDOWN_125K                     = 0b10;     //!< Powered down, VOUT 125K resistor to ground
    static const uint8_t POWERDOWN_640K                     = 0b11;     //!< Powered down, VOUT 640K resistor to ground
    
    static const uint8_t GAIN_1X                            = 0b0;      //!< Gain is 1x
    static const uint8_t GAIN_2X                            = 0b1;      //!< Gain is 2x, only when VREF is used


    static const uint8_t CMD_WRITE_VOLATILE_REG             = 0b000;    //!< Write volatile DAC register (PD bits and DAC value)
    static const uint8_t CMD_WRITE_VOLATILE_MEMORY          = 0b010;    //!< Write volatile memory
    static const uint8_t CMD_WRITE_ALL_MEMORY               = 0b011;    //!< Write all memory
    static const uint8_t CMD_WRITE_VOLATILE_CONFIG          = 0b100;    //!< Write volatile config bits
    
    static const unsigned long MAX_EEPROM_WAIT_MS           = 100;      //!< Maximum time to wait for an EEPROM write to complete (should take 50 ms max)

protected:
	/**
	 * @brief Low-level call to read from the device
	 *
     * @param buf Buffer to hold the data
     * 
	 * @param bufLen The number of bytes to read
     * 
     * The MCP47x6 has a weird read scheme where instead of specifying a register to read from
     * you just do a read and it gets all of the device data. The number of bytes varies
     * depending on the device: 
     * 
     * - MCP4706: 4 bytes
     * - MCP4716 and MCP4726: 6 bytes
	 */
	void readDevice(uint8_t *buf, size_t bufLen);

	/**
	 * @brief Low-level call to write to the device
	 *
     * @param buf Buffer to hold the data
     * 
	 * @param bufLen The number of bytes to read
	 */
	bool writeDevice(const uint8_t *buf, size_t bufLen);

    static void convert(const MAX47x6Status8 *from, MAX47x6Status *to);
    

protected:
    /**
     * @brief Which MAX47x6 model this device is
     */
    Model model;

	/**
	 * @brief The I2C address (0x00 - 0x7f). Default is 0x60.
	 *
	 * If you passed in an address 0 - 7 into the constructor, 0x60 - 0x6f is stored here.
	 */
	uint8_t addr;

	/**
	 * @brief The I2C interface to use. Default is Wire. Could be Wire1 or Wire3 on some devices.
	 */
	TwoWire &wire;

};


#endif /* __MAX47X6_RK_H */
