// Tell Coccinelle to ignore these common kernel decorators
#define __init
#define __exit
#define __user
#define __kernel
#define __percpu
#define __packed __attribute__((packed))
#define __aligned(x) __attribute__((aligned(x)))

// Handle inline assembly so it doesn't break the parser
#define __asm__(x)
#define __volatile__