/*
 * Project: ATSAMC21J15 – Rectifier CAN Interface (Classic CAN, 29-bit EID)
 * Author:  Sripad Madhusudan Upadhyaya
 * License: MIT
 * Year:    2025
 */
#include "atmel_start.h"
#include "drivers/can_if.h"
#include "app/rectifier.h"
#include "sys/log.h"

int main(void)
{
    atmel_start_init();       // START init (clocks, pins, MCAN instance)

    can_init_100k();

    // Filter: match only ID=2 and ToFrom=0
    can_filter_ext_mask_t f = {0};
    f.match = rect_pack_eid(2, 0, 0, 0, 0, 0);
    f.mask  = ((uint32_t)0x1F << 24) | ((uint32_t)0x01 << 8);
    can_set_ext_filter_mask(0, &f);

    rect_init(220.5f, 20.0f);

    while (1) {
        rect_send_setpoints_and_wait_ack(50);
        can_sleep_ms(200);
    }
}
