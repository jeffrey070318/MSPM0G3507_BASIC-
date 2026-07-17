#ifndef _BSP_FLASH_H
#define _BSP_FLASH_H
#include "bsp_def.h"

/* Base address of the Flash sectors */
#define BSP_FLASH_BASE_ADDR ((uint32_t)0x00000000)
#define BSP_FLASH_SIZE      ((uint32_t)0x00020000)
#define BSP_FLASH_PAGE_SIZE ((uint32_t)DL_FLASHCTL_SECTOR_SIZE)

#ifndef BSP_FLASH_STORAGE_START_ADDR
#define BSP_FLASH_STORAGE_START_ADDR ((uint32_t)0x0001E000)
#endif

#ifndef BSP_FLASH_STORAGE_PAGE_COUNT
#define BSP_FLASH_STORAGE_PAGE_COUNT ((uint32_t)8U)
#endif
#define BSP_FLASH_STORAGE_SIZE \
    (BSP_FLASH_STORAGE_PAGE_COUNT * BSP_FLASH_PAGE_SIZE)
#define BSP_FLASH_STORAGE_END_ADDR \
    (BSP_FLASH_STORAGE_START_ADDR + BSP_FLASH_STORAGE_SIZE)

#define ADDR_FLASH_SECTOR_0 \
    (BSP_FLASH_STORAGE_START_ADDR + (0U * BSP_FLASH_PAGE_SIZE))
#define ADDR_FLASH_SECTOR_1 \
    (BSP_FLASH_STORAGE_START_ADDR + (1U * BSP_FLASH_PAGE_SIZE))
#define ADDR_FLASH_SECTOR_2 \
    (BSP_FLASH_STORAGE_START_ADDR + (2U * BSP_FLASH_PAGE_SIZE))
#define ADDR_FLASH_SECTOR_3 \
    (BSP_FLASH_STORAGE_START_ADDR + (3U * BSP_FLASH_PAGE_SIZE))
#define ADDR_FLASH_SECTOR_4 \
    (BSP_FLASH_STORAGE_START_ADDR + (4U * BSP_FLASH_PAGE_SIZE))
#define ADDR_FLASH_SECTOR_5 \
    (BSP_FLASH_STORAGE_START_ADDR + (5U * BSP_FLASH_PAGE_SIZE))
#define ADDR_FLASH_SECTOR_6 \
    (BSP_FLASH_STORAGE_START_ADDR + (6U * BSP_FLASH_PAGE_SIZE))
#define ADDR_FLASH_SECTOR_7 \
    (BSP_FLASH_STORAGE_START_ADDR + (7U * BSP_FLASH_PAGE_SIZE))

#define FLASH_END_ADDR (BSP_FLASH_BASE_ADDR + BSP_FLASH_SIZE)

typedef char bsp_flash_storage_start_must_be_sector_aligned[
    ((BSP_FLASH_STORAGE_START_ADDR % BSP_FLASH_PAGE_SIZE) == 0U) ? 1 : -1];
typedef char bsp_flash_storage_must_end_at_flash_boundary[
    (BSP_FLASH_STORAGE_END_ADDR == FLASH_END_ADDR) ? 1 : -1];


/**
  * @brief          erase flash
  * @param[in]      address: flash address
  * @param[in]      len: page num
  * @retval         none
  */
void flash_erase_address(uint32_t address, uint16_t len);

/**
  * @brief          write data to one page of flash
  * @param[in]      start_address: flash address
  * @param[in]      buf: data point
  * @param[in]      len: data num
  * @retval         success 0, fail -1
  */
int8_t flash_write_single_address(uint32_t start_address, uint32_t *buf, uint32_t len);


/**
  * @brief          write data to some pages of flash
  * @param[in]      start_address: flash start address
  * @param[in]      end_address: flash end address
  * @param[in]      buf: data point
  * @param[in]      len: data num
  * @retval         success 0, fail -1
  */
int8_t flash_write_muli_address(uint32_t start_address, uint32_t end_address, uint32_t *buf, uint32_t len);

/**
  * @brief          read data for flash
  * @param[in]      address: flash address
  * @param[out]     buf: data point
  * @param[in]      len: data num
  * @retval         none
  */
void flash_read(uint32_t address, uint32_t *buf, uint32_t len);

/**
  * @brief          get the next page flash address
  * @param[in]      address: flash address
  * @retval         next page flash address
  */
uint32_t get_next_flash_address(uint32_t address);
#endif
