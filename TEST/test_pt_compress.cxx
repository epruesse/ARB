#include <stdio.h>
#include <string.h>
#include <memory.h>
// #include <malloc.h>
#include <arbdb.h>
#include <arbdbt.h>
#include "test.hxx"
#include <BI_helix.hxx>

void pt_align(FILE *out, int &pos, int mod) {
	int m = pos % mod;
	if (!m) return;
	while (m<mod) {
		if (out) putc(0,out);
		pos++;
		m++;
	}
}

char *pt_getstring(FILE *in){
	char *p,*buffer;
	int c;
	void *strstruct = GBS_stropen(1024);
	GBS_strcat(strstruct,0);
	for (c = 1; c; ) {
		c = getc(in);
		if (c== EOF) return 0;
		GBS_chrcat(strstruct,c);
	}
	p = GBS_strclose(strstruct);

	if (!p[0]){
		delete p;
		return 0;
	}
	if (p[0] == 1) {
		buffer = strdup(p+1);
	}else{
		buffer = strdup(p);
	}
	delete p;
	return buffer;
}

int pt_putstring(char *s,FILE *out){
	if (!s) {
		putc(0,out);
		return 0;
	}
	int len = strlen(s)+1;
	putc(1,out);
	len = fwrite(s,len,1,out);
	if (1 != len){
		return -1;
	}
	return 0;
}


master_gap_compress::master_gap_compress(void) {
	memset((char *)this,0,sizeof(master_gap_compress));
}
client_gap_compress::client_gap_compress(master_gap_compress *mgc) {
	memset((char *)this,0,sizeof(client_gap_compress));
	master = mgc;
	memsize = MEM_ALLOC_INIT_CLIENT;
	l = (client_gap_compress_data *)calloc(sizeof(client_gap_compress_data),memsize);
}

master_gap_compress::~master_gap_compress(void) {
	delete rel_2_abs;
	delete rel_2_abss;
}

client_gap_compress::~client_gap_compress(void) {
	delete l;
}

void client_gap_compress::clean(master_gap_compress *mgc){
	if (memsize < MEM_ALLOC_INIT_CLIENT) {
		delete l;
		memset((char *)this,0,sizeof(client_gap_compress));
		memsize = MEM_ALLOC_INIT_CLIENT;
		l = (client_gap_compress_data *)calloc(sizeof(client_gap_compress_data),memsize);
	}else{
		client_gap_compress_data *oldl = l;
		int old_memsize = memsize;
		memset((char *)this,0,sizeof(client_gap_compress));
		memsize = old_memsize;
		l = oldl;
	}
	master = mgc;
}

void master_gap_compress::optimize(void) {
	if (!rel_2_abs) return;
	if (rel_2_abs[len-1] > 0x7fff) {
		rel_2_abs = (long *)realloc((char *)rel_2_abs,len*sizeof(long));
	}else{
		rel_2_abss = (short *)malloc(sizeof(short) * len);
		int i;
		for (i=0;i<len;i++) {
			rel_2_abss[i] = (short) rel_2_abs[i];
		}
		delete rel_2_abs; rel_2_abs = 0;
	}

	memsize = len;
}

void master_gap_compress::write(long rel, long abs){
	if (rel_2_abss) GB_CORE;	// no write after optimize

	if (!rel_2_abs) {
		memsize = MEM_ALLOC_INIT_MASTER;
		rel_2_abs = (long *)calloc(sizeof(long),memsize);
	}

	if (rel >= memsize) {
		memsize = memsize * MEM_ALLOC_FACTOR;
		rel_2_abs = (long *)realloc((char *)rel_2_abs,sizeof(long)*memsize);
	}
	for( ; len<rel; len++) rel_2_abs[len] = 0;

	rel_2_abs[rel] = abs;
	if (len <= rel) len = (int)rel+1;
}

long master_gap_compress::read(long rel, long def_abs){
	if (rel>=len) {
		this->write(rel,def_abs);
		return def_abs;
	}
	if (rel_2_abss) return rel_2_abss[rel];
	return rel_2_abs[rel];
}

void client_gap_compress::basic_write(long rel, long x, long y){

	if (rel >= memsize) {
		memsize = (int)rel * MEM_ALLOC_FACTOR;
		l = (client_gap_compress_data *)realloc(
				(char *)l,sizeof(client_gap_compress_data)*memsize);
	}
	l[rel].rel = rel;
	l[rel].x = (short)x;
	l[rel].y = y;
	if (len <= rel) len = (int)rel+1;
}

