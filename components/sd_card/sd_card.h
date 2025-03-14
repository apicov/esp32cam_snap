#pragma once

#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"

/**
 * @brief Mounts an sd card to the vfs
 *
 * @param mount_point string where the card is to be mounted
 * @param card object to be mounted
 *
 * @return an esp_err_t
 */
esp_err_t initi_sd_card(const char *mount_point, sdmmc_card_t **card);
