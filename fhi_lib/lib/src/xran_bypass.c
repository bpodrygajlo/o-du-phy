#include "xran_bypass.h"
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_mbuf.h>
#include "xran_pkt.h"
#include "xran_fh_o_du.h"
#include "xran_ethdi.h"
#include "xran_common.h"

#define MAX_PACKETS_SENT_PER_SYMBOL 16 // has to be power of 2

typedef struct {
  void *pHandle;
  int mu;
  int port;
  sym_ota_fn sym_ota_fn;
  void *sym_ota_fn_args;
} callback_args;

typedef struct {
  // Used to replace XRAN packet processing functions
  process_uplane_fn process_uplane_fn;
  void *process_uplane_fn_args;
  process_cplane_fn process_cplane_fn;
  void *process_cplane_fn_args;

  // Used to schedule TX in the future.
  struct rte_ring *tx_rings[XRAN_VF_MAX][XRAN_N_FE_BUF_LEN][XRAN_SYMBOLNUMBER_MAX];
  bool enabled;
} hook_cfg_t;

static hook_cfg_t hook_cfg = {0};

// Process the scheduled packets, done once per symbol at OTA time
void xran_hook_process_tx_packets(void *pHandle, int port, int slot, int symbol)
{
  struct xran_device_ctx *p_dev_ctx = (struct xran_device_ctx *)pHandle;
  int ring_index = slot % XRAN_N_FE_BUF_LEN;
  struct rte_ring *ring = hook_cfg.tx_rings[port][ring_index][symbol];
  void *mbufs[MAX_PACKETS_SENT_PER_SYMBOL];
  int dequeued = rte_ring_dequeue_burst(ring, mbufs, MAX_PACKETS_SENT_PER_SYMBOL, NULL);
  struct xran_ethdi_ctx *ctx = xran_ethdi_get_ctx();
  struct rte_ring *tx_ring = ctx->tx_ring[port];
  int enqueued = rte_ring_enqueue_burst(tx_ring, mbufs, dequeued, NULL);
  if (enqueued != dequeued) {
    rte_panic("Expected to be able to enqueue all the packets on the TX ring\n");
  }
}

int32_t send_packets(void *arg, struct xran_sense_of_time *p_sense_of_time)
{
  callback_args *cb_args = arg;
  int slots_per_subframe[XRAN_MAX_NUM_MU] = {1, 2, 4, 8, 16};
  int slot_in_frame = p_sense_of_time->nSlotIdx + p_sense_of_time->nSubframeIdx * slots_per_subframe[cb_args->mu];
  xran_hook_process_tx_packets(cb_args->pHandle, cb_args->port, slot_in_frame, p_sense_of_time->nSymIdx);
  if (cb_args->sym_ota_fn) {
    cb_args->sym_ota_fn(cb_args->sym_ota_fn_args, p_sense_of_time);
  }
  return 0;
}