void client_gap_compress::write(long rel, long abs) {
	if (rel >= master->len) {
		master->write(rel,abs);
		this->basic_write(rel,0,0);
		return;
	}
	register long m,r;
	register long *ml = master->rel_2_abs;
	old_rel++;
	old_m++;
		// search ?:	rel_2_abs[?] <= abs	&& 	rel_2_abs[?+1] > abs

	if ( rel == old_rel  && old_m < master->len && (r=ml[old_m] ) >= abs ) {
		if (r == abs) {
			m = old_m;
		}else{
			if (ml[ old_m -1 ] > abs) goto cgc_search_by_divide;
			old_m--;
			m = old_m;
		}
	}else{
		cgc_search_by_divide:
		long l;
		l = 0; r = master->len-1;
		while (l<r-1) {			// search the masters rel_2_abs[?] == abs
			m = (l+r)>>1;
			if ( ml[m] <= abs ) {
				l = m;
				continue;
			}
			r = m;
		}
		if ( ml[r] <= abs) m = r; else m = l;
		old_rel = rel;
		old_m = m;
	}
	this->basic_write(rel,m - rel,abs - ml[m] );
}

	// remove (eval. ) duplicated entries in client compress
	//  eg.   (4,20,5) (5,21,5) -> delete second

void client_gap_compress::optimize(int realloc_flag) {
	if (!len) return;
	register int i;
	int newlen = 0;
	register long old_x = -9999;
	register long old_y = -9999;
	register long old_dx = 0;

	for (i=0; i<len; i++) {
		if ( l[i].x != old_x || l[i].y != old_y ) {
			if (i<len+1) {
				old_dx = l[i+1].x - l[i].x;
				if (old_dx < -1 ) old_dx = 0;
				if (old_dx > 0 ) old_dx = 0;
			}
			old_x = l[i].x;
			old_y = l[i].y;
			l[i].dx = (short)old_dx;
			l[newlen++] = l[i];
		}
		old_x += old_dx;
		old_y -= old_dx;
	}
	if (realloc_flag) {
		l = (struct client_gap_compress_data *)realloc((char *)l,
			sizeof(struct client_gap_compress_data) * newlen);
	}
	len = memsize = newlen;
}


long client_gap_compress::rel_2_abs(long rel){
	register int left,right;
	register int m;
	long lrel;
	right = len-1;
	left = 0;
	if (left>=right) {		// client is master
		return master->read(rel,0);
	}
	while (left<right-1) {			// search the clients l[??].rel <= rel
						// && l[??+1] > rel
		m = (left+right)>>1;
		lrel  = l[m].rel;
		if ( lrel <= rel ) {
			left = m;
			continue;
		}
		right = m;
	}
	if ( l[right].rel <= rel) m = right; else m = left;
	long master_pos = l[m].rel;
	long pos;
	if (!l[m].dx) {		// substitution or deletion
		master_pos += l[m].x;		// masterpos offset
		master_pos += (rel - l[m].rel); // compressed data offset
		pos = master->read(master_pos	,0)	// read master at rel+offset
		+ l[m].y;
	}else{			// insertion
		master_pos += l[m].x;
		pos = master->read(master_pos	,0)	// read master at rel+offset
			 + (rel - l[m].rel)		// + offset (missing data in compressed )
			 + l[m].y;
	}
	return pos;
}

gap_compress::gap_compress(void) {
	memset((char *)this,0,sizeof(gap_compress));
	memclients = MEM_ALLOC_INIT_CLIENTS;
	clients = (client_gap_compress **)calloc(sizeof(void *),memclients);
	memmasters = MEM_ALLOC_INIT_MASTERS;
	masters = (master_gap_compress **)calloc(sizeof(void *),memmasters);
}

gap_compress::~gap_compress(void) {
	int i;
	for (i=0;i<nmasters;i++) delete masters[i];
	delete masters;
	for (i=0;i<nclients;i++) delete clients[i];
	delete clients;
}

void client_gap_compress::basic_write_sequence(char *sequence, long seq_len, int gap1,int gap2){
	long rel = 0;
	long abs = 0;
	rel = 0;
	for (abs = 0; abs < seq_len; abs++) {
		if ( sequence[abs] == gap1 ) continue;
		if ( sequence[abs] == gap2 ) continue;
		this->write(rel,abs);
		rel++;
	}
	this->abs_seq_len = seq_len;
	this->rel_seq_len = rel;
}
		// create a new master and client sequence
