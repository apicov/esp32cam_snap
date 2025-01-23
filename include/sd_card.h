#pragma once

#include <esp_err.h>
#include "driver/sdmmc_host.h"
#include "driver/sdmmc_defs.h"
#include "sdmmc_cmd.h"
#include "esp_vfs_fat.h"

esp_err_t initi_sd_card(void);

