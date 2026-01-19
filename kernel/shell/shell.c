/**
 * @file shell.c
 * @brief CaneOS Interactive Shell
 * 
 * Provides a command-line interface for system interaction. This module
 * manages a local input buffer, handles character insertion/deletion,
 * and interfaces with VGA driver to provide a flicker-free experience
 * through hardware cursor masking and coordinate math.
 */

#include <cane/shell.h>
#include <cane/stdio.h>
#include <cane/string.h>
#include <cane/io.h>
#include <cane/pmm.h>
#include <cane/spinlock.h>

static char input_buffer[MAX_BUFFER];
static int buffer_len = 0;
static int cursor_idx = 0;
static int prompt_start_y = 1;
static spinlock_t shell_lock = SPINLOCK_INIT;

/**
 * @brief Disables hardware cursor rendering.
 * 
 * Communicates with CRT Controller (CRTC) registers. Setting bit 5
 * of Cursor Start Register (0x0A) instructs VGA hardware to
 * stop rendering blinking cursor.
 */
static void hide_hardware_cursor(void)
{
    outb(0x3D4, 0x0A);
    outb(0x3D5, inb(0x3D5) | 0x20);
}

/**
 * @brief Enables hardware cursor rendering.
 * 
 * Clears bit 5 of Cursor Start Register (0x0A) to allow
 * VGA hardware to render blinking cursor at current register position.
 */
static void show_hardware_cursor(void)
{
    outb(0x3D4, 0x0A);
    outb(0x3D5, inb(0x3D5) & ~0x20);
}

/**
 * @brief Gets current cursor X position.
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
 * @brief Gets current cursor Y position.
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
 * @brief Sets cursor to specific position.
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
 * @brief Resets shell state and initializes prompt.
 * 
 * Clears internal input buffer, resets logical cursor index,
 * and establishes vertical anchor (prompt_start_y) for current
 * command line.
 */
void shell_init(void)
{
    spinlock_acquire(&shell_lock);
    
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

    spinlock_release(&shell_lock);
}

/**
 * @brief Redraws the command line while preventing cursor ghosting.
 * 
 * This function masks hardware cursor during printing process.
 * It calculates absolute VGA coordinates based on prompt length
 * and current buffer index to handle line-wrapping across 80 columns.
 */
static void redraw_line(void)
{
    /* Calculate final landing spot for hardware cursor after redraw */
    int final_total = PROMPT_LEN + cursor_idx;
    int final_x = final_total % 80;
    int final_y = prompt_start_y + (final_total / 80);

    /* Temporarily hide cursor to prevent "ghosting" during putchar calls */
    hide_hardware_cursor();

    /* Reset software cursor to start of current input line */
    set_cursor_pos(PROMPT_LEN, prompt_start_y);

    /* Overwrite current screen line(s) with the updated buffer */
    for (int i = 0; i < buffer_len; i++)
    {
        putchar(input_buffer[i]);
    }

    /* Print a trailing space to erase characters left over by backspaces */
    putchar(' ');

    /* Synchronize VGA cursor registers with the final calculated position */
    set_cursor_pos(final_x, final_y);

    /* Restore hardware cursor visibility at the final destination */
    show_hardware_cursor();
}

/**
 * @brief Logic for interpreting and executing shell commands.
 * 
 * Compares input buffer against known commands and dispatches
 * to appropriate kernel subsystems with formatted output.
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
    spinlock_acquire(&shell_lock);
    
    if (c == KEY_ENTER)
    {
        input_buffer[buffer_len] = '\0';
        printf("\n");
        process_command(input_buffer);
        shell_init();
    }
    else if (c == KEY_BACKSPACE && cursor_idx > 0)
    {
        for (int i = cursor_idx - 1; i < buffer_len - 1; i++)
        {
            input_buffer[i] = input_buffer[i + 1];
        }
        buffer_len--;
        cursor_idx--;
        redraw_line();
    }
    else if (c == KEY_ARROW_LEFT && cursor_idx > 0)
    {
        cursor_idx--;
        redraw_line();
    }
    else if (c == KEY_ARROW_RIGHT && cursor_idx < buffer_len)
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
    
    spinlock_release(&shell_lock);
}