void client_gap_compress::basic_write_master_sequence(char *sequence, long seq_len, int gap1,int gap2){
	long rel = 0;
	long abs = 0;

	for (abs = 0; abs < seq_len; abs++) {		// write sequence
		if ( sequence[abs] == gap1 ) continue;
		if ( sequence[abs] == gap2 ) continue;
		master->write(rel,abs);
		rel++;
	}
	this->basic_write(0,0,0);

	this->abs_seq_len = seq_len;
	this->rel_seq_len = rel;
}

master_gap_compress *gap_compress::search_best_master(client_gap_compress *client,
		master_gap_compress *exclude,long max_diff,
		char *sequence, long seq_len, int gap1,int gap2, int &best_alt) {

	long best = max_diff;
	best_alt = -1;
	master_gap_compress *best_master = 0;
	int best_try;
	master_gap_compress *master;
	int	_try = 0;

	if (!nmasters) best_alt = (int)max_diff;

	for (_try = 0;_try<nmasters;_try++) {
		master = masters[_try];
		if (master == exclude) continue;
		client->clean(master);
		client->basic_write_sequence(sequence,seq_len,gap1,gap2);
		client->optimize(0);
		if (best_alt < 0 || client->len < best_alt) best_alt = client->len;
		if (best <0 || client->len < best) {
			best = client->len;
			best_master = master;
			best_try = _try;
			if (best <=3 ) break;	// very good choice, stop search
		}
	}
	if (!best_master) return 0;

	int i;
	for (i=best_try; i>0; i--) masters[i] = masters[i-1];
	masters[0] = best_master;	// bring found client to top of list

	client->clean(best_master);
	client->basic_write_sequence(sequence,seq_len,gap1,gap2);
	client->optimize(1);
	return best_master;
}

long sort_masters(void *m1, void *m2, char *cd ){
	cd = cd;
	master_gap_compress *master1 = (master_gap_compress *)m1;
	master_gap_compress *master2 = (master_gap_compress *)m2;
	if( 	master1->ref_cnt * master1->alt_master >
		master2->ref_cnt * master2->alt_master )
		return -1;
	return 0;
}

gap_index gap_compress::write_sequence(char *sequence, long seq_len, int gap1,int gap2){
	client_gap_compress *client = 0;
	client = new client_gap_compress(0);
	master_gap_compress *master;
	int i;
	long max_diff = REL_DIFF_CLIENT_MASTER(seq_len);
	int	alt;			// alternate master !!!!


	master = search_best_master(client,0,max_diff, sequence, seq_len, gap1,gap2, alt);

	if (!master){				// create a new master !!
		master = new master_gap_compress();
		master->alt_master = alt;
		client->clean(master);
		master->main_child = client;
		client->basic_write_sequence(sequence,seq_len,gap1,gap2);
		client->optimize(1);

		if (nmasters >= memmasters) {
			memmasters = memmasters * MEM_ALLOC_FACTOR;
			masters = (master_gap_compress **)realloc(
				(char *)masters,sizeof(void *)*memmasters);
		}
		for (i=nmasters; i>0; i--) masters[i] = masters[i-1];
		nmasters++;
		masters[0] = master;
	}
	master->ref_cnt++;

	if (nmasters >= MAX_MASTERS) {	// maximum masters exceeded delete
		int j;			// search single refed masters and search new master
					// for the only single child
		for (j=0;j<MAX_MASTERS_REMOVE_N;j++){
			GB_mergesort((void **)masters,0,nmasters, sort_masters,0);
			i = nmasters-1;
			int cli;
			for (cli = 0; cli < nclients; cli++) {
				client_gap_compress *cl2 = clients[cli];
				if ( cl2->master != masters[i] ) continue;
				char *seq2 = this->read_sequence(cli);
				long seq_len2 = cl2->abs_seq_len;
				master = search_best_master(clients[cli],
						masters[i],-1,
						seq2, seq_len2, '-','-',alt);
				delete seq2;
				master->ref_cnt++;
			}
			nmasters--;
			delete masters[i];
		}
	}

	if (nclients >= memclients) {
		memclients = memclients * MEM_ALLOC_FACTOR;
		clients = (client_gap_compress **)realloc(
			(char *)clients,sizeof(void *)*memclients);
	}
	clients[nclients++] = client;
	return nclients-1;
}

