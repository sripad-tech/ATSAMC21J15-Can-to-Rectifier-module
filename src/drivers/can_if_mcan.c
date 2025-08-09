/*
 * Project: ATSAMC21J15 – Rectifier CAN Interface (Classic CAN, 29-bit EID)
 * Author:  Sripad Madhusudan Upadhyaya
 * License: MIT
 * Year:    2025
 */
#include "drivers/can_if.h"
#include "config/build_config.h"
#if CAN_BACKEND_MCAN
#include "atmel_start.h"
#include "hpl_mcan_base.h"

#define MCAN_INST   MCAN0
#define MCAN_IRQ_N  MCAN0_IRQn

typedef struct { uint32_t eid; uint8_t dlc; uint8_t data[8]; } rx_frame_t;
#define RXQ_SZ 8
static volatile rx_frame_t rxq[RXQ_SZ];
static volatile uint8_t qh, qt, qcount;

static inline void busy_wait_us(uint32_t us) { while (us--) for (volatile uint32_t i=0;i<4;i++) __asm__ __volatile__("nop"); }

static inline uint8_t _ds_to_bytes(uint8_t code){
    switch(code & 0x7){
        case 0: return 8; case 1: return 12; case 2: return 16; case 3: return 20;
        case 4: return 24; case 5: return 32; case 6: return 48; case 7: return 64;
    } return 8;
}
static inline uint8_t _dlc_to_len(uint8_t dlc){ return (dlc <= 8) ? dlc : 8; }

void can_init_100k(void)
{
    hri_mcan_set_CCCR_INIT_bit(MCAN_INST); while(!hri_mcan_get_CCCR_INIT_bit(MCAN_INST)){}; hri_mcan_set_CCCR_CCE_bit(MCAN_INST);
    hri_mcan_write_NBTP_reg(MCAN_INST, NBTP_4MHZ_100K);
    hri_mcan_clear_CCCR_FDOE_bit(MCAN_INST); hri_mcan_clear_CCCR_BRSE_bit(MCAN_INST);

    // RX FIFO0 new msg interrupt
    hri_mcan_write_IE_reg(MCAN_INST, MCAN_IE_RF0NE);
    hri_mcan_write_ILS_reg(MCAN_INST, 0x0);
    hri_mcan_write_ILE_reg(MCAN_INST, MCAN_ILE_EINT0);

    hri_mcan_clear_CCCR_INIT_bit(MCAN_INST); while(hri_mcan_get_CCCR_INIT_bit(MCAN_INST)){};
    NVIC_ClearPendingIRQ(MCAN_IRQ_N); NVIC_EnableIRQ(MCAN_IRQ_N);
}

bool can_set_ext_filter_mask(uint8_t idx, const can_filter_ext_mask_t *f)
{
    (void)idx;
    uint32_t inv = ~f->mask;
    hri_mcan_set_CCCR_INIT_bit(MCAN_INST); while(!hri_mcan_get_CCCR_INIT_bit(MCAN_INST)){}; hri_mcan_set_CCCR_CCE_bit(MCAN_INST);
    hri_mcan_write_XIDAM_reg(MCAN_INST, inv & 0x1FFFFFFF);
    hri_mcan_clear_CCCR_INIT_bit(MCAN_INST); while(hri_mcan_get_CCCR_INIT_bit(MCAN_INST)){};
    return true;
}

