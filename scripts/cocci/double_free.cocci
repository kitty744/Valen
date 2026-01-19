// Detect if free is called twice on the same pointer without re-allocation
@@
expression ptr;
@@

free(ptr);
... when != ptr = malloc(...)
    when exists
*free(ptr);