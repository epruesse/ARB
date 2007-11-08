#include <stdio.h>
#include <stdlib.h>
// #include <malloc.h>
#include <string.h>
#include <arbdb.h>
#include <arbdbt.h>

#define ADD_LEN        10
#define PRM_HASH_SIZE  1024
#define PRM_HASH2_SIZE 512
#define PRM_BUFFERSIZE 256


struct arb_prm_struct {
	char **alignment_names;
	int	al_len;
	int	max_name;
	GBDATA	*gb_main;
	char	buffer[PRM_BUFFERSIZE];
	char	*source;
	int	prmanz;
	int	prmlen;
	int	prmsmin;
	GB_HASH *hash;
	char	**data;
	int	sp_count;
	int	key_cnt;
	int	one_key_cnt;
	int	reduce;
	FILE	*out;
	char	*outname;
} aprm;


void arb_prm_menu()
	{
	char **alignment_name;
	int	i;
	printf(" Please select an Alignment:\n");
	for (	alignment_name = aprm.alignment_names,i=1;
		*alignment_name;
		alignment_name++,i++){
		printf("%i:	%s\n",i,*alignment_name);
	}
	aprm.max_name = i;
	fgets(aprm.buffer, PRM_BUFFERSIZE, stdin);
	i = atoi(aprm.buffer);
	if ((i<1) || (i>=aprm.max_name)) {
		printf ("ERROR: select %i out of range\n",i);
		exit(-1);
	}
	aprm.source = aprm.alignment_names[i-1];
	printf(	"This module will search for primers for all positions.\n"
		"	The best result is one primer for all (marked) taxa , the worst case\n"
		"	are n primers for n taxa.\n"
		"	Please specify the maximum number of primers:\n"
		);
	fgets(aprm.buffer, PRM_BUFFERSIZE, stdin);
	i = atoi(aprm.buffer);
	aprm.prmanz = i;
	printf( "Select minimum length of a primer, the maximum will be (minimum + %i)\n",
		ADD_LEN);
	fgets(aprm.buffer, PRM_BUFFERSIZE, stdin);
	i = atoi(aprm.buffer);
	if ((i<4) || (i>30)) {
		printf ("ERROR: select %i out of range\n",i);
		exit(-1);
	}
	aprm.prmlen = i;

	printf( "There may be short sequences or/and deletes in full sequences\n"
		"	So a primer normally does not match all sequences\n"
		"	Specify minimum percentage of species (0-100 %%):\n");
	fgets(aprm.buffer, PRM_BUFFERSIZE, stdin);
	i = atoi(aprm.buffer);
	if ((i<1) || (i>100)) {
		printf ("ERROR: select %i out of range\n",i);
		exit(-1);
	}
	aprm.prmsmin = i;

	printf( "Write output to file (enter \"\" to write to screen)\n");
	fgets(aprm.buffer, PRM_BUFFERSIZE, stdin);
	aprm.outname = strdup(aprm.buffer);
}

char *arb_prm_read(int /*prmanz*/)
	{
	GBDATA *gb_presets;
	GBDATA *gb_source;
	GBDATA	*gb_species;
	GBDATA	*gb_source_data;
	GBDATA	*gb_len;
	int	sp_count;
	//	char	*flag;
	char	*data;
	char	*hdata;
	//	int	size;

	gb_presets = GB_find(aprm.gb_main,"presets",0,down_level);

	gb_source = GB_find(gb_presets,"alignment_name",aprm.source,down_2_level);
	gb_len = GB_find(gb_source,"alignment_len",0,this_level);
	aprm.al_len = GB_read_int(gb_len);


	sp_count = GBT_count_marked_species(aprm.gb_main);

	aprm.data = (char **)calloc(sp_count,sizeof(char *));
	sp_count = 0;
	for (	gb_species = GBT_first_marked_species(aprm.gb_main);
		gb_species;
		gb_species = GBT_next_marked_species(gb_species) ){

		gb_source = GB_find(gb_species,aprm.source,0,down_level);
		if (!gb_source) continue;
		gb_source_data = GB_find(gb_source,"data",0,down_level);
		if (!gb_source_data) continue;
		data = (char *)calloc(sizeof(char),aprm.al_len+1);
		hdata = GB_read_char_pntr(gb_source_data);
		if (!hdata) {
			GB_print_error();
			continue;
		}
		aprm.data[sp_count ++] = data;
		if (sp_count %50 == 0)	printf("Reading taxa %i\n",sp_count);
		{
			int i,size;char c;
			size = GB_read_string_count(gb_source_data);
			for (i=0;i<size;i++) {
				c = hdata[i];
				if ( (c>='a') && (c<='z'))
					data[i] = c-'a'+'A';
				else
					data[i] = c;
			}
			for (i=i ; i < aprm.al_len; i++) data[i] = '.';
		}
	}
	printf("%i taxa read\n",sp_count);
	aprm.sp_count = sp_count;
	if (sp_count ==0) {
		exit(0);
	}
	return 0;
}

long arb_count_keys(const char */*key*/,long val)
	{
	if (val >1) {
		aprm.key_cnt++;
	}else{
		aprm.one_key_cnt++;
	}
	return val;
}

long arb_print_primer(const char *key,long val)
{
	if (val <=1) return val;
	int gc = 0;
	const char *p;
	for (p = key; *p; p++) {
		if (*p == 'G' || *p== 'C') gc++;
	}
	fprintf(aprm.out,"	%s matching %4li taxa     GC = %3i%%\n",
		key,val,100*gc/(int)strlen(key));
	return val;
}

