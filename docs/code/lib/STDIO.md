# STDIO Library

The STDIO library provides VGA text mode output, serial communication, and formatted printing capabilities for CaneOS.

## Overview

The STDIO library is the primary interface for kernel output in CaneOS. It supports both VGA text mode display for user interaction and serial port output for debugging. The library includes a powerful printf implementation with extensive formatting options.

## Quick Start

```c
#include "cane/stdio.h"

void kernel_main(void) {
    // Set text color (optional)
    set_color(0x0F); // White text on black background

    // Basic output
    printf("Hello, CaneOS!\n");

    // Formatted output
    printf("System initialized: %d MB RAM\n", memory_size);

    // Debug output to serial
    serial_write("Debug: Kernel started successfully\n");
}
```

## VGA Text Mode Output

### Basic Printing

The VGA text mode provides an 80x25 character display with color support:

```c
// Simple text output
printf("This is printed to the VGA buffer\n");

// Character output
putc('A');

// String output
puts("Hello World");
```

### Screen Management

```c
// Clear the screen (preserves status bar at row 0)
print_clear();

// Manual cursor control
set_cursor(10, 5);  // Column 10, Row 5
int x = get_cursor_x();
int y = get_cursor_y();
```

### Color Support

Use VGA color attributes where the lower 4 bits are foreground and upper 4 bits are background:

```c
// Common colors
#define COLOR_BLACK     0x00
#define COLOR_BLUE      0x01
#define COLOR_GREEN     0x02
#define COLOR_CYAN      0x03
#define COLOR_RED       0x04
#define COLOR_MAGENTA   0x05
#define COLOR_BROWN     0x06
#define COLOR_LIGHT_GREY 0x07
#define COLOR_GREY      0x08
#define COLOR_LIGHT_BLUE 0x09
#define COLOR_LIGHT_GREEN 0x0A
#define COLOR_LIGHT_CYAN  0x0B
#define COLOR_LIGHT_RED   0x0C
#define COLOR_LIGHT_MAGENTA 0x0D
#define COLOR_YELLOW    0x0E
#define COLOR_WHITE      0x0F

set_color(COLOR_LIGHT_GREEN | COLOR_BLUE);  // Light green on blue
printf("Colored text\n");
set_color(COLOR_WHITE | COLOR_BLACK);        // Reset to default
```

### Cursor Control

```c
// Enable/disable hardware cursor
enable_cursor(14, 15);  // Set cursor shape
hide_hardware_cursor();
show_hardware_cursor();

// Manual cursor positioning
set_cursor(0, 1);  // Start of user area (below status bar)
```

## Formatted Printing

The printf function supports extensive formatting options:

### Number Formatting

```c
int value = -42;
unsigned int unsigned_val = 255;
long long big_num = 1234567890LL;

printf("Signed: %d\n", value);           // -42
printf("Unsigned: %u\n", unsigned_val);   // 255
printf("Long: %ld\n", big_num);          // 1234567890
printf("Long long: %llu\n", big_num);    // 1234567890
```

### Base Conversion

```c
unsigned int val = 255;

printf("Hex: %x\n", val);      // ff
printf("HEX: %X\n", val);      // FF
printf("Octal: %o\n", val);    // 377
printf("Binary: %b\n", val);    // 11111111
```

### Pointers and Addresses

```c
void *ptr = &some_variable;
printf("Pointer: %p\n", ptr);   // 0xFFFFFFFF80001000
```

### Strings and Characters

```c
printf("String: %s\n", "Hello");
printf("Character: %c\n", 'A');
printf("Literal %%: %%\n");
```

## Serial Communication

Serial output is primarily used for debugging and diagnostics:

```c
// Basic serial output
serial_write("Kernel boot started\n");

// Formatted serial output
serial_write_int(memory_size);
serial_write_hex(0xDEADBEEF);
```

## Error Handling

The STDIO library is designed to be robust:

- All VGA operations use spinlocks for thread safety
- Serial operations are protected against concurrent access
- Invalid format specifiers are handled gracefully
- Cursor operations are bounded to screen dimensions

## Best Practices

1. **Use serial output for debugging** - Serial works even when VGA is unavailable
2. **Preserve the status bar** - Row 0 is reserved for system status
3. **Use appropriate colors** - Maintain visual consistency
4. **Handle thread safety** - The library handles locking automatically
5. **Format strings carefully** - Use proper format specifiers

## Integration Example

```c
#include "cane/stdio.h"

void init_display(void) {
    // Initialize display system
    set_color(COLOR_WHITE | COLOR_BLACK);
    print_clear();

    // Set up status bar
    set_cursor(0, 0);
    set_color(COLOR_BLACK | COLOR_LIGHT_GREY);
    printf("CaneOS v1.0 - Ready");

    // Reset to user area
    set_color(COLOR_WHITE | COLOR_BLACK);
    set_cursor(0, 1);
}

void kernel_main(void) {
    init_display();

    printf("System initialization complete\n");
    printf("Memory: %d MB available\n", get_memory_size());

    serial_write("Kernel initialization successful\n");
}
```

## Thread Safety

All STDIO functions that modify shared state (VGA buffer, cursor position, serial port) use spinlocks to ensure thread safety in multi-kernel-thread environments. The locking is handled automatically - no additional synchronization is required from the caller.

## Constants and Configuration

- **VGA Buffer Address**: `0xFFFFFFFF800B8000` (Higher Half Virtual Address)
- **Screen Dimensions**: 80 columns × 25 rows
- **Default Color**: `0x0F` (White text on black background)
- **Serial Port**: COM1 (0x3f8)
