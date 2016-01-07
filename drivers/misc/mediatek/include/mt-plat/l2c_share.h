#ifndef _L2_SHARE_H
#define _L2_SHARE_H

enum mt_l2c_options {
	BORROW_L2,
	RETURN_L2,
	BORROW_NONE
};

int switch_L2(enum mt_l2c_options option);

#endif