bool can_tx_ext(uint32_t eid, const uint8_t *data, uint8_t dlc)
{
    if (dlc > 8) dlc = 8;

    uint32_t txfqs = hri_mcan_read_TXFQS_reg(MCAN_INST);
    if (txfqs & MCAN_TXFQS_TFQF) { return false; } // FIFO/Queue full
    uint8_t idx = (uint8_t)((txfqs & MCAN_TXFQS_TFQPI_Msk) >> MCAN_TXFQS_TFQPI_Pos);

    uint32_t txbc = hri_mcan_read_TXBC_reg(MCAN_INST);
    uint32_t tbsa = (txbc & MCAN_TXBC_TBSA_Msk) >> MCAN_TXBC_TBSA_Pos; // word address
    uint32_t txesc = hri_mcan_read_TXESC_reg(MCAN_INST);
    uint8_t tbds = (uint8_t)(txesc & MCAN_TXESC_TBDS_Msk);
    uint8_t bytes = _ds_to_bytes(tbds);
    uint32_t stride_words = 2u + ((bytes + 3u) >> 2);
    volatile uint32_t *elem = (volatile uint32_t *)((tbsa + (uint32_t)idx * stride_words) * 4u);

    // T0/T1 (Classic frame, XTD=1, RTR=0, FDF=0, BRS=0)
    uint32_t T0 = (1u << 30) | (0u << 29) | (eid & 0x1FFFFFFFu);
    uint32_t T1 = ((uint32_t)dlc & 0xFu) << 16;
    elem[0] = T0; elem[1] = T1;

    // Copy payload
    uint32_t w = 0; uint8_t i = 0; uint8_t out = 0;
    for (; i < dlc; i++) { w |= ((uint32_t)data[i]) << (8u * (i & 3u)); if ((i & 3u) == 3u) { elem[2 + out] = w; w = 0; out++; } }
    if ((i & 3u) != 0) { elem[2 + out] = w; out++; }
    for (; out < ((bytes + 3u) >> 2); out++) elem[2 + out] = 0;

    // Request transmission
    hri_mcan_write_TXBAR_reg(MCAN_INST, (uint32_t)1u << idx);

    // Optionally wait for TXBTO
    for (uint32_t t=0; t<100000; t++) { if (hri_mcan_read_TXBTO_reg(MCAN_INST) & ((uint32_t)1u << idx)) return true; }
    return true; // queued
}

bool can_rx_wait(uint32_t *eid, uint8_t *data, uint8_t *dlc, uint32_t timeout_ms)
{
    uint32_t t=0; while(qcount==0){ if(t++>=timeout_ms) return false; busy_wait_us(1000); }
    uint8_t n = qt;
    *eid = rxq[n].eid; *dlc = rxq[n].dlc;
    for (uint8_t i=0;i<*dlc;i++) data[i] = rxq[n].data[i];
    qt = (qt + 1) & (RXQ_SZ-1);
    __atomic_fetch_sub((volatile int*)&qcount, 1, __ATOMIC_RELAXED);
    return true;
}

void can_sleep_ms(uint32_t ms) { while (ms--) busy_wait_us(1000); }

void MCAN0_Handler(void)
{
    uint32_t ir = hri_mcan_read_IR_reg(MCAN_INST);
    if (ir & MCAN_IR_RF0N) {
        uint32_t f0s = hri_mcan_read_RXF0S_reg(MCAN_INST);
        uint8_t idx = (uint8_t)((f0s & MCAN_RXF0S_F0GI_Msk) >> MCAN_RXF0S_F0GI_Pos);
        uint32_t f0c = hri_mcan_read_RXF0C_reg(MCAN_INST);
        uint32_t f0sa = (f0c & MCAN_RXF0C_F0SA_Msk) >> MCAN_RXF0C_F0SA_Pos; // word address
        uint32_t rxesc = hri_mcan_read_RXESC_reg(MCAN_INST);
        uint8_t f0ds = (uint8_t)((rxesc & MCAN_RXESC_F0DS_Msk) >> MCAN_RXESC_F0DS_Pos);
        uint8_t bytes = _ds_to_bytes(f0ds);
        uint32_t stride_words = 2u + ((bytes + 3u) >> 2);
        volatile uint32_t *elem = (volatile uint32_t *)((f0sa + (uint32_t)idx * stride_words) * 4u);

        uint32_t R0 = elem[0];
        uint32_t R1 = elem[1];
        uint8_t dlc = (uint8_t)((R1 >> 16) & 0x0F); if (dlc > 8) dlc = 8;
        uint32_t eid = (R0 & 0x1FFFFFFFu);

        uint8_t n = qh; rxq[n].eid = eid; rxq[n].dlc = dlc;
        volatile uint8_t *srcb = (volatile uint8_t *)&elem[2];
        for (uint8_t i=0;i<dlc;i++) rxq[n].data[i] = srcb[i];

        qh = (qh + 1) & (RXQ_SZ-1);
        if (qcount < RXQ_SZ) qcount++; else qt = (qt + 1) & (RXQ_SZ-1);

        hri_mcan_write_RXF0A_reg(MCAN_INST, idx);
        hri_mcan_write_IR_reg(MCAN_INST, MCAN_IR_RF0N);
    }
    hri_mcan_write_IR_reg(MCAN_INST, ir & ~(MCAN_IR_RF0N));
}
#endif
