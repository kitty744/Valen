# STDIO Library Documentation

The STDIO library provides VGA text mode output, serial communication, and formatted printing capabilities for CaneOS.

## VGA Text Mode Functions

### Color Management

Sets the global text color for all subsequent kprint operations. The color is a VGA color attribute where the lower 4 bits are the foreground color and the upper 4 bits are the background color.

```c
void set_color(uint8_t color);
```

### Cursor Management

Returns the current cursor position coordinates.

```c
int get_cursor_x(void);
int get_cursor_y(void);
```

Manually sets the cursor position with spinlock protection. Coordinates are 0-indexed.

```c
void set_cursor(int x, int y);
```

Communicates with the VGA hardware to move the blinking cursor to the specified position.

```c
void update_cursor(int x, int y);
```

Configures the hardware cursor shape by setting the scan line start and end positions.

```c
void enable_cursor(uint8_t cursor_start, uint8_t cursor_end);
```

Disables the hardware cursor rendering by setting bit 5 of the Cursor Start Register (0x0A).

```c
void hide_hardware_cursor(void);
```

Enables the hardware cursor rendering by clearing bit 5 of the Cursor Start Register (0x0A).

```c
void show_hardware_cursor(void);
```

### Screen Management

Clears the entire screen except for the status bar (Row 0). Resets cursor to position (0, 1) and enables the hardware cursor.

```c
void print_clear(void);
```

Handles newline operations including text wrapping and screen scrolling. Preserves the status bar while scrolling content up.

```c
void print_newline(void);
```

### Character Output

Prints a single character to the VGA buffer. Handles special characters like '\n', automatic line wrapping, and scrolling. Uses spinlock protection for thread safety.

```c
void putc(char c);
```

Prints a null-terminated string to the VGA buffer by calling putc() for each character.

```c
void puts(const char *str);
```

Implements backspace functionality by moving the cursor back one position (with proper line wrapping) and replacing the character with a space. Prevents backspacing into Row 0 (status bar).

```c
void print_backspace(void);
```

## Formatted Printing

### printf Function

Enhanced printf function with extensive formatting support. Uses variable arguments for formatted output.

```c
void printf(const char *format, ...);
```

## printf Formatting Options

CaneOS includes an enhanced printf with extensive formatting:

```c
// Numbers
printf("%d", -42);        // Signed integer
printf("%u", 42);         // Unsigned integer
printf("%ld", 12345678);  // Long integer
printf("%llu", 1234567890); // Long long unsigned

// Bases
printf("%x", 255);        // Hexadecimal (lowercase)
printf("%X", 255);        // Hexadecimal (uppercase)
printf("%o", 8);          // Octal
printf("%b", 5);          // Binary

// Other
printf("%p", ptr);        // Pointer (0x format)
printf("%s", "hello");    // String
printf("%c", 'A');        // Character
printf("%%");             // Literal percent
```

### Number Printing Functions

Prints a signed 64-bit integer in decimal format.

```c
void print_int(uint64_t n);
```

Prints an unsigned 64-bit integer in decimal format.

```c
void print_uint(uint64_t num);
```

Prints an unsigned 64-bit integer in hexadecimal format with "0x" prefix (uppercase letters).

```c
void print_hex(uint64_t n);
```

Prints an unsigned 64-bit integer in hexadecimal format without prefix (uppercase letters).

```c
void print_hex_upper(uint64_t num);
```

Prints an unsigned 64-bit integer in octal format.

```c
void print_octal(uint64_t num);
```

Prints an unsigned 64-bit integer in binary format.

```c
void print_binary(uint64_t num);
```

## Serial Communication

### Serial Output Functions

Writes a null-terminated string to the COM1 serial port for diagnostics. Uses spinlock protection for thread safety. I/O ports (outb) work in both lower and higher half memory.

```c
void serial_write(char *s);
```

Sends an unsigned 64-bit integer to the serial port in decimal format.

```c
void serial_write_int(uint64_t n);
```

Sends a 32-bit integer to the serial port in hexadecimal format with "0x" prefix.

```c
void serial_write_hex(uint32_t n);
```

## Constants and Configuration

- **VGA Buffer Address**: `0xFFFFFFFF800B8000` (Higher Half Virtual Address)
- **Screen Dimensions**: 80 columns × 25 rows
- **Default Color**: `0x0F` (White text on black background)
- **Serial Port**: COM1 (0x3f8)

## Thread Safety

Most functions use spinlock protection to ensure thread safety in a multi-kernel-thread environment. The lock is initialized and acquired/released for operations that modify shared state like the cursor position or VGA buffer.
