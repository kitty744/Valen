// Ensure every spinlock_acquire has a matching release before returning
@@
expression lk;
@@

 spinlock_acquire(lk);
 ... when != spinlock_release(lk)
*return ...;