void xran_hook_install(void *pHandle,
                       process_uplane_fn process_uplane_fn_p,
                       void *process_uplane_fn_args,
                       process_cplane_fn process_cplane_fn_p,
                       void *process_cplane_fn_args,
                       sym_ota_fn sym_ota_fn_p,
                       void *sym_ota_fn_args,
                       int mu)
{
  static bool installed = false;
  if (installed)
    rte_panic("Can only install once for one numerology\n");
  installed = true;

  struct xran_device_ctx *p_dev_ctx = (struct xran_device_ctx *)pHandle;
  hook_cfg.process_uplane_fn = process_uplane_fn_p;
  hook_cfg.process_uplane_fn_args = process_uplane_fn_args;
  hook_cfg.process_cplane_fn = process_cplane_fn_p;
  hook_cfg.process_cplane_fn_args = process_cplane_fn_args;

  struct xran_ethdi_ctx *ctx = xran_ethdi_get_ctx();
  for (int port = 0; port < ctx->io_cfg.num_vfs; port++) {
    int ring_index = 0;
    for (int slot = 0; slot < XRAN_N_FE_BUF_LEN; slot++) {
      for (int symbol = 0; symbol < N_SYM_PER_SLOT; symbol++) {
        char ring_name[RTE_RING_NAMESIZE];
        snprintf(ring_name, RTE_DIM(ring_name), "%s_%d_%d", "hook_tx_ring", port, ring_index++);
        struct rte_ring *ring = rte_ring_create(ring_name, MAX_PACKETS_SENT_PER_SYMBOL, SOCKET_ID_ANY, RING_F_SC_DEQ);
        if (ring == NULL)
          rte_panic("Cannot allocate rte_ring\n");
        hook_cfg.tx_rings[port][slot][symbol] = ring;
      }
    }
  }

  // Setup a callback each OTA symbol to send packets from per-symbol rings into the TX ring
  static struct xran_sense_of_time xran_sense_of_time[N_SYM_PER_SLOT];
  static callback_args cb_args = {0};
  cb_args.port = 0;
  cb_args.mu = mu;
  cb_args.pHandle = pHandle;
  cb_args.sym_ota_fn = sym_ota_fn_p;
  cb_args.sym_ota_fn_args = sym_ota_fn_args;
  for (int i = 0; i < N_SYM_PER_SLOT; i++) {
    xran_reg_sym_cb(pHandle, send_packets, &cb_args, &xran_sense_of_time[i], i, XRAN_CB_SYM_OTA_TIME, mu);
  }
}

int xran_hook_schedule_packet(void *pHandle,
                              struct rte_mbuf *mbuf,
                              int port,
                              enum xran_pkt_dir direction,
                              int ru_port_id,
                              int slot,
                              int symbol)
{
  void *ret = rte_pktmbuf_prepend(mbuf, sizeof(struct rte_ether_hdr));
  if (ret == NULL)
    rte_panic("not enough headroom for ethernet header");
  struct xran_device_ctx *p_dev_ctx = (struct xran_device_ctx *)pHandle;
  int vf_id = xran_map_ecpriPcid_to_vf(pHandle, direction, 0, ru_port_id);
  struct xran_ethdi_ctx *ctx = xran_ethdi_get_ctx();

  mbuf->port = ctx->io_cfg.port[vf_id];
  xran_add_eth_hdr_vlan(&ctx->entities[vf_id][ID_O_RU], ETHER_TYPE_ECPRI, mbuf);

  struct rte_ring *ring = hook_cfg.tx_rings[port][slot][symbol];
  int res = rte_ring_enqueue(ring, mbuf);
  if (res != 0)
    return -1;
  return 0;
}

int xran_hook_send_packet(void *pHandle, struct rte_mbuf *mbuf, int port, enum xran_pkt_dir direction, int ru_port_id)
{
  void *ret = rte_pktmbuf_prepend(mbuf, sizeof(struct rte_ether_hdr));
  if (ret == NULL)
    rte_panic("not enough headroom for ethernet header");
  int vf_id = xran_map_ecpriPcid_to_vf(pHandle, direction, 0, ru_port_id);
  if (xran_ethdi_mbuf_send(mbuf, ETHER_TYPE_ECPRI, vf_id) == 1) {
    return 0;
  }
  return -1;
}

bool is_bypass_enabled(void)
{
  return hook_cfg.enabled;
}

void bypass_process_uplane(struct rte_mbuf *pkt,
                           struct xran_eaxc_info *p_cid,
                           uint16_t xport_id,
                           struct xran_sense_of_time *sense_of_time)
{
  if (hook_cfg.process_uplane_fn) {
    hook_cfg.process_uplane_fn(pkt, hook_cfg.process_uplane_fn_args, p_cid, xport_id, sense_of_time);
  }
}

void bypass_process_cplane(struct rte_mbuf *pkt, uint16_t xport_id, struct xran_sense_of_time *sense_of_time)
{
  if (hook_cfg.process_cplane_fn) {
    hook_cfg.process_cplane_fn(pkt, hook_cfg.process_cplane_fn_args, xport_id, sense_of_time);
  }
}