#define is_base(c) ( ((c>='a') && (c<='z')) || ( (c>='A')&&(c<='Z') ) )

int primer_print(char *dest,char * source,int size)
	{
	char c;
	c = *(source++);
	if (!is_base(c)) return 1;
	while (size){
		while (!is_base(c)) c=*(source++);
		if ( c == 'N' || c == 'n' ) return 1;
		*(dest++) = c;
		size--;
		if (!c) return 1;
		c = 0;
	}
	*dest = 0;
	return 0;
}

long arb_reduce_primer_len(const char *key,long val)
	{
	static char buffer[256];
	int	size = strlen(key)-aprm.reduce;
	strncpy(buffer,key,size);
	buffer[size] = 0;
	val += GBS_read_hash(aprm.hash,buffer);
	GBS_write_hash(aprm.hash,buffer,val);
	return val;
}

void arb_prm_primer(int /*prmanz*/)
{
	GB_HASH         *hash;
	GB_HASH		*mhash;
	int             sp;
	//	int             len;
	char           *buffer;
	int             pos;
	int             prmlen;
	int             pspecies;

	int		cutoff_cnt;

	int		*best_primer_cnt;
	int		*best_primer_new;
	int		*best_primer_swap;
	//	int		newlen;

	prmlen = aprm.prmlen + ADD_LEN;

	buffer = (char *) calloc(sizeof(char), prmlen + 1);
	best_primer_cnt = (int *)calloc(prmlen+1,sizeof(int));
	best_primer_new = (int *)calloc(prmlen+1,sizeof(int));

	for (pos = 0; pos < aprm.al_len; pos++) {
		prmlen = aprm.prmlen + ADD_LEN;
		mhash = GBS_create_hash(PRM_HASH_SIZE,0);
		pspecies = 0;
		if (pos % 50 == 0) printf("Pos. %i (%i)\n",pos,aprm.al_len);
		cutoff_cnt = aprm.prmanz+1;
		for (sp = 0; sp < aprm.sp_count; sp++) {	/* build initial hash table */
			if (!primer_print(buffer, aprm.data[sp] + pos, prmlen)) {
				GBS_incr_hash(mhash, buffer);
				pspecies++;
			}
		}
		if (pspecies*100 >= aprm.prmsmin * aprm.sp_count ) {	/* reduce primer length */
			for (hash = mhash; prmlen >= aprm.prmlen; prmlen-=aprm.reduce) {
				hash = GBS_create_hash(aprm.prmanz*2,0);
				aprm.hash = hash;

				aprm.key_cnt = 0;
				aprm.one_key_cnt = 0;
				GBS_hash_do_loop(mhash, arb_count_keys);
				if ((aprm.key_cnt + aprm.one_key_cnt < cutoff_cnt) &&
				//	(aprm.key_cnt > aprm.one_key_cnt) &&
					(aprm.key_cnt<best_primer_cnt[prmlen+1])){
					fprintf(aprm.out,"%3i primer found len %3i(of %4i taxa) for position %i\n", aprm.key_cnt, prmlen, pspecies, pos);
					GBS_hash_do_loop(mhash, arb_print_primer);
					fprintf(aprm.out,"\n\n");
					cutoff_cnt = aprm.key_cnt;
				}
				best_primer_new[prmlen] = aprm.key_cnt;
				aprm.reduce = 1;
				while (aprm.key_cnt > aprm.prmanz*4){
					aprm.key_cnt/=4;
					aprm.reduce++;
				}
				GBS_hash_do_loop(mhash,arb_reduce_primer_len);
				GBS_free_hash(mhash);
				mhash = hash;
			}
		}else{
			for (;prmlen>0;prmlen--) best_primer_new[prmlen] = aprm.prmanz+1;
		}
		GBS_free_hash(mhash);
		best_primer_swap = best_primer_new;
		best_primer_new = best_primer_cnt;
		best_primer_cnt = best_primer_swap;
		mhash = 0;
	}

}

int main(int argc, char **/*argv*/)
	{
	char *error;
	const char *path;
	if (argc != 1) {
		fprintf(stderr,"no parameters\n");
		printf(" Converts RNA or DNA Data to Pro\n");
		exit(-1);
	}
	path = ":";
	aprm.gb_main = GB_open(path,"r");
	if (!aprm.gb_main){
		fprintf(stderr,"ERROR cannot find server\n");
		exit(-1);
	}
	GB_begin_transaction(aprm.gb_main);
	aprm.alignment_names = GBT_get_alignment_names(aprm.gb_main);
	GB_commit_transaction(aprm.gb_main);
	arb_prm_menu();
	GB_begin_transaction(aprm.gb_main);
	error = arb_prm_read(aprm.prmanz);
	if (error) {
		printf("ERROR: %s\n",error);
		exit(0);
	}
	GB_commit_transaction(aprm.gb_main);
	if (strlen(aprm.outname)) {
		aprm.out = fopen(aprm.outname,"w");
		if (!aprm.out) {
			printf("Cannot open outfile\n");
			exit (-1);
		}
		arb_prm_primer(aprm.prmanz);
		fclose(aprm.out);
	}else{
		aprm.out = stdout;
		arb_prm_primer(aprm.prmanz);
	}
	return 0;
}
