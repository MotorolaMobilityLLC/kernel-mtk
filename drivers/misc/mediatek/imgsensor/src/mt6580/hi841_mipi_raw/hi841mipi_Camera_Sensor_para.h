/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

/*******************************************************************************************/


/*******************************************************************************************/

/* SENSOR FULL SIZE */
#ifndef _HI841MIPI_CAMERA_SENSOR_PARA_H
#define _HI841MIPI_CAMERA_SENSOR_PARA_H

#define CAMERA_SENSOR_REG_DEFAULT_VALUE  \
    /* ARRAY: SENSOR.reg[11] */\
    {\
        /* STRUCT: SENSOR.reg[0] */\
        {\
            /* SENSOR.reg[0].addr */ 0x00000304, /* SENSOR.reg[0].para */ 0x00000000\
        },\
        /* STRUCT: SENSOR.reg[1] */\
        {\
            /* SENSOR.reg[1].addr */ 0x00000305, /* SENSOR.reg[1].para */ 0x0000000D\
        },\
        /* STRUCT: SENSOR.reg[2] */\
        {\
            /* SENSOR.reg[2].addr */ 0x00000306, /* SENSOR.reg[2].para */ 0x00000000\
        },\
        /* STRUCT: SENSOR.reg[3] */\
        {\
            /* SENSOR.reg[3].addr */ 0x00000307, /* SENSOR.reg[3].para */ 0x000000C0\
        },\
        /* STRUCT: SENSOR.reg[4] */\
        {\
            /* SENSOR.reg[4].addr */ 0x00000300, /* SENSOR.reg[4].para */ 0x00000000\
        },\
        /* STRUCT: SENSOR.reg[5] */\
        {\
            /* SENSOR.reg[5].addr */ 0x00000301, /* SENSOR.reg[5].para */ 0x00000004\
        },\
        /* STRUCT: SENSOR.reg[6] */\
        {\
            /* SENSOR.reg[6].addr */ 0x0000030A, /* SENSOR.reg[6].para */ 0x00000000\
        },\
        /* STRUCT: SENSOR.reg[7] */\
        {\
            /* SENSOR.reg[7].addr */ 0x0000030B, /* SENSOR.reg[7].para */ 0x00000002\
        },\
        /* STRUCT: SENSOR.reg[8] */\
        {\
            /* SENSOR.reg[8].addr */ 0x00000308, /* SENSOR.reg[8].para */ 0x00000000\
        },\
        /* STRUCT: SENSOR.reg[9] */\
        {\
            /* SENSOR.reg[9].addr */ 0x00000309, /* SENSOR.reg[9].para */ 0x00000008\
        },\
        /* STRUCT: SENSOR.reg[10] */\
        {\
            /* SENSOR.reg[10].addr */ 0xFFFFFFFF, /* SENSOR.reg[10].para */ 0x00000001\
        }\
}

#define CAMERA_SENSOR_CCT_DEFAULT_VALUE {{ 0xFFFFFFFF, 0x08 } ,{ 0x0205, 0x20 } ,{ 0x020e, 0x01 } ,{ 0x0210, 0x01 } ,{ 0x0212, 0x01 }}
#endif /* __CAMERA_SENSOR_PARA_H */
