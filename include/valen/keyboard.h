#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

void keyboard_init(void);
void keyboard_handler(void);
void process_pending_key(void);
void wait_for_keypress(void);

#endif
