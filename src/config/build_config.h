#pragma once
/*
 * Author:  Sripad Madhusudan Upadhyaya
 * License: MIT
 * Year:    2025
 *
 * Backend selection (pick exactly one):
 */
#define CAN_BACKEND_ASF4   1
#define CAN_BACKEND_MCAN   0

/*
 * Bit timing for 100 kbit/s @ 4 MHz CAN clock (20 TQ, SP≈80%, SJW=3)
 * M_CAN encoding is (value - 1):
 *   NBRP=1  -> NBRP-1 = 0
 *   TSEG1=15-> NTSEG1  = 14
 *   TSEG2=4 -> NTSEG2  = 3
 *   SJW=3   -> NSJW    = 2
 */
#define NBTP_4MHZ_100K   ( MCAN_NBTP_NBRP(0) | MCAN_NBTP_NTSEG1(14) | MCAN_NBTP_NTSEG2(3) | MCAN_NBTP_NSJW(2) )
