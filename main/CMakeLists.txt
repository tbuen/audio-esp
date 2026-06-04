idf_component_register(SRCS "main.c" "rpc.c" "rpc_handler.c" "rpc_json.c"
                       INCLUDE_DIRS "")

fatfs_create_spiflash_image(storage ../spiflash_image FLASH_IN_PROJECT)
