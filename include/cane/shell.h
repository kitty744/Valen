#ifndef SHELL_H
#define SHELL_H

#include <stdint.h>

#define MAX_BUFFER 256
#define PROMPT ">> "
#define PROMPT_LEN 3

void shell_init(void);
void shell_input(signed char c);

#endif
