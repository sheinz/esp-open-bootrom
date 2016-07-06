# esp-open-bootrom
Reverse engineered bootrom of  ESP8266

This is an attempt to reverse engineer some of ESP8266 bootrom functions.
The result is written in plain C where possible.
The C code uses [esp-open-rtos](https://github.com/SuperHouse/esp-open-rtos) framework.

### Functions

|   Address  | Function                                              | Status  |
|------------|-------------------------------------------------------|---------|
| 0x40004174 | [SPI_page_program](https://github.com/sheinz/esp-open-bootrom/blob/master/spi_flash.c#L13) | tested |
| 0x4000443c | [SPI_write_enable](https://github.com/sheinz/esp-open-bootrom/blob/master/spi_flash.c#L93) | tested |
| 0x4000448c | [Wait_SPI_Idle](https://github.com/sheinz/esp-open-bootrom/blob/master/spi_flash.c#L126)   | tested |
| 0x400043c8 | [SPI_read_status](https://github.com/sheinz/esp-open-bootrom/blob/master/spi_flash.c#L110) | tested |

### Resources
- [xtobjdis](https://bitbucket.org/foogod/xtobjdis) Xtenas disassembler.
- [Xtensa® Instruction Set Architecture (ISA)](https://www.google.com.ua/search?q=xtensa+instruction+set)
- [Annotated bootrom disassembly](http://cholla.mmto.org/esp8266/bootrom/)
