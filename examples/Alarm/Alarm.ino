#include "GPIO.h"
#include "OWI.h"
#include "Driver/DS18B20.h"

// Configure: Software/Hardware OWI Bus Manager
#define USE_SOFTWARE_OWI
#if defined(USE_SOFTWARE_OWI)
#include "Software/OWI.h"
Software::OWI<BOARD::D7> owi;

#else
#include "TWI.h"
#include "Hardware/OWI.h"
// Configure: Software/Hardware TWI Bus Manager
// #define USE_SOFTWARE_TWI
#if defined(USE_SOFTWARE_TWI)
#include "Software/TWI.h"
#if defined(SAM)
Software::TWI<BOARD::D8,BOARD::D9> twi;
#else
Software::TWI<BOARD::D18,BOARD::D19> twi;
#endif
#else
#include "Hardware/TWI.h"
Hardware::TWI twi;
#endif
Hardware::OWI owi(twi);
#endif

DS18B20 sensor(owi);

#define ASSERT(expr)							\
  do {									\
    if (!(expr)) {							\
      Serial.print(__LINE__);						\
      Serial.println(F(":assert:" #expr));				\
      Serial.flush();							\
      exit(0);								\
    }									\
  } while (0)

void setup()
{
  Serial.begin(57600);
  while (!Serial);

  // Set sensor alarm triggers (20..25 C) and resolution (10 bits)
  // Iterate though all thermometers and configure.
  uint8_t* rom = sensor.rom();
  int8_t last = owi.FIRST;
  do {
    last = owi.search_rom(sensor.FAMILY_CODE, rom, last);
    if (last == owi.ERROR) break;
    sensor.resolution(10);
    sensor.set_trigger(20, 25);
    ASSERT(sensor.write_scratchpad());
  } while (last != owi.LAST);
}

void loop()
{
  // Check if any thermometer sersors have exceeded thresholds.
  // Broadcast a convert request to all thermometer sensors.
  // Print timestamp and sensor identity and temperature.

  int8_t last = owi.FIRST;
  uint8_t* rom = sensor.rom();
  static uint16_t timestamp = 0;
  uint8_t id = 0;
  if (!sensor.convert_request(true)) return;
  delay(sensor.conversion_time());
  do {
    last = owi.alarm_search(rom, last);
    if (last == owi.ERROR) break;
    ASSERT(sensor.read_scratchpad(false));
    Serial.print(timestamp);
    Serial.print('.');
    Serial.print(id++);
    Serial.print(F(":rom="));
    for (size_t i = 1; i < owi.ROM_MAX; i++) {
      if (rom[i] < 0x10) Serial.print(0);
      Serial.print(rom[i], HEX);
    }
    Serial.print(F(",temperature="));
    Serial.println(sensor.temperature());
  } while (last != owi.LAST);
  timestamp += 1;
  delay(1000);
}