void gap_compress::optimize(void) {
	int i;
	for (i=0;i<nmasters;i++) masters[i]->optimize();
	masters = (master_gap_compress **)realloc(
		(char *)masters,sizeof(master_gap_compress) * nmasters);
	memmasters = nmasters;
	clients = (client_gap_compress **)realloc(
		(char *)clients,sizeof(client_gap_compress) * nclients);
	memclients = nclients;
}

long gap_compress::rel_2_abs(gap_index index, long rel){
	if (index >= nclients) GB_CORE;
	return clients[index]->rel_2_abs(rel);
}

char *gap_compress::read_sequence(gap_index index){
	if (index >= nclients) GB_CORE;
	client_gap_compress *client = clients[index];
	int i;
	long abs;
	char *res = (char *)malloc((int)client->abs_seq_len+1);
	memset(res,'-',(int)client->abs_seq_len);
	res[client->abs_seq_len] = 0;
	for (i=0;i<client->rel_seq_len; i++) {
		abs = client->rel_2_abs(i);
		res[abs] = '+';
	}
	return res;
}

void gap_compress::statistic(int full){

	int i;
	printf("nmasters %i\n",nmasters);
	printf("nclients %i\n",nclients);
	if (full) {
		long sum = 0;
		for (i=0;i<nmasters;i++) {
			printf("	%i:	ref %i	alt %i\n",
			i,masters[i]->ref_cnt,masters[i]->alt_master);
		}
		for (i=0;i<nclients;i++) {
			printf("%i ",clients[i]->len);
			sum += clients[i]->len;
		}
		printf("\nsum len clients %i  avg %f\n",sum,(double)sum/nclients);
#if 0
		client_gap_compress *client = clients[nclients-1];
		for (i=0 ; i < client->len; i++){
			printf("	%i	rel   %i	x %i	dx %i	y %i\n",
				i,client->l[i].rel,client->l[i].x,client->l[i].dx,client->l[i].y);
		}
#endif
	}
}

int master_gap_compress::save(FILE *out, int index_write, int &pos){
	if (index_write) {
		putw(len,out);
		if (rel_2_abs) {
			pt_align(0,pos,sizeof(long));
			putc(1,out);
		}else{
			pt_align(0,pos,sizeof(short));
			putc(0,out);
		}
		putw(pos,out);
	}else{
		if (rel_2_abs) {
			pt_align(out,pos,sizeof(long));
			if (len != fwrite((char *)rel_2_abs,sizeof(long), len, out)){
				return -1;
			}
		}else{
			pt_align(out,pos,sizeof(short));
			if (len != fwrite((char *)rel_2_abss,sizeof(short), len, out)){
				return -1;
			}
		}
	}
	if (rel_2_abs) {
		pos += len * sizeof(long);
	}else{
		pos += len * sizeof(short);
	}
	return 0;
}

int master_gap_compress::load(FILE *in, char *baseaddr){
	len = getw(in);
	if (getc(in) != 0) {
		rel_2_abs = (long *)(getw(in) + baseaddr);
	}else{
		rel_2_abss = (short *)(getw(in) + baseaddr);
	}
	return 0;
}

int client_gap_compress::save(FILE *out, int index_write, int &pos){
	if (index_write) {
		putw(master->index,out);
		putw(len,out);
		pt_align(0,pos,sizeof(long));
		putw(pos,out);
		putw((int)abs_seq_len,out);
		putw((int)rel_seq_len,out);
	}else{
		pt_align(out,pos,sizeof(long));
		if (len != fwrite((char *)l,sizeof(client_gap_compress_data), len, out)){
			return -1;
		}
	}
	pos += sizeof(client_gap_compress_data) * len;
	return 0;
}

int client_gap_compress::load(FILE *in, char *baseaddr,master_gap_compress **masters){
	int master_index = getw(in);
	master = masters[master_index];
	len = getw(in);
	l = (client_gap_compress_data *)(getw(in) + baseaddr);
	abs_seq_len = getw(in);
	rel_seq_len = getw(in);
	return 0;
}

int gap_compress::save(FILE *out, int index_write, int &pos){
	if (index_write) {
		putw(nmasters,out);
		putw(nclients,out);
	}
	long i;
	for (i=0;i<nmasters;i++){
		if(masters[i]->save(out,index_write,pos)){
			return -1;
		}
		masters[i]->index = (int)i;
	}
	for (i=0;i<nclients;i++){
		if (clients[i]->save(out,index_write,pos)){
			return -1;
		}
	}
	return 0;
}

