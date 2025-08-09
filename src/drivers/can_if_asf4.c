/*
 * Project: ATSAMC21J15 – Rectifier CAN Interface (Classic CAN, 29-bit EID)
 * Author:  Sripad Madhusudan Upadhyaya
 * License: MIT
 * Year:    2025
 */
#include "drivers/can_if.h"
#include "config/build_config.h"
#if CAN_BACKEND_ASF4
#include "hal_can_async.h"
#include "hpl_mcan_base.h"
#include "atmel_start.h"

#ifdef CANIF_USE_FREERTOS
#include "FreeRTOS.h"
#include "semphr.h"
static StaticSemaphore_t rx_sem_buf; static SemaphoreHandle_t rx_sem;
#define RX_SIGNAL()   xSemaphoreGiveFromISR(rx_sem, NULL)
#define RX_WAIT(ms)   (xSemaphoreTake(rx_sem, pdMS_TO_TICKS(ms)) == pdTRUE)
#else
static volatile uint8_t rx_flag;
#define RX_SIGNAL()   do{ rx_flag = 1; }while(0)
static bool RX_WAIT(uint32_t ms){ while(ms--){ if(rx_flag){rx_flag=0; return true;} for(volatile uint32_t i=0;i<4000;i++) __asm__ __volatile__("nop"); } return false; }
#endif

extern struct can_async_descriptor CAN_0; // from START

typedef struct { uint32_t eid; uint8_t dlc; uint8_t data[8]; } rx_frame_t;
#define RXQ_SZ 8
static volatile rx_frame_t rxq[RXQ_SZ];
static volatile uint8_t qh, qt, qcount;

static void rx_cb(const struct can_async_descriptor *const descr)
{
    (void)descr;
    struct can_message m;
    if (can_async_read(&CAN_0, &m) != 0) return;

    uint8_t n = qh;
    rxq[n].eid = m.id;
    rxq[n].dlc = m.len;
    for (uint8_t i=0;i<m.len && i<8;i++) rxq[n].data[i] = m.data[i];

    qh = (qh + 1) & (RXQ_SZ-1);
    if (qcount < RXQ_SZ) qcount++; else qt = (qt + 1) & (RXQ_SZ-1);
    RX_SIGNAL();
}

void can_init_100k(void)
{
#ifdef CANIF_USE_FREERTOS
    rx_sem = xSemaphoreCreateBinaryStatic(&rx_sem_buf);
#endif
    // Force 100k @ 4 MHz if not set in START
    hri_mcan_set_CCCR_INIT_bit(MCAN0); while (!hri_mcan_get_CCCR_INIT_bit(MCAN0)) {}
    hri_mcan_set_CCCR_CCE_bit(MCAN0);
    hri_mcan_write_NBTP_reg(MCAN0, NBTP_4MHZ_100K);
    hri_mcan_clear_CCCR_FDOE_bit(MCAN0); hri_mcan_clear_CCCR_BRSE_bit(MCAN0);
    hri_mcan_clear_CCCR_INIT_bit(MCAN0); while (hri_mcan_get_CCCR_INIT_bit(MCAN0)) {}

    can_async_register_callback(&CAN_0, CAN_ASYNC_RX_CB, (FUNC_PTR)rx_cb);
    can_async_enable(&CAN_0);
}

bool can_set_ext_filter_mask(uint8_t idx, const can_filter_ext_mask_t *f)
{
    (void)idx;
    struct can_filter flt; flt.id=f->match; flt.mask=f->mask; flt.extended=true;
    return can_async_set_filter(&CAN_0,&flt)==0;
}

bool can_tx_ext(uint32_t eid, const uint8_t *data, uint8_t dlc)
{
    if (dlc > 8) dlc = 8;
    struct can_message m;
    m.id=eid; m.type=CAN_TYPE_DATA; m.format=CAN_FMT_EXTID; m.data=(uint8_t*)data; m.len=dlc;
    return can_async_write(&CAN_0,&m)==0;
}

bool can_rx_wait(uint32_t *eid, uint8_t *data, uint8_t *dlc, uint32_t timeout_ms)
{
    if (qcount == 0) { if (!RX_WAIT(timeout_ms)) return false; }
    if (qcount == 0) return false;

    uint8_t n = qt;
    *eid = rxq[n].eid; *dlc = rxq[n].dlc;
    for (uint8_t i=0;i<*dlc;i++) data[i] = rxq[n].data[i];

    qt = (qt + 1) & (RXQ_SZ-1);
    __atomic_fetch_sub((volatile int*)&qcount, 1, __ATOMIC_RELAXED);
    return true;
}

void can_sleep_ms(uint32_t ms) { while (ms--) for (volatile uint32_t i=0;i<4000;i++) __asm__ __volatile__("nop"); }
#endif
