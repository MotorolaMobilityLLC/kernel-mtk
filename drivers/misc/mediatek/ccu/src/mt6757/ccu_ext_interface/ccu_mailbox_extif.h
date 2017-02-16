/*******************************************************************************************
* Module: CCU mailbox external interface
*
* Description: External interface for both CCU/APMCU side reference,
*              Develop an unified data structure and circular queue full/empty definition
********************************************************************************************/
#ifndef __CCU_MAILBOX_EXTIF__
#define __CCU_MAILBOX_EXTIF__

#include "ccu_ext_interface/ccu_ext_interface.h"

#define CCU_MAILBOX_QUEUE_SIZE 16	/*must be power of 2 for modulo operation take work*/

/******************************************************************************
* Mailbox is a circular queue
* Composed of 2 pointer front/rear, and a buffer of ccu_msg_t

!!! Implement mailbox operation with following rules !!!
    - Initial queue with front=rear=0
    - Dequeue with read queue[front+1] and increase front pointer by 1 (modulus add)
    - Enqueue with write into queue[rear+1] and increase rear pointer by 1 (modulus add)
    - Queue is full when front=rear+1 (modulus add: rear+1 = rear+1 % CCU_MAILBOX_QUEUE_SIZE)
    - Queue is empty when front=rear
******************************************************************************/
typedef struct _ccu_mailbox_t {
		volatile MUINT32 front;
		volatile MUINT32 rear;
		volatile ccu_msg_t queue[CCU_MAILBOX_QUEUE_SIZE];
} ccu_mailbox_t;

#endif
