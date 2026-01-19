/**
 * @file shell.c
 * @brief CaneOS Interactive Shell
 */

#include <cane/shell.h>
#include <cane/stdio.h>
#include <cane/string.h>
#include <cane/io.h>
#include <cane/pmm.h>

#define MAX_BUFFER 256
#define PROMPT ">> "
#define PROMPT_LEN 3

static char input_buffer[MAX_BUFFER];
static int buffer_len = 0;
static int cursor_idx = 0;
static int prompt_start_y = 1;

/**
 * @brief Get current cursor X position.
 */
static int get_cursor_x(void)
{
    uint16_t pos;
    outb(0x3D4, 0x0F);
    pos = inb(0x3D5);
    outb(0x3D4, 0x0E);
    pos |= ((uint16_t)inb(0x3D5)) << 8;
    return pos % 80;
}

/**
 * @brief Get current cursor Y position.
 */
static int get_cursor_y(void)
{
    uint16_t pos;
    outb(0x3D4, 0x0F);
    pos = inb(0x3D5);
    outb(0x3D4, 0x0E);
    pos |= ((uint16_t)inb(0x3D5)) << 8;
    return pos / 80;
}

/**
 * @brief Set cursor to specific position.
 */
static void set_cursor_pos(int x, int y)
{
    uint16_t pos = y * 80 + x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

/**
 * @brief Disables hardware cursor rendering.
 */
static void hide_hardware_cursor(void)
{
    outb(0x3D4, 0x0A);
    outb(0x3D5, inb(0x3D5) | 0x20);
}

/**
 * @brief Enables hardware cursor rendering.
 */
static void show_hardware_cursor(void)
{
    outb(0x3D4, 0x0A);
    outb(0x3D5, inb(0x3D5) & ~0x20);
}

/**
 * @brief Resets shell state and initializes prompt.
 */
void shell_init(void)
{
    memset(input_buffer, 0, MAX_BUFFER);
    buffer_len = 0;
    cursor_idx = 0;

    /* Ensure shell starts below row 0 status bar */
    if (get_cursor_y() < 1)
        set_cursor_pos(0, 1);

    prompt_start_y = get_cursor_y();
    printf(PROMPT);

    /* Show hardware cursor at the end of prompt */
    int final_total = PROMPT_LEN + cursor_idx;
    int final_x = final_total % 80;
    int final_y = prompt_start_y + (final_total / 80);
    set_cursor_pos(final_x, final_y);
    show_hardware_cursor();
}

/**
 * @brief Redraws the command line while preventing cursor ghosting.
 */
static void redraw_line(void)
{
    /* Move cursor to start of input area */
    set_cursor_pos(PROMPT_LEN, prompt_start_y);
    
    /* Clear the current line */
    for (int i = 0; i < MAX_BUFFER; i++) {
        putchar(' ');
    }
    
    /* Move back to start */
    set_cursor_pos(PROMPT_LEN, prompt_start_y);
    
    /* Print the buffer */
    for (int i = 0; i < buffer_len; i++) {
        putchar(input_buffer[i]);
    }
    
    /* Position cursor at current position */
    int final_total = PROMPT_LEN + cursor_idx;
    int final_x = final_total % 80;
    int final_y = prompt_start_y + (final_total / 80);
    set_cursor_pos(final_x, final_y);
}

/**
 * @brief Logic for interpreting and executing shell commands.
 */
static void process_command(char *cmd)
{
    if (strcmp(cmd, "clear") == 0)
    {
        clear_screen();
    }
    else if (strcmp(cmd, "help") == 0)
    {
        printf("\n--- CaneOS Command Interface ---\n");
        printf("  help    - Display this menu\n");
        printf("  clear   - Clear the terminal screen\n");
        printf("  mem     - Show physical memory utilization\n");
        printf("  reboot  - Restart the system via PS/2\n");
        printf("----------------------------------\n");
    }
    else if (strcmp(cmd, "mem") == 0)
    {
        uint64_t total = pmm_get_total_kb();
        uint64_t used = pmm_get_used_kb();
        uint64_t free = total - used;

        printf("\n--- Physical Memory Mapping ---\n");
        printf("  Total: ");
        printf("%llu", total / 1024);
        printf(" MB\n");
        printf("  Used:  ");
        printf("%llu", used / 1024);
        printf(" MB\n");
        printf("  Free:  ");
        printf("%llu", free / 1024);
        printf(" MB\n");
        printf("-------------------------------\n");
    }
    else if (strcmp(cmd, "reboot") == 0)
    {
        printf("Sending reset signal to PS/2 controller...\n");
        outb(0x64, 0xFE);
    }
    else if (strlen(cmd) > 0)
    {
        printf("Error: '");
        printf("%s", cmd);
        printf("' is not recognized as a command.\n");
    }
}

/**
 * @brief Handles raw keyboard input characters.
 */
void shell_input(signed char c)
{
    /* Debug: show what we received */
    if (c >= 32 && c <= 126) {
        printf("[%c]", c);
    } else {
        printf("{%d}", c);
    }
    
    if (c == '\n')
    {
        input_buffer[buffer_len] = '\0';
        printf("\n");
        process_command(input_buffer);
        shell_init();
    }
    else if (c == '\b' && cursor_idx > 0)
    {
        for (int i = cursor_idx - 1; i < buffer_len - 1; i++)
        {
            input_buffer[i] = input_buffer[i + 1];
        }
        buffer_len--;
        cursor_idx--;
        redraw_line();
    }
    else if (c == -1 && cursor_idx > 0)
    {
        cursor_idx--;
        redraw_line();
    }
    else if (c == -2 && cursor_idx < buffer_len)
    {
        cursor_idx++;
        redraw_line();
    }
    else if (c >= 32 && c <= 126 && buffer_len < MAX_BUFFER - 1)
    {
        for (int i = buffer_len; i > cursor_idx; i--)
        {
            input_buffer[i] = input_buffer[i - 1];
        }
        input_buffer[cursor_idx] = (char)c;
        buffer_len++;
        cursor_idx++;
        redraw_line();
    }
}
