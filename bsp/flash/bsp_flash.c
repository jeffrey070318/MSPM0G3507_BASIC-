#include "bsp_flash.h"

#include "string.h"

static uint32_t flash_page_start(uint32_t address)
{
    return address & ~(BSP_FLASH_PAGE_SIZE - 1U);
}

static bool flash_word_range_valid(uint32_t address, uint32_t word_count)
{
    if ((address < BSP_FLASH_STORAGE_START_ADDR) ||
        (address >= BSP_FLASH_STORAGE_END_ADDR) ||
        ((address % sizeof(uint32_t)) != 0U)) {
        return false;
    }

    return word_count <=
        ((BSP_FLASH_STORAGE_END_ADDR - address) / sizeof(uint32_t));
}

void flash_erase_address(uint32_t address, uint16_t len)
{
    uint32_t page = flash_page_start(address);

    if ((len == 0U) || (address < BSP_FLASH_STORAGE_START_ADDR) ||
        (address >= BSP_FLASH_STORAGE_END_ADDR) ||
        ((uint32_t) len >
            ((BSP_FLASH_STORAGE_END_ADDR - page) / BSP_FLASH_PAGE_SIZE))) {
        return;
    }

    for (uint16_t i = 0U; i < len; ++i) {
        uint32_t page_addr = page + ((uint32_t) i * BSP_FLASH_PAGE_SIZE);

        DL_FlashCTL_executeClearStatus(FLASHCTL);
        DL_FlashCTL_unprotectSector(
            FLASHCTL, page_addr, DL_FLASHCTL_REGION_SELECT_MAIN);
        if (DL_FlashCTL_eraseMemoryFromRAM(FLASHCTL, page_addr,
                DL_FLASHCTL_COMMAND_SIZE_SECTOR) !=
            DL_FLASHCTL_COMMAND_STATUS_PASSED) {
            return;
        }
    }
}

int8_t flash_write_single_address(
    uint32_t start_address, uint32_t *buf, uint32_t len)
{
    uint32_t end_address = get_next_flash_address(start_address);
    uint32_t address = start_address;

    if ((buf == NULL) || !flash_word_range_valid(start_address, len) ||
        (len > ((end_address - start_address) / sizeof(uint32_t)))) {
        return -1;
    }

    for (uint32_t i = 0U; i < len; ++i) {
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

    if ((buf == NULL) || (end_address < start_address) ||
        !flash_word_range_valid(start_address, len) ||
        (end_address >= BSP_FLASH_STORAGE_END_ADDR) ||
        ((end_address % sizeof(uint32_t)) != 0U) ||
        (len > (((end_address - start_address) / sizeof(uint32_t)) + 1U))) {
        return -1;
    }

    for (uint32_t i = 0U; i < len; ++i) {
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
    if ((buf == NULL) || !flash_word_range_valid(address, len)) {
        return;
    }

    memcpy(buf, (void *) address, len * sizeof(uint32_t));
}

uint32_t get_next_flash_address(uint32_t address)
{
    if ((address < BSP_FLASH_STORAGE_START_ADDR) ||
        (address >= BSP_FLASH_STORAGE_END_ADDR)) {
        return BSP_FLASH_STORAGE_END_ADDR;
    }

    return flash_page_start(address) + BSP_FLASH_PAGE_SIZE;
}
