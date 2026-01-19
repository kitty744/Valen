// Detect usage of a pointer after it has been freed
@@
expression ptr;
identifier f;
@@

free(ptr);
... when != ptr = malloc(...)
(
* f(..., ptr, ...)
|
* ptr->f
|
* *ptr
)