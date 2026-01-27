cmake_minimum_required(VERSION 3.10)
project(xran C CXX)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DPDK REQUIRED libdpdk)

set(XRAN_SOURCES
    ethernet/xran_ethdi.c
    ethernet/xran_ethernet.c
    src/xran_up_api.c
    src/xran_sync_api.c
    src/xran_timer.c
    src/xran_cp_api.c
    src/xran_transport.c
    src/xran_common.c
    src/xran_ul_tables.c
    src/xran_frame_struct.c
    src/xran_dev.c
    src/xran_rx_proc.c
    src/xran_tx_proc.c
    src/xran_cp_proc.c
    src/xran_cb_proc.c
    src/xran_mem_mgr.c
    src/xran_main.c
    src/xran_delay_measurement.c
    src/xran_compression.cpp
    src/xran_bfp_ref.cpp
    src/xran_bfp_cplane8.cpp
    src/xran_bfp_cplane16.cpp
    src/xran_bfp_cplane32.cpp
    src/xran_bfp_cplane64.cpp
    src/xran_bfp_uplane.cpp
    # src/xran_mod_compression.cpp
    src/xran_compression_snc.cpp
    src/xran_bfp_cplane8_snc.cpp
    src/xran_bfp_cplane16_snc.cpp
    src/xran_bfp_cplane32_snc.cpp
    src/xran_bfp_cplane64_snc.cpp
    src/xran_bfp_uplane_snc.cpp
    src/xran_bfp_uplane_spr.cpp
)

if(XRAN_LIB_SO)
    add_library(xran SHARED ${XRAN_SOURCES})
else()
    add_library(xran STATIC ${XRAN_SOURCES})
endif()

target_include_directories(xran PUBLIC
    api
    ethernet
    src
    ${DPDK_INCLUDE_DIRS}
)

target_link_libraries(xran PUBLIC
    ${DPDK_LIBRARIES}
)
