#pragma once

#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"

esp_err_t initi_sd_card(const char *mount_point, sdmmc_card_t **card);

