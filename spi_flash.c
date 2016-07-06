#include <stdint.h>

#include "esp/dport_regs.h"
#include "esp/spi_regs.h"
#include "espressif/spi_flash.h"


uint32_t IRAM Wait_SPI_Idle(sdk_flashchip_t *flashchip);
uint32_t IRAM SPI_write_enable(sdk_flashchip_t *flashchip);
uint32_t IRAM SPI_read_status(sdk_flashchip_t *flashchip, uint32_t *status);


uint32_t IRAM SPI_page_program(sdk_flashchip_t *flashchip, uint32_t dest_addr,
    uint32_t *buf, uint32_t size)
{
    if (size & 0b11) {
        return 1;
    }

    // check if block to write doesn't cross page boundary
    if (flashchip->page_size < size + (dest_addr % flashchip->page_size)) {
        return 1;
    }
    Wait_SPI_Idle(flashchip);
    if (size < 1) {
        return 0;
    }

    // a12 = 0x60000200
    // a0 =  0x00FFFFFF
    // a6 = (dest_addr & 0x00FFFFFF) | 0x20000000
    while (size >= 32) {
        SPI(0).ADDR = (dest_addr & 0x00FFFFFF) | 0x20000000;
        // a4 - loop variable += 4
        // a5 = buf[0]
        for (uint8_t i = 0; i != 8; i++) {
            (&(SPI(0).W0))[i] = buf[i];
        }
        size -= 32;
        dest_addr += 32;
        buf += 8;
        if (SPI_write_enable(flashchip)) {
            return 1;
        }
        SPI(0).CMD = 0x02000000;
        while (SPI(0).CMD) {}   // wait for reg->cmd to be 0
        Wait_SPI_Idle(flashchip); 
        // a0 = 0x00FFFFFF
        if (size < 1) {
            return 0;
        }
    }
    // a7 = 0x00FFFFFF & dest_addr
    // a4 = size << 24;
    // a4 = a7 | a4
    SPI(0).ADDR = (size << 24) | (0x00FFFFFF & dest_addr);
    // a6 = 0b11 & size
    // a3 = size >> 2;
    // a5 = a3 + 1
    uint32_t words = size >> 2;
    if (0b11 & size) {
        words += 1;
    }
    words = words & 0xFF;
    if (words != 0) {
        // a4 = 0
        uint8_t i = 0;

        if (words & 0b1) {  // bit 0 is set in a3
            SPI(0).W0 = buf[0];
            i++;
        }
        // a6 = a3 >> 1;
        if (words >> 1) {
            // a6 =  0x600000200
            // buff[0]
            for (; i != words; i++) {
                (&(SPI(0).W0))[i] = buf[i];
            } 
        }
    }
   
    if (SPI_write_enable(flashchip)) {
        return 1;
    }
    SPI(0).CMD = 0x02000000;
    while (SPI(0).CMD) {}   // wait for reg->cmd to be 0
    Wait_SPI_Idle(flashchip); 
    // a0 = 0x00FFFFFF
    return 0;
}

uint32_t IRAM SPI_write_enable(sdk_flashchip_t *flashchip)
{
    uint32_t local0 = 0; 

    Wait_SPI_Idle(flashchip);

    SPI(0).CMD = 0x40000000;
    while (SPI(0).CMD) {}

    if (!(local0 & 0b1)) {
        do {
            SPI_read_status(flashchip, &local0);
        } while (!(local0 & (1<<1)));
    }
    return 0;
}

uint32_t IRAM SPI_read_status(sdk_flashchip_t *flashchip, uint32_t *status)
{
    uint32_t _status;

    do {
        SPI(0).RSTATUS = 0;
        SPI(0).CMD = 0x08000000;
        while (SPI(0).CMD) {}
        _status = SPI(0).RSTATUS & flashchip->status_mask;
    } while ( _status & 0b1);

    *status = _status;

    return 0;
}

uint32_t IRAM Wait_SPI_Idle(sdk_flashchip_t *flashchip)
{
    while (DPORT.SPI_READY & DPORT_SPI_READY_IDLE) {}
    uint32_t a3;
    return SPI_read_status(flashchip, &a3);
}