int gap_compress::load(FILE *in, char *baseaddr){
	delete masters;
	delete clients;
	int i;
	memmasters = nmasters = getw(in);
	memclients = nclients = getw(in);
	clients = (client_gap_compress **)calloc(sizeof(void *),memclients);
	masters = (master_gap_compress **)calloc(sizeof(void *),memmasters);

	for (i=0;i<nmasters;i++){
		masters[i] = (master_gap_compress *)calloc(sizeof(master_gap_compress),1);
		masters[i]->load(in,baseaddr);
	}
	for (i=0;i<nclients;i++){
		clients[i] = (client_gap_compress *)calloc(sizeof(client_gap_compress),1);
		clients[i]->load(in,baseaddr,masters);
	}
	return 0;
}

	// pt_species_class::set destroys the sequence !!!!!!
	// name and fullname must be a new copy
void pt_species_class::set(char *sequence, long seq_len, char *name, char *fullname){
	delete this->fullname;
	this->fullname = fullname;
	delete this->name;
	this->name = name;

	delete this->sequence;
	delete this->abs_sequence;

	this->abs_sequence = sequence;
	this->sequence = (char *)calloc(sizeof(char), (int)seq_len+1);


	char	c;
	char	*src,
		*dest,
		*abs;
	src = sequence;
	dest = this->sequence;
	abs = this->abs_sequence;
	char	*first_n = dest;	// maximum number of consequtive n's
	char	*first_abs_n = abs;

#define SET_NN first_n = dest; first_abs_n = abs

	while(c=*(src++)){
		switch (c) {
			case 'A':
			case 'a': *(dest++) = PT_A;*(abs++) = '+';SET_NN; break;
			case 'C':
			case 'c': *(dest++) = PT_C;*(abs++) = '+';SET_NN; break;
			case 'G':
			case 'g': *(dest++) = PT_G;*(abs++) = '+';SET_NN; break;
			case 'U':
			case 'u':
			case 'T':
			case 't': *(dest++) = PT_T;*(abs++) = '+';SET_NN; break;
			case '.': *(dest)++ = PT_QU;*(abs++) = '+';
					while (*src =='.') { src++; *(abs++) = '-'; }
					SET_NN;
					break;
			case '-': *(abs++) = '-'; break;
			default:
				if (dest-first_n < PT_MAX_NN) {
					*(dest++) = PT_N;*(abs++) = '+';break;
				}else{
					while (first_abs_n < abs) *(first_abs_n++) = '-';
					dest = first_n;
					*(dest++) = PT_QU;
					*(abs++) = '+';
				}
				break;
		}

	}
	*dest = PT_QU;
	*abs = 0;
	if ( abs - this->abs_sequence != seq_len) GB_CORE;
	this->abs_seq_len = seq_len;
	this->seq_len = dest-this->sequence;
}
pt_species_class::pt_species_class(void){
	memset((char *)this,0,sizeof(pt_species_class));
}
pt_species_class::~pt_species_class(void){
	delete sequence;
	delete abs_sequence;
	delete name;
	delete fullname;
}

void pt_species_class::calc_gap_index(gap_compress *gc){
	index = gc->write_sequence(abs_sequence, abs_seq_len, '-', '-');
}

int	pt_species_class::save(FILE *out, int index_write, int &pos){
	if (index_write) {
		putw(pos,out);
		putw((int)abs_seq_len,out);
		putw((int)seq_len,out);
		if (pt_putstring(name,out) ){
			return -1;
		}
		if (pt_putstring(fullname,out) ){
			return -1;
		}
		putw((int)index,out);
	}else{
		if (seq_len+1 != fwrite(sequence,sizeof(char), (int)seq_len+1, out))
			return -1;
	}
	pos += (int)seq_len+1;
	return 0;
}


int	pt_species_class::load(FILE *in, char *baseaddr){
	sequence = getw(in) + baseaddr;
	abs_seq_len = getw(in);
	seq_len = getw(in);
	name = pt_getstring(in);
	fullname = pt_getstring(in);
	index = getw(in);
	return 0;
}

long	pt_species_class::rel_2_abs(long rel){
	return cgc->rel_2_abs(rel);
}

