// Flag potential zero-byte allocations if not guarded
@@
expression size;
@@

*malloc(size);
// This is a "manual review" rule to ensure 'size' is validated elsewhere