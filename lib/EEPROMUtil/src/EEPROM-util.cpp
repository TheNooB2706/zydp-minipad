#include <EEPROM-util.hpp>

void write_uint32(uint16 addr, uint32 data) {
  uint16 *ptr = (uint16 *)&data;

  for (size_t i=0; i<2; i++) {
    EEPROM.update(addr++, *(ptr++));
  }
}

uint32 read_uint32(uint16 addr) {
  uint32 result;

  uint16 *ptr = (uint16 *)&result;

  for (size_t i=0; i<2; i++) {
    *(ptr++) = EEPROM.read(addr++);
  }

  return result;
}