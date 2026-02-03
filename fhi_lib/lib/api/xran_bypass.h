#ifndef XRAN_BYPASS_H
#define XRAN_BYPASS_H

#include <stdint.h>
#include "xran_pkt.h"
#include "xran_fh_o_du.h"
#include "xran_cp_api.h"

typedef void (*process_uplane_fn)(struct rte_mbuf *pkt,
                                  void *handle,
                                  struct xran_eaxc_info *p_cid,
                                  uint16_t xport_id,
                                  struct xran_sense_of_time *sense_of_time);
typedef void (*process_cplane_fn)(struct rte_mbuf *pkt, void *handle, uint16_t xport_id, struct xran_sense_of_time *sense_of_time);
typedef void (*sym_ota_fn)(void *handle, struct xran_sense_of_time *sense_of_time);

/**
 * @brief Installs user-defined hooks for processing U-plane, C-plane, and OTA symbols in the xRAN stack.
 *
 * @param pHandle Pointer to the xRAN handle.
 * @param process_uplane_fn_p Function pointer for U-plane packet processing.
 * @param process_uplane_fn_args Arguments for the U-plane processing function.
 * @param process_cplane_fn_p Function pointer for C-plane packet processing.
 * @param process_cplane_fn_args Arguments for the C-plane processing function.
 * @param sym_ota_fn_p Function pointer for OTA symbol processing.
 * @param sym_ota_fn_args Arguments for the OTA symbol processing function.
 * @param mu Numerology
 */
void xran_hook_install(void *pHandle,
                       process_uplane_fn process_uplane_fn_p,
                       void *process_uplane_fn_args,
                       process_cplane_fn process_cplane_fn_p,
                       void *process_cplane_fn_args,
                       sym_ota_fn sym_ota_fn_p,
                       void *sym_ota_fn_args,
                       int mu);
/**
 * @brief Schedules a packet for transmission or processing in the xRAN stack.
 *
 * @param pHandle Pointer to the xRAN handle.
 * @param mbuf Pointer to the packet buffer (mbuf).
 * @param port Port number.
 * @param direction Packet direction (uplink or downlink).
 * @param ru_port_id RU port identifier.
 * @param slot Slot number.
 * @param symbol Symbol number.
 * @return int      Status code (0 for success, negative for error).
 */
int xran_hook_schedule_packet(void *pHandle,
                              struct rte_mbuf *mbuf,
                              int port,
                              enum xran_pkt_dir direction,
                              int ru_port_id,
                              int slot,
                              int symbol);
/**
 * @brief Sends a packet immediately through the xRAN stack.
 *
 * @param pHandle Pointer to the xRAN handle.
 * @param mbuf Pointer to the packet buffer (mbuf).
 * @param port Port number.
 * @param direction Packet direction (uplink or downlink).
 * @param ru_port_id RU port identifier.
 * @return int      Status code (0 for success, negative for error).
 */
int xran_hook_send_packet(void *pHandle, struct rte_mbuf *mbuf, int port, enum xran_pkt_dir direction, int ru_port_id);

#endif