void	pt_species_class::test_print(void){
	char *buffer = (char *)calloc(sizeof(char), (int)abs_seq_len + 1);
	memset(buffer,'-',(int)abs_seq_len);
	int i;
	long abs;
	for (i=0; i <seq_len; i++) {
		abs = rel_2_abs(i);
		buffer[abs] = PT_BASE_2_ASCII[this->get(i)];
	}
	for (i=0;i< abs_seq_len; i+= 80) {
		printf("%i	",i);
		fwrite(buffer+i,1,80,stdout);
		printf("\n");
	}
	delete buffer;
}


pt_main_class::pt_main_class(void) {
	memset((char *)this,0,sizeof(pt_main_class));
}

pt_main_class::~pt_main_class(void) {
	GB_CORE;
}

void pt_main_class::calc_name_hash(void){
	long i;
	namehash = GBS_create_hash(PT_NAME_HASH_SIZE);
	for (i=0;i<data_count;i++){
		GBS_write_hash(namehash, species[i].name,i+1);
	}

}

void pt_main_class::calc_max_size_char_count(void){
	max_size = 0;
	long	i;
	for (i = 0; i < data_count; i++){	/* get max sequence len */
		max_size = (int)MAX(max_size, species[i].seq_len);
		char_count += species[i].seq_len;
	}
}

void pt_main_class::calc_bi_ecoli(void){
	if ( ecoli && strlen(ecoli) ){
		BI_ecoli_ref *ref = new BI_ecoli_ref;
		char *error = ref->init(ecoli,strlen(ecoli));
		if (error) {
			delete ecoli;
			ecoli = 0;
		}else{
			bi_ecoli = (void *)ref;
		}
	}
}
void pt_main_class::init(GBDATA *gb_main){
	GB_transaction dummy(gb_main);
	GBDATA *gb_extended_data = GB_search(gb_main, "extended_data", GB_CREATE_CONTAINER);
	GBDATA *gb_species_data = GB_search(gb_main, "species_data", GB_CREATE_CONTAINER);
	GBDATA *gb_extended;
	GBDATA *gb_species;
	GBDATA *gb_data;
	GBDATA *gb_name;
	alignment_name = GBT_get_default_alignment(gb_main);

	gb_extended = GBT_find_extended(gb_extended_data, GBT_get_default_ref(gb_main));
	delete ecoli;
	ecoli = 0;
	if (gb_extended) {
		gb_data = GBT_read_sequence(gb_extended,alignment_name);
		if (gb_data) {
			ecoli = GB_read_string(gb_data);
		}
	}
	data_count = 0;

	for (	gb_species = GBT_first_species_rel_species_data(gb_species_data);
		gb_species;
		gb_species = GBT_next_species(gb_species) ){
		gb_data = GBT_read_sequence(gb_extended,alignment_name);
		if (!gb_data) continue;
		gb_name = GB_search(gb_species,"name",GB_FIND);
		if (!gb_name) continue;
		data_count ++;
	}
}
		// pt_main_class::save does more than save: all the conversation is done
		// by this procedure

int pt_main_class::save(GBDATA *gb_main, FILE *out, int index_write, int &pos){

	GB_transaction dummy(gb_main);
	GBDATA *gb_species_data = GB_search(gb_main, "species_data", GB_CREATE_CONTAINER);
	GBDATA *gb_species;
	GBDATA *gb_name;
	GBDATA *gb_fullname;
	GBDATA *gb_data;
	char *name;
	char *fullname;
	pt_species_class *species = new pt_species_class;
	if (index_write){
		this->init(gb_main);		// calculate the number of species
		putw(data_count,out);
		pt_putstring(ecoli,out);
		gc = new gap_compress;
	}
	int count = 0;
	for (	gb_species = GBT_first_species_rel_species_data(gb_species_data);
		gb_species;
		gb_species = GBT_next_species(gb_species) ){
		gb_data = GBT_read_sequence(gb_species,alignment_name);
		if (!gb_data) continue;
		gb_name = GB_search(gb_species,"name",GB_FIND);
		if (!gb_name) continue;
		name = GB_read_string(gb_name);
		gb_fullname = GB_search(gb_species,"full_name",GB_FIND);
		if (gb_fullname) fullname = GB_read_string(gb_fullname);
		else		fullname = strdup("");
		char *seq = GB_read_string(gb_data);;
		long seq_len = GB_read_string_count(gb_data);
		species->set(seq,seq_len, name,fullname);
		if (index_write) {
			species->calc_gap_index(gc);
			if (count != species->index) GB_CORE;
		}
		if (species->save(out,index_write, pos)){
			return -1;
		}

		count ++;
		if (count % 100 == 0) {
			printf("%i ...\n",count);
		}
	}
	delete species;
	if (index_write) {
		gc->optimize();
		gc->statistic();
	}
	if (gc->save(out,index_write,pos)){
		return -1;
	}
	return 0;
}

