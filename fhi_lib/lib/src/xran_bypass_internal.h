#ifndef XRAN_BYPASS_INTERNAL_H
#define XRAN_BYPASS_INTERNAL_H

#include <stdint.h>

bool is_bypass_enabled(void);
void bypass_process_uplane(struct rte_mbuf *pkt,
                           struct xran_eaxc_info *p_cid,
                           uint16_t xport_id,
                           struct xran_sense_of_time *sense_of_time);
void bypass_process_cplane(struct rte_mbuf *pkt, uint16_t xport_id, struct xran_sense_of_time *sense_of_time);

#endif
