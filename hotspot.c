
/* just a stub function. not called anywhere in this file	*/
void sim_main()
{
	static double cur_time, prev_time;
	int i;

	/* the main simulator loop */
	while (1) {
		
		/* set the per cycle power values as returned by Wattch/power simulator	*/
		power[get_blk_index(flp, "Icache")] +=  0;	/* set the power numbers instead of '0'	*/
		power[get_blk_index(flp, "Dcache")] +=  0;	
		power[get_blk_index(flp, "Bpred")] +=  0;	
		/* ... more functional units ...	*/


		/* call compute_temp at regular intervals */
		if ((cur_time - prev_time) >= sampling_intvl) {
			double elapsed_time = (cur_time - prev_time);
			prev_time = cur_time;

			/* find the average power dissipated in the elapsed time */
			for (i = 0; i < flp->n_units; i++) {
				overall_power[i] += power[i];
				/* 
				 * 'power' array is an aggregate of per cycle numbers over 
				 * the sampling_intvl. so, compute the average power 
				 */
				power[i] /= (elapsed_time * base_proc_freq);
			}

			/* calculate the current temp given the previous temp,
			 * time elapsed since then, and the average power dissipated during 
			 * that interval */
			compute_temp(power, temp, flp->n_units, elapsed_time);

			/* reset the power array */
			for (i = 0; i < flp->n_units; i++)
				power[i] = 0;
		}
	}
}



/* 
 * read a single line of trace file containing names
 * of functional blocks
 */
int read_names(FILE *fp, char names[][STR_SIZE])
{
	char line[STR_SIZE], *src;
	int i;

	/* read the entire line	*/
	fgets(line, STR_SIZE, fp);
	if (feof(fp))
		fatal("not enough names in trace file\n");

	/* new line not read yet	*/	
	if(line[strlen(line)-1] != '\n')
		fatal("line too long\n");

	/* chop the names from the line read	*/
	for(i=0,src=line; *src && i < MAX_UNITS; i++) 
	{
		if(!sscanf(src, "%s", names[i]))
			fatal("invalid format of names");
		src += strlen(names[i]);
		while (isspace(*src))
			src++;
	}
	if(*src && i == MAX_UNITS)
		fatal("no. of units exceeded limit");

	return i;
}

/* read a single line of power trace numbers	*/
int read_vals(FILE *fp, double vals[])
{
	char line[STR_SIZE], temp[STR_SIZE], *src;
	int i;

	/* read the entire line	*/
	fgets(line, STR_SIZE, fp);
	if (feof(fp))
		return 0;

	/* new line not read yet	*/	
	if(line[strlen(line)-1] != '\n')
		fatal("line too long\n");

	/* chop the power values from the line read	*/
	for(i=0,src=line; *src && i < MAX_UNITS; i++) 
	{
		if(!sscanf(src, "%s", temp) || !sscanf(src, "%lf", &vals[i]))
			fatal("invalid format of values");
		src += strlen(temp);
		while (isspace(*src))
			src++;
	}
	if(*src && i == MAX_UNITS)
		fatal("no. of entries exceeded limit");

	return i;
}

/* write a single line of functional unit names	*/
void write_names(FILE *fp, char names[][STR_SIZE], int size)
{
	int i;
	for(i=0; i < size-1; i++)
		fprintf(fp, "%s\t", names[i]);
	fprintf(fp, "%s\n", names[i]);
}

/* write a single line of temperature trace(in degree C)	*/
void write_vals(FILE *fp, double vals[], int size)
{
	int i;
	for(i=0; i < size-1; i++)
		fprintf(fp, "%.2f\t", vals[i]-273.15);
	fprintf(fp, "%.2f\n", vals[i]-273.15);
}

/* 
 * demo function - reads instantaneous power values (in W) from a trace
 * file ("gcc.p") and outputs instantaneous temperature values (in C) to
 * a trace file("gcc.t"). also outputs steady state temperature values
 * (including those of the internal nodes of the model) onto stdout. the
 * trace files are 2-d matrices with each column representing a functional
 * functional block and each row representing a time unit(sampling_intvl).
 * columns are tab-separated and each row is a separate line. the first
 * line contains the names of the functional blocks. the order in which
 * the columns are specified doesn't have to match that of the floorplan 
 * file.
 */
