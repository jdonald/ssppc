void writemillicode(struct mem_t *mem) {
	word_t data[] = { /* quous */ 0x3380, 0x7c632396, 0x4e800020, 0,
				/* quoss */ 0x3300, 0x7c6323d6, 0x4e800020, 0,
				/* divus */ 0x3280, 0x7c0323de, 0x7c8021d6, 0x7c841850, 0x60600000, 				0x4e800020, 0,
				/* mulh */ 0x3100, 0x7c632096, 0x4e800020, 0,
				/* mull */ 0x3180, 0x7c0321d6,0x7c632096, 0x60800000, 0x4e800020, 0,
	
				/* divss */ 0x3200, 0x7c0323d6, 0x7c8021d6, 0x7c841850, 0x60600000,
            0x4e800020, 0 };		

	int i;
	int x;
	int DATAMAX;

	i = 0;
	DATAMAX = sizeof(data)/sizeof(word_t);
	while (i < DATAMAX) {
		for (x = i+1; data[x] != 0; x++) {
#ifdef PPC_DEBUG
			printf("writing millicode %x %x %x\n",data[i]+sizeof(word_t)*(x-i-1), x-(i+1), data[x]);
#endif
			mem_access(mem, Write, data[i]+ sizeof(word_t)*(x-i-1), &data[x], sizeof(word_t) );
		}
		i = x+1;
	}
	/*
	data = 0x7c632396;
	mem_access(mem, Write, 0x0003380, &data, sizeof(word_t) );
	data = 0x4e800020;
	mem_access(mem, Write, 0x0003380+4, &data, sizeof(word_t) );
	*/
}
