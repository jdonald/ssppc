#include "flp.h"
#include "misc.h" // redefine fatal() /jdonald
#include "util.h"

#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>

void print_flp_fig (flp_t *flp)
{
	int i;
	unit_t *unit;

	fprintf(stderr, "FIG starts\n");
	for (i=0; i< flp->n_units; i++) {
		unit = &flp->units[i];
		fprintf(stderr, "%.5f %.5f %.5f %.5f %.5f %.5f %.5f %.5f %.5f %.5f\n", unit->leftx, unit->bottomy, \
				unit->leftx, unit->bottomy + unit->height, unit->leftx + unit->width, unit->bottomy + \
				unit->height, unit->leftx + unit->width, unit->bottomy, unit->leftx, unit->bottomy);
		fprintf(stderr, "%s\n", unit->name);
	}
	fprintf(stderr, "FIG ends\n");
}

/* LYM CMP TEMP */
// Originally written by Yingmin for Turandot /jdonald
flp_t *read_cmpflp(const char *file, const char *cmpfile, const char *attachfile)
{
    char str[STR_SIZE], copy[STR_SIZE], processor_name[STR_SIZE];
    FILE *fp = fopen(file, "r");
    FILE *cmpfp = fopen(cmpfile, "r");
	FILE *attachfp = fopen(attachfile, "r");
    flp_t *flp;
    int i=0, count = 0, cmpcount = 0, attachcount = 0, j=0;
    char *ptr;
    double left, bottom;
    double core_width = 0.0; // jdonald
    const int flip_core_flp = 1; // jdonald, for SS-PPC

    if (!fp) {
        sprintf(str, "error opening file %s\n", file);
        fatal(str);
    }

    if (!cmpfp) {
        sprintf(str, "error opening file %s\n", cmpfile);
        fatal(str);
    }

	if (!attachfp) {
		sprintf(str, "error opening file %s\n", attachfile);
		fatal(str);
	}

    while(!feof(fp)) {                /* first pass   */
    	fgets(str, STR_SIZE, fp);
        if (feof(fp))
        	break;
        ptr =   strtok(str, " \t\n");
		if ( ptr && ptr[0] != '#')      /* ignore comments and empty lines      */
        count++;
    }

    while(!feof(cmpfp)) {
        fgets(str, STR_SIZE, cmpfp);
        if (feof(cmpfp))
            break;
        ptr = strtok(str, " \t\n");
        if (ptr && ptr[0] != '#')
            cmpcount++;
    }

	while(!feof(attachfp)) {
		fgets(str, STR_SIZE, attachfp);
		if (feof(attachfp))
			break;
		ptr = strtok(str, " \t\n");
		if (ptr && ptr[0] != '#')
			attachcount++;
	}

    // if (cmpcount < num_cores)
    //	fatal("read_flp(): not enough cores available in floorplan file\n");

    if(!count)
		fatal("no units specified in the floorplan file\n");
    flp = (flp_t *) calloc (1, sizeof(flp_t));
    if(!flp)
    	fatal("memory allocation error\n");
    flp->units = (unit_t *) calloc (count*cmpcount+attachcount, sizeof(unit_t));
    if (!flp->units)
    	fatal("memory allocation error\n");
    flp->n_units = count*cmpcount+attachcount;
    flp->core_units = count; // jdonald
	flp->attach_units = attachcount;

    fseek(fp, 0, SEEK_SET);
    while(!feof(fp)) {               /* second pass  */
        fgets(str, STR_SIZE, fp);
        if (feof(fp))
	        break;
        strcpy(copy, str);
        ptr =   strtok(str, " \t\n");
        if ( ptr && ptr[0] != '#') {    /* ignore comments and empty lines      */
            if (sscanf(copy, "%s%lf%lf%lf%lf", flp->units[i].name, &flp->units[i].width, \
                  &flp->units[i].height, &flp->units[i].leftx, &flp->units[i].bottomy) != 5)
                fatal("invalid floorplan file format\n");
            if (flp->units[i].width + flp->units[i].leftx > core_width)
            	core_width = flp->units[i].width + flp->units[i].leftx;
        	i++;
        }
    }
    fclose(fp);

    fseek(cmpfp, 0 , SEEK_SET);
    i = 0;
    while(!feof(cmpfp)){
        fgets(str, STR_SIZE, cmpfp);
        if (feof(cmpfp))
        	break;
        strcpy(copy, str);
        ptr = strtok(str, " \t\n");
        if ( ptr && ptr[0] != '#'){
	    if (sscanf(copy, "%s%lf%lf", processor_name, &left, &bottom) != 3)
	        fatal("invalid cmp floorplan file format\n");
            if (i>0)
                for (j=0;j< count ;j++){
                    flp->units[i*count + j].name[0] = '\0';
                    strcat(flp->units[i*count + j].name,flp->units[j].name);
                    strcat(flp->units[i*count + j].name,"_");
                    strcat(flp->units[i*count + j].name,processor_name);
                    flp->units[i*count + j].width = flp->units[j].width;
                    flp->units[i*count + j].height = flp->units[j].height;
                    if (flip_core_flp && i >= (cmpcount+1)/2)
                    	flp->units[i*count + j].leftx = left + core_width - flp->units[j].leftx - flp->units[j].width; // horizontally flip the second half of cores
                    else
                        flp->units[i*count + j].leftx = flp->units[j].leftx + left;
                    flp->units[i*count + j].bottomy = flp->units[j].bottomy + bottom;
                }
            i++;
        }
    }

	fseek(attachfp, 0, SEEK_SET);
	i = 0;
    while(!feof(attachfp)) {               /* second pass  */
        fgets(str, STR_SIZE, attachfp);
        if (feof(attachfp))
            break;
        strcpy(copy, str);
        ptr =   strtok(str, " \t\n");
        if ( ptr && ptr[0] != '#') {    /* ignore comments and empty lines      */
            if (sscanf(copy, "%s%lf%lf%lf%lf", flp->units[i+ cmpcount*count].name, &flp->units[i+ cmpcount*count].width, \
                 &flp->units[i+ cmpcount*count].height, &flp->units[i+ cmpcount*count].leftx, &flp->units[i+ cmpcount*count].bottomy) != 5)
            fatal("invalid floorplan file format\n");
            i++;
    	}
    }
	fclose(attachfp);

    fseek(cmpfp, 0 , SEEK_SET);
    i = 0;
    while(!feof(cmpfp)){
        fgets(str, STR_SIZE, cmpfp);
        if (feof(cmpfp))
            break;
        strcpy(copy, str);
        ptr = strtok(str, " \t\n");
        if ( ptr && ptr[0] != '#'){
            if (sscanf(copy, "%s%lf%lf", processor_name, &left, &bottom) != 3)
                fatal("invalid cmp floorplan file format\n");
            for (j=0;j<count;j++){
                strcat(flp->units[i*count + j].name,"_");
                strcat(flp->units[i*count + j].name,processor_name);
				flp->units[i*count + j].leftx = flp->units[j].leftx + left;
				flp->units[i*count + j].bottomy = flp->units[j].bottomy + bottom;
				// these two lines added by jdonald to fix the first core's floorplan.
            }
            break; // it seemsthis statement ensures only the *first core* is affected by this loop. -jdonald
        }
    }

    fclose(cmpfp);

    // print_flp_fig(flp);
    // use the -printflp knob to print the floorplan
    return flp;
}
/* LYM CMP TEMP END */

