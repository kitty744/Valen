// Ensure magic field is only modified in heap.c
@@
identifier f != {heap_init, malloc, free};
struct heap_node *N;
@@

f(...) {
 ...
* N->magic = ...
 ...
}