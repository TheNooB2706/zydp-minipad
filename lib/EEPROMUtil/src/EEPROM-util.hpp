#define EEPROM_PAGE_SIZE (uint16)0x400  /* Page size = 1KByte */
#define EEPROM_START_ADDRESS ((uint32)(0x8000000 + 64 * 1024 - 2 * EEPROM_PAGE_SIZE))
// Definition of page size and start address for STM32F103C8, from https://github.com/rogerclarkmelbourne/Arduino_STM32/pull/388#issuecomment-347013537

#include <EEPROM.h>

/// @brief Write a uint32 variable into the virtual EEPROM
/// @param addr Virtual address of the EEPROM
/// @param data Data to be written
void write_uint32(uint16 addr, uint32 data);

/// @brief Read a uint32 variable from the virtual EEPROM
/// @param addr Virtual address of the EEPROM
/// @return Data retrieved from the EEPROM
uint32 read_uint32(uint16 addr);