flp_t *read_flp(char *file)
{
	char str[STR_SIZE], copy[STR_SIZE];
	FILE *fp = fopen(file, "r");
	flp_t *flp;
	int i=0, count = 0;
	char *ptr;

	if (!fp) {
		sprintf(str, "error opening file %s\n", file);
		fatal(str);
	}

	while(!feof(fp))		/* first pass	*/
	{
		fgets(str, STR_SIZE, fp);
		if (feof(fp))
			break;
		ptr =	strtok(str, " \t\n");
		if ( ptr && ptr[0] != '#')	/* ignore comments and empty lines	*/
			count++;
	}

	if(!count)
		fatal("no units specified in the floorplan file\n");

	flp = (flp_t *) calloc (1, sizeof(flp_t));
	if(!flp)
		fatal("memory allocation error\n");
	flp->units = (unit_t *) calloc (count, sizeof(unit_t));
	if (!flp->units)
		fatal("memory allocation error\n");
	flp->n_units = count;

	fseek(fp, 0, SEEK_SET);
	while(!feof(fp))		/* second pass	*/
	{
		fgets(str, STR_SIZE, fp);
		if (feof(fp))
			break;
		strcpy(copy, str);
		ptr =	strtok(str, " \t\n");
		if ( ptr && ptr[0] != '#') {	/* ignore comments and empty lines	*/
			if (sscanf(copy, "%s%lf%lf%lf%lf", flp->units[i].name, &flp->units[i].width, \
			&flp->units[i].height, &flp->units[i].leftx, &flp->units[i].bottomy) != 5)
				fatal("invalid floorplan file format\n");
			i++;
		}	
	}

	fclose(fp);	
	// print_flp_fig(flp);
	// no need to print this out if read_cmpflp() also prints out one.
	return flp;
}

