#ifndef FBC_TOUCH_H
#define FBC_TOUCH_H

extern struct mutex notify_lock;
extern int fbc_game;
extern void notify_touch(int action);
#endif

