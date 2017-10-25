	.set	r0,0
	.set	r3,3
	.set	r4,4

.__divus:
	.globl	.__divus
        divwu	r0,r3,r4        # r0 = r3 / r4
        mullw	r4,r0,r4        # r4 = (r3/r4) * r4
        subf	r4,r4,r3        # remainder: r4 = r3 - ((r3/r4)* r4)
        ori	r3,r0,0x0       # quotient to correct reg
        bclr	0x14,0          # branch to link always

.__mulh:
	.globl	.__mulh
        mulhw   r3,r3,r4        # r3 = (high 32 bits) r3 * r4
        bclr    0x14,0          # branch to link always

.__mull:
	.globl	.__mull
        mullw   r0,r3,r4        # r0 = (low 32 bits) r3 * r4
        mulhw   r3,r3,r4        # r3 = (high 32 bits) r3 * r4
        ori     r4,r0,0x0       # move low part to correct reg
        bclr    0x14,0          # branch to link always

.__divss:
	.globl	.__divss
        divw    r0,r3,r4        # r0 = r3 / r4
        mullw   r4,r0,r4        # r4 = (r3/r4) * r4
        subf    r4,r4,r3        # remainder: r4 = r3 - ((r3/r4)*r4)
        ori     r3,r0,0x0       # quotient to correct reg
        bclr    0x14,0          # branch to link always

.__quoss:
	.globl	.__quoss
        divw    r3,r3,r4        # r0 = r3 / r4
        bclr    0x14,0          # branch to link always

.__quous:
	.globl	.__quous
        divwu   r3,r3,r4        # r0 = r3 / r4
        bclr    0x14,0          # branch to link always

