/* void tlb_access(void *base, uint64_t num_pages, uint64_t descending) */

/* Access num_pages 4k pages, starting at base (or base + (num_pages - 1) * 4k) */
/* in ascending (descending == 0) or descending order without touching the stack */


/* a0 = base address                                    */
/* a1 = number of pages to touch                        */
/* a2 = access order (0 == ascending, 1 == descending)  */
.global tlb_access
.type tlb_access, @function
.align 4
tlb_access:
beq a1, x0, 2f /* Don't touch any pages? Sure! */

/* Default increment: one 4k page in positive direction */
addi a3, x0, 1
slli a3, a3, 12

bne a2, x0, 3f  /* Ascending or do we have to update the base address? */

1:
ld t2, 0(a0)  /* Access the page */

addi a1, a1, -1
add a0, a0, a3
blt x0, a1, 1b

2:
ret

3:  /* This is the more unlikely case of accessing in descending direction */
addi t0, a1, -1
slli t1, t0, 12
add a0, a0, t1

/* Increment: one 4k page in NEGATIVE direction */
addi a3, x0, -1
slli a3, a3, 12

jal x0, 1b
