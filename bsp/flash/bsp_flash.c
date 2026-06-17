#include "bsp_flash.h"

#include "string.h"

static uint32_t flash_page_start(uint32_t address)
{
    return address & ~(BSP_FLASH_PAGE_SIZE - 1U);
}

void flash_erase_address(uint32_t address, uint16_t len)
{
    uint32_t page = flash_page_start(address);

    for (uint16_t i = 0U; i < len; ++i) {
        uint32_t page_addr = page + ((uint32_t) i * BSP_FLASH_PAGE_SIZE);
        if (page_addr >= FLASH_END_ADDR) {
            return;
        }

        DL_FlashCTL_executeClearStatus(FLASHCTL);
        DL_FlashCTL_unprotectSector(
            FLASHCTL, page_addr, DL_FLASHCTL_REGION_SELECT_MAIN);
        (void) DL_FlashCTL_eraseMemoryFromRAM(
            FLASHCTL, page_addr, DL_FLASHCTL_COMMAND_SIZE_SECTOR);
    }
}

int8_t flash_write_single_address(
    uint32_t start_address, uint32_t *buf, uint32_t len)
{
    uint32_t end_address = get_next_flash_address(start_address);
    uint32_t address = start_address;

    for (uint32_t i = 0U; (i < len) && (address < end_address); ++i) {
        DL_FlashCTL_executeClearStatus(FLASHCTL);
        DL_FlashCTL_unprotectSector(
            FLASHCTL, address, DL_FLASHCTL_REGION_SELECT_MAIN);

        if (DL_FlashCTL_programMemoryFromRAM32(FLASHCTL, address, &buf[i]) !=
            DL_FLASHCTL_COMMAND_STATUS_PASSED) {
            return -1;
        }

        address += sizeof(uint32_t);
    }

    return 0;
}

int8_t flash_write_muli_address(
    uint32_t start_address, uint32_t end_address, uint32_t *buf, uint32_t len)
{
    uint32_t address = start_address;

    for (uint32_t i = 0U; (i < len) && (address <= end_address); ++i) {
        DL_FlashCTL_executeClearStatus(FLASHCTL);
        DL_FlashCTL_unprotectSector(
            FLASHCTL, address, DL_FLASHCTL_REGION_SELECT_MAIN);

        if (DL_FlashCTL_programMemoryFromRAM32(FLASHCTL, address, &buf[i]) !=
            DL_FLASHCTL_COMMAND_STATUS_PASSED) {
            return -1;
        }

        address += sizeof(uint32_t);
    }

    return 0;
}

void flash_read(uint32_t address, uint32_t *buf, uint32_t len)
{
    memcpy(buf, (void *) address, len * sizeof(uint32_t));
}

uint32_t get_next_flash_address(uint32_t address)
{
    uint32_t next = flash_page_start(address) + BSP_FLASH_PAGE_SIZE;

    if (next > FLASH_END_ADDR) {
        return FLASH_END_ADDR;
    }

    return next;
}