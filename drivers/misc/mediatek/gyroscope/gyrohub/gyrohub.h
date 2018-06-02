#ifndef GYROHUB_H
#define GYROHUB_H

#include <linux/ioctl.h>

#define GYROHUB_AXIS_X          0
#define GYROHUB_AXIS_Y          1
#define GYROHUB_AXIS_Z          2
#define GYROHUB_AXES_NUM        3
#define GYROHUB_DATA_LEN        6
#define GYROHUB_SUCCESS             0

#define GYROHUB_BUFSIZE 60

/* 1 rad = 180/PI degree, MAX_LSB = 131, */
/* 180*131/PI = 7506 */
#define DEGREE_TO_RAD	7506

#endif

