#include "sd_card.h"

esp_err_t initi_sd_card(const char *mount_point, sdmmc_card_t **card)
{  
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    esp_vfs_fat_sdmmc_mount_config_t mount_config
      = VFS_FAT_MOUNT_DEFAULT_CONFIG();
    mount_config.allocation_unit_size = 16 * 1024;
    
    return esp_vfs_fat_sdmmc_mount(
      mount_point, &host, &slot_config, &mount_config, card);
}