int main()
{
	int i, n, num, lines = 0;
	FILE *pin, *tout;	/* trace file pointers	*/
	char names[MAX_UNITS][STR_SIZE];
	double vals[MAX_UNITS];

	/* initialize	*/
	sim_init();
	n=flp->n_units;

	if(!(pin = fopen(p_infile, "r")))
		fatal("unable to open power trace input file\n");
	if(!(tout = fopen(t_outfile, "w")))
		fatal("unable to open temperature trace file for output\n");

	/* names of functional units	*/
	if(read_names(pin, names) != n)
		fatal("no. of units in floorplan and trace file differ\n");

	/* header line of temperature trace	*/	
	write_names(tout, names, n);

	/* read the instantaneous power trace	*/
	while ((num=read_vals(pin, vals)) != 0)
	{
		if(num != n)
			fatal("invalid trace file format\n");

		/* permute the power numbers according to the floorplan order	*/
		for(i=0; i < n; i++)	
			power[get_blk_index(flp, names[i])] = vals[i];

		/* compute temperature	*/
		compute_temp(power, temp, n, sampling_intvl);

		/* permute back to the trace file order	*/
		for(i=0; i < n; i++)	
			vals[i] = temp[get_blk_index(flp, names[i])];
	
		/* output instantaneous temperature trace	*/
		write_vals(tout, vals, n);

		/* for computing average	*/
		for(i=0; i < n; i++)
			overall_power[i] += power[i];
		lines++;
	}

	if(!lines)
		fatal("no power numbers in trace file\n");
		
	/* for computing average	*/
	for(i=0; i < n; i++)
		overall_power[i] /= lines;

	/* steady state temperature	*/
	steady_state_temp(overall_power, steady_temp, n);

	/* print steady state results	*/
	printf("Unit\tSteady\n");
	for (i=0; i < n; i++)
		printf("%s\t%.2f\n", flp->units[i].name, steady_temp[i]-273.15);
	for (i=0; i < n; i++)
		printf("interface_%s\t%.2f\n", flp->units[i].name, steady_temp[IFACE*n+i]-273.15);
	for (i=0; i < n; i++)
		printf("spreader_%s\t%.2f\n", flp->units[i].name, steady_temp[HSP*n+i]-273.15);
	printf("%s\t%.2f\n", "spreader_west", steady_temp[NL*n+SP_W]-273.15);
	printf("%s\t%.2f\n", "spreader_east", steady_temp[NL*n+SP_E]-273.15);
	printf("%s\t%.2f\n", "spreader_north", steady_temp[NL*n+SP_N]-273.15);
	printf("%s\t%.2f\n", "spreader_south", steady_temp[NL*n+SP_S]-273.15);
	printf("%s\t%.2f\n", "spreader_bottom", steady_temp[NL*n+SP_B]-273.15);
	printf("%s\t%.2f\n", "sink_west", steady_temp[NL*n+SINK_W]-273.15);
	printf("%s\t%.2f\n", "sink_east", steady_temp[NL*n+SINK_E]-273.15);
	printf("%s\t%.2f\n", "sink_north", steady_temp[NL*n+SINK_N]-273.15);
	printf("%s\t%.2f\n", "sink_south", steady_temp[NL*n+SINK_S]-273.15);
	printf("%s\t%.2f\n", "sink_bottom", steady_temp[NL*n+SINK_B]-273.15);

	/* dump steady state temperatures on to file if needed	*/
	if (steady_file)
		dump_temp (flp, steady_temp, steady_file);

	/* cleanup	*/
	fclose(pin);
	fclose(tout);
	cleanup_hotspot(n);
	free_flp(flp);
	free_vector(temp);
	free_vector(power);
	free_vector(steady_temp);
	free_vector(overall_power);

	return 0;
}
