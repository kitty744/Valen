// Ensure vmm_alloc result is always checked before use
@@
expression p;
constant flags;
@@

p = vmm_alloc(..., flags);
... when != (p == NULL || p != NULL)
* p[...] // Usage before NULL check