void free_flp(flp_t *flp)
{
	free(flp->units);
	free(flp);
}

int get_blk_index(flp_t *flp, char *name)
{
	int i;
	char msg[STR_SIZE];

	if (!flp)
		fatal("null pointer in get_blk_index\n");

	for (i = 0; i < flp->n_units; i++) {
		if (!strcasecmp(name, flp->units[i].name)) {
			return i;
		}
	}

	sprintf(msg, "block %s not found\n", name);
	fatal(msg);
	return -1;
}

int is_horiz_adj(flp_t *flp, int i, int j)
{
	double x1, x2, x3, x4;
	double y1, y2, y3, y4;

	if (i == j) 
		return 0;

	x1 = flp->units[i].leftx;
	x2 = x1 + flp->units[i].width;
	x3 = flp->units[j].leftx;
	x4 = x3 + flp->units[j].width;

	y1 = flp->units[i].bottomy;
	y2 = y1 + flp->units[i].height;
	y3 = flp->units[j].bottomy;
	y4 = y3 + flp->units[j].height;

	/* diagonally adjacent => not adjacent */
	if (eq(x2,x3) && eq(y2,y3))
		return 0;
	if (eq(x1,x4) && eq(y1,y4))
		return 0;
	if (eq(x2,x3) && eq(y1,y4))
		return 0;
	if (eq(x1,x4) && eq(y2,y3))
		return 0;

	if (eq(x1,x4) || eq(x2,x3))
		if ((y3 >= y1 && y3 <= y2) || (y4 >= y1 && y4 <= y2) ||
		    (y1 >= y3 && y1 <= y4) || (y2 >= y3 && y2 <= y4))
			return 1;

	return 0;
}

int is_vert_adj (flp_t *flp, int i, int j)
{
	double x1, x2, x3, x4;
	double y1, y2, y3, y4;

	if (i == j)
		return 0;

	x1 = flp->units[i].leftx;
	x2 = x1 + flp->units[i].width;
	x3 = flp->units[j].leftx;
	x4 = x3 + flp->units[j].width;

	y1 = flp->units[i].bottomy;
	y2 = y1 + flp->units[i].height;
	y3 = flp->units[j].bottomy;
	y4 = y3 + flp->units[j].height;

	/* diagonally adjacent => not adjacent */
	if (eq(x2,x3) && eq(y2,y3))
		return 0;
	if (eq(x1,x4) && eq(y1,y4))
		return 0;
	if (eq(x2,x3) && eq(y1,y4))
		return 0;
	if (eq(x1,x4) && eq(y2,y3))
		return 0;

	if (eq(y1,y4) || eq(y2,y3))
		if ((x3 >= x1 && x3 <= x2) || (x4 >= x1 && x4 <= x2) ||
		    (x1 >= x3 && x1 <= x4) || (x2 >= x3 && x2 <= x4))
			return 1;

	return 0;
}

double get_shared_len(flp_t *flp, int i, int j)
{
	double p11, p12, p21, p22;
	p11 = p12 = p21 = p22 = 0.0;

	if (i==j) 
		return 0;

	if (is_horiz_adj(flp, i, j)) {
		p11 = flp->units[i].bottomy;
		p12 = p11 + flp->units[i].height;
		p21 = flp->units[j].bottomy;
		p22 = p21 + flp->units[j].height;
	}

	if (is_vert_adj(flp, i, j)) {
		p11 = flp->units[i].leftx;
		p12 = p11 + flp->units[i].width;
		p21 = flp->units[j].leftx;
		p22 = p21 + flp->units[j].width;
	}

	return (MIN(p12, p22) - MAX(p11, p21));
}

double get_total_width(flp_t *flp)
{	
	int i;
	double min_x = flp->units[0].leftx;
	double max_x = flp->units[0].leftx + flp->units[0].width;
	
	for (i=1; i < flp->n_units; i++) {
		if (flp->units[i].leftx < min_x)
			min_x = flp->units[i].leftx;
		if (flp->units[i].leftx + flp->units[i].width > max_x)
			max_x = flp->units[i].leftx + flp->units[i].width;
	}

	return (max_x - min_x);
}

double get_total_height(flp_t *flp)
{	
	int i;
	double min_y = flp->units[0].bottomy;
	double max_y = flp->units[0].bottomy + flp->units[0].height;
	
	for (i=1; i < flp->n_units; i++) {
		if (flp->units[i].bottomy < min_y)
			min_y = flp->units[i].bottomy;
		if (flp->units[i].bottomy + flp->units[i].height > max_y)
			max_y = flp->units[i].bottomy + flp->units[i].height;
	}

	return (max_y - min_y);
}

