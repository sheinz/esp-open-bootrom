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

uint32_t IRAM SPI_read_data(sdk_flashchip_t *flashchip, uint32_t addr, 
        uint32_t *dst, uint32_t size)
{
    // a12 = dst
    if (flashchip->page_size < (addr + size)) {
        return 1; 
    }

    // a14 = addr
    // a13 = size
    Wait_SPI_Idle(flashchip);
    if (size < 1) {
        return 0; 
    }
    // SPI(0).CMD
    while (size >= 32) {
        // a8 = addr | 0x20000000;
        SPI(0).ADDR = addr | 0x20000000;
        SPI(0).CMD = 0x80000000;
        while (SPI(0).CMD) {};
        for (uint32_t a2 = 0; a2 < 8; a2++) {
            dst[a2] = (&(SPI(0).W0))[a2];
        }
        size -= 32;
        addr += 32;
    }

    if (size >= 1) {
        // a7 = size << 24;
        // a7 = addr | a7
        SPI(0).ADDR = addr | (size << 24);
        SPI(0).CMD = 0x80000000;
        while (SPI(0).CMD) {};
        // a10 = size & 0b11
        uint8_t a7 = size >> 2;
        // a9 = a7 + 1
        if (size & 0b11) {
           // a7 = a7 + 1 
           a7++;
        }
        // a7 = a7 & 0xFF
        if (!a7) {
            return 0;
        } 
        uint8_t a2 = 0;
        if (a7 & 0b1) {
            a2 = 1;
            // a11 = SPI(0).W0
            *dst = SPI(0).W0;
            dst += 1;
        }
        size = a7 >> 1;
        if (!size) {
            return 0;
        }
        for (; a2 != a7; a2++) {
            *dst = (&(SPI(0).W0))[a2];
            dst += 1;
        }
    }

    return 0;
}