int pt_main_class::load(FILE *in, char *baseaddr){
	data_count = getw(in);
	ecoli = pt_getstring(in);
	species = (pt_species_class *)calloc(sizeof(pt_species_class),data_count);
	int i;
	for (i=0;i<data_count;i++) {
		species[i].load(in,baseaddr);
	}

	gc = new gap_compress;
	if (gc->load(in,baseaddr)){
		return -1;
	}

	for (i=0;i<data_count;i++) {
		species[i].cgc = gc->clients[i];
	}

	calc_name_hash();
	calc_max_size_char_count();
	calc_bi_ecoli();

	return 0;
}

long pt_main_class::abs_2_ecoli(long pos)
	{
	if (!bi_ecoli) return pos;
	long res,dummy;
	((BI_ecoli_ref *)bi_ecoli)->abs_2_rel(pos,res,dummy);
	return res;
}

int pt_main_class::main_convert(GBDATA *gb_main, char *basename){
	FILE		*index_file;
	FILE		*sequence_file;
	char		*index_name = GBS_string_eval(basename,"*=*1.ind",0);
	char		*sequence_name = GBS_string_eval(basename,"*=*1.seq",0);

	index_file = fopen(index_name,"w");
	if (!index_file) {
		GB_ERROR("Error Cannot create and write to  file %s\n",index_name);
		return -1;
	}
	sequence_file = fopen(sequence_name,"w");
	if (!sequence_file) {
		GB_ERROR("Error Cannot create and write to  file %s\n",sequence_name);
		return -1;
	}
	GB_transaction dummy(gb_main);

	int pos = 0;
	if (this->save(gb_main, index_file,1,pos)) {
		GB_ERROR("Index Write Error\n"); return(-1);
	};
	fclose(index_file);
	pos = 0;
	if (this->save(gb_main, sequence_file,0,pos)) {
		GB_ERROR("Index Write Error\n"); return (-1);
	};
	fclose(sequence_file);
	printf("Sequence File length: %i\n",pos);
	return 0;
}

int pt_main_class::import_all(char *basename){
	FILE		*index_file;
	char		*index_name = GBS_string_eval(basename,"*=*1.ind",0);
	char		*sequence_name = GBS_string_eval(basename,"*=*1.seq",0);
	index_file = fopen(index_name,"r");
	if (!index_file) {
		fprintf(stderr,"Error Cannot create and write to  file %s\n",index_name);
		return -1;
	}
	char *baseaddr = GB_map_file(sequence_name);
	if (!baseaddr) {
		GB_print_error();
		return -1;
	}
	if (load(index_file, baseaddr) ) {
		return -1;
	}
	return 0;
}

int
main(int argc, char **argv)
{
	char           *path;
	GBDATA		*gb_main;
	pt_main_class	*mc = new pt_main_class;
	char		*filename = "out.arb";
	if (argc < 2){
		if (mc->import_all(filename)) {
			fprintf(stderr,"Load Error\n");
			return -1;
		}
		int i;
		for (i=0; i < mc->data_count ; i++ ) {
			mc->species[i].test_print();
		}
		return 0;
	}else{
		path = argv[1];
	}

	if (!(gb_main = GB_open(path, "r")))
		return (-1);
	if (mc->main_convert(gb_main,filename)) {
		fprintf(stderr,"Error\n");
		return -1;
	}
	GB_close(gb_main);
	return 0;
}

#if 0
	char		*cmp;
	int		i;
		cmp = gc.read_sequence(index);
		for (i=0;i<seq_len;i++) {
			if ( cmp[i] == '-' && (
				seq[i] == '-' || seq[i] == '.')) continue;
			if ( cmp[i] != '-' && (
				seq[i] != '-' && seq[i] != '.')) continue;
			break;
		}
		if (i<seq_len) {
			printf("sequences differ %i\n%s\n%s\n",count, seq,cmp);
		}

		delete cmp;
#endif
