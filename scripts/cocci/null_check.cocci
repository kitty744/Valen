// Find malloc calls where the result isn't checked for NULL
@@
expression p;
statement S;
@@

p = malloc(...);
... when != (p == NULL || p != NULL)