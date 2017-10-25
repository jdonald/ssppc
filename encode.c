int main(void) {
	int i;
	int loc;
	int data[] = {31, 0, 3, 4, 0, 491 , 0};
//	int sizes[] = {6, 5, 5, 32-16}; 
//	int data[] = {31, 4, 0, 4, 0, 235, 0};
//	int data[] = {31, 0, 3, 4, 0, 495, 0};
//	int data[] = {31, 3, 3, 4, 0, 491, 0};
//	int data[] = {31, 0, 3, 4, 0, 459, 0};
	int sizes[] = {6, 5, 5, 5, 1, 9, 1};
	int L = 7;
	int inst = 0;

	loc = 32;

#ifdef TESTING
	printf("testing.");
#endif
	for (i = 0; i < L; i++) {
//		printf("%d %x %d\n", loc, inst, data[i]);
		inst = inst | (data[i] << (loc - sizes[i]) );
		loc = loc - sizes[i];
	}
	printf("%x\n", inst);
	
}
