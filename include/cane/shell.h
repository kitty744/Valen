#ifndef SHELL_H
#define SHELL_H

#include <stdint.h>

#define MAX_BUFFER 256
#define PROMPT ">> "
#define PROMPT_LEN 3

/* Special key codes */
#define KEY_BACKSPACE   '\b'
#define KEY_ENTER      '\n'
#define KEY_ARROW_LEFT  -1
#define KEY_ARROW_RIGHT -2

void shell_init(void);
void shell_input(signed char c);

#endif
