#ifndef _BSP_FLASH_H
#define _BSP_FLASH_H
#include "main.h"

/* Base address of the Flash sectors */
#define BSP_FLASH_BASE_ADDR ((uint32_t)0x00000000)
#define BSP_FLASH_SIZE      ((uint32_t)0x00020000)
#define BSP_FLASH_PAGE_SIZE ((uint32_t)DL_FLASHCTL_SECTOR_SIZE)

#define ADDR_FLASH_SECTOR_0  (BSP_FLASH_BASE_ADDR + (0U * BSP_FLASH_PAGE_SIZE))
#define ADDR_FLASH_SECTOR_1  (BSP_FLASH_BASE_ADDR + (1U * BSP_FLASH_PAGE_SIZE))
#define ADDR_FLASH_SECTOR_2  (BSP_FLASH_BASE_ADDR + (2U * BSP_FLASH_PAGE_SIZE))
#define ADDR_FLASH_SECTOR_3  (BSP_FLASH_BASE_ADDR + (3U * BSP_FLASH_PAGE_SIZE))
#define ADDR_FLASH_SECTOR_4  (BSP_FLASH_BASE_ADDR + (4U * BSP_FLASH_PAGE_SIZE))
#define ADDR_FLASH_SECTOR_5  (BSP_FLASH_BASE_ADDR + (5U * BSP_FLASH_PAGE_SIZE))
#define ADDR_FLASH_SECTOR_6  (BSP_FLASH_BASE_ADDR + (6U * BSP_FLASH_PAGE_SIZE))
#define ADDR_FLASH_SECTOR_7  (BSP_FLASH_BASE_ADDR + (7U * BSP_FLASH_PAGE_SIZE))
#define ADDR_FLASH_SECTOR_8  (BSP_FLASH_BASE_ADDR + (8U * BSP_FLASH_PAGE_SIZE))
#define ADDR_FLASH_SECTOR_9  (BSP_FLASH_BASE_ADDR + (9U * BSP_FLASH_PAGE_SIZE))
#define ADDR_FLASH_SECTOR_10 (BSP_FLASH_BASE_ADDR + (10U * BSP_FLASH_PAGE_SIZE))
#define ADDR_FLASH_SECTOR_11 (BSP_FLASH_BASE_ADDR + (11U * BSP_FLASH_PAGE_SIZE))
#define ADDR_FLASH_SECTOR_12 (BSP_FLASH_BASE_ADDR + (12U * BSP_FLASH_PAGE_SIZE))
#define ADDR_FLASH_SECTOR_13 (BSP_FLASH_BASE_ADDR + (13U * BSP_FLASH_PAGE_SIZE))
#define ADDR_FLASH_SECTOR_14 (BSP_FLASH_BASE_ADDR + (14U * BSP_FLASH_PAGE_SIZE))
#define ADDR_FLASH_SECTOR_15 (BSP_FLASH_BASE_ADDR + (15U * BSP_FLASH_PAGE_SIZE))
#define ADDR_FLASH_SECTOR_16 (BSP_FLASH_BASE_ADDR + (16U * BSP_FLASH_PAGE_SIZE))
#define ADDR_FLASH_SECTOR_17 (BSP_FLASH_BASE_ADDR + (17U * BSP_FLASH_PAGE_SIZE))
#define ADDR_FLASH_SECTOR_18 (BSP_FLASH_BASE_ADDR + (18U * BSP_FLASH_PAGE_SIZE))
#define ADDR_FLASH_SECTOR_19 (BSP_FLASH_BASE_ADDR + (19U * BSP_FLASH_PAGE_SIZE))
#define ADDR_FLASH_SECTOR_20 (BSP_FLASH_BASE_ADDR + (20U * BSP_FLASH_PAGE_SIZE))
#define ADDR_FLASH_SECTOR_21 (BSP_FLASH_BASE_ADDR + (21U * BSP_FLASH_PAGE_SIZE))
#define ADDR_FLASH_SECTOR_22 (BSP_FLASH_BASE_ADDR + (22U * BSP_FLASH_PAGE_SIZE))
#define ADDR_FLASH_SECTOR_23 (BSP_FLASH_BASE_ADDR + (23U * BSP_FLASH_PAGE_SIZE))

#define FLASH_END_ADDR (BSP_FLASH_BASE_ADDR + BSP_FLASH_SIZE)


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
