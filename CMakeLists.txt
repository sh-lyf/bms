# Copyright (C) 2018 LongSung Technologies Limited and/or its affiliates("LongSung").
# All rights reserved.
#
# This software is supplied "AS IS" without any warranties.
# LongSung assumes no responsibility or liability for the use of the software,
# conveys no license or title under any patent, copyright, or mask work
# right to the product. LongSung reserves the right to make changes in the
# software without notification.  LongSung also make no representation or
# warranty that such application will be suitable for the specified use
# without further testing or modification.

set(source_dir ${SOURCE_TOP_DIR}/bms_app)

include_directories(${source_dir}/include/)
include_directories(${source_dir}/src/)

#nanopbgen(${source_dir}/src/at_setting.proto)

SET(SRC_LIST       ${source_dir}/src/bms_main.c
					${source_dir}/src/bms_checksum.c
					${source_dir}/src/bms_gps_parser.c
					${source_dir}/src/bms_mqtt.c
					${source_dir}/src/bms_ring.c
					${source_dir}/src/any.pb.c
					${source_dir}/src/LibSafeTrackerMessage.pb.c
					${source_dir}/src/sc7a20_i2c_spi.c
					${source_dir}/src/SL_Watch_Algorithm_driver.c
					${source_dir}/src/sc7a20_main.c
)

SET(SRC_INC       ${source_dir}/include/bms_main.h
					${source_dir}/include/bms_checksum.h
					${source_dir}/include/bms_gps_parser.h
					${source_dir}/include/bms_info.h
					${source_dir}/include/bms_mqtt.h
					${source_dir}/include/bms_ring.h
					${source_dir}/include/bms_tracker_protocol.h
					${source_dir}/include/any.pb.h
					${source_dir}/include/LibSafeTrackerMessage.pb.h
					${source_dir}/include/sc7a20_i2c_spi.h
					${source_dir}/include/SL_Watch_Algorithm_driver.h
					
)

if(CONFIG_APPIMG_LOAD_FLASH)
    set(target bms_app)
    add_appimg(${target} ${flash_ldscript}  ${SRC_LIST} ${SRC_INC})
    target_link_libraries(${target} PRIVATE ${libc_file_name} ${libm_file_name} ${libgcc_file_name})

    set(pac_file ${out_hex_dir}/${target}.pac)
    set(target_img_file ${out_hex_dir}/${target}.img)
    pac_init_fdl(init_fdl)
    add_custom_command(OUTPUT ${pac_file}
        COMMAND python3 ${pacgen_py} ${init_fdl}
            cfg-image -i APPIMG -a ${CONFIG_APPIMG_FLASH_ADDRESS}
                -s ${CONFIG_APPIMG_FLASH_SIZE}
                -p ${target_img_file}
            pac-gen ${pac_file}
        DEPENDS ${pacgen_py} ${target_img_file}
        WORKING_DIRECTORY ${SOURCE_TOP_DIR}
    )
    add_custom_target(${target}_pacgen ALL DEPENDS ${pac_file})
endif()

