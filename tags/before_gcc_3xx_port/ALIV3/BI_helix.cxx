#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
// #include <malloc.h>
#include <arbdb.h>
#include <arbdbt.h>



#ifdef _USE_AW_WINDOW
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#endif

#include <BI_helix.hxx>

#define LEFT_HELIX "{[<("
#define RIGHT_HELIX "}]>)"
#define LEFT_NONS "#*abcdefghij"
#define RIGHT_NONS "#*ABCDEFGHIJ"

char BI_helix::error[256];

struct helix_stack {
	struct helix_stack *next;
	long	pos;
	BI_PAIR_TYPE type;
	char	c;
};

struct {
	char *awar;
	BI_PAIR_TYPE pair_type;
	} helix_awars[] = {
	{ "Strong_Pair", HELIX_STRONG_PAIR },
	{ "Normal_Pair", HELIX_PAIR },
	{ "Weak_Pair", HELIX_WEAK_PAIR },
	{ "No_Pair", HELIX_NO_PAIR },
	{ "User_Pair", HELIX_USER0 },
	{ "User_Pair2", HELIX_USER1 },
	{ "User_Pair3", HELIX_USER2 },
	{ "User_Pair4", HELIX_USER3 },
	{ "Default", HELIX_DEFAULT },
	{ "Non_Standart_aA", HELIX_NON_STANDART0 },
	{ "Non_Standart1", HELIX_NON_STANDART1 },
	{ "Non_Standart2", HELIX_NON_STANDART2 },
	{ "Non_Standart3", HELIX_NON_STANDART3 },
	{ "Non_Standart4", HELIX_NON_STANDART4 },
	{ "Non_Standart5", HELIX_NON_STANDART5 },
	{ "Non_Standart6", HELIX_NON_STANDART6 },
	{ "Non_Standart7", HELIX_NON_STANDART7 },
	{ "Non_Standart8", HELIX_NON_STANDART8 },
	{ "Non_Standart9", HELIX_NON_STANDART9 },
	{ "Not_Non_Standart", HELIX_NO_MATCH },
	{ 0, HELIX_NONE },
};

#ifdef _USE_AW_WINDOW
BI_helix::BI_helix(AW_root * aw_root)
#else
BI_helix::BI_helix(void)
#endif
{
	int i;
	char awar[256];
	entries = 0;
	size = 0;

	pairs[HELIX_NONE]=strdup("");
	char_bind[HELIX_NONE] = strdup(" ");

	pairs[HELIX_STRONG_PAIR]=strdup("CG");
	char_bind[HELIX_STRONG_PAIR] = strdup("-");

	pairs[HELIX_PAIR]=strdup("AU GU");
	char_bind[HELIX_PAIR] = strdup("-");

	pairs[HELIX_WEAK_PAIR]=strdup("GA GT");
	char_bind[HELIX_WEAK_PAIR] = strdup(".");

	pairs[HELIX_NO_PAIR]=strdup("AA AC CC CT CU TU");
	char_bind[HELIX_NO_PAIR] = strdup("#");

	pairs[HELIX_USER0]=strdup(".A .C .G .T .U");
	char_bind[HELIX_USER0] = strdup("?");

	pairs[HELIX_USER1]=strdup("-A -C -G -T -U");
	char_bind[HELIX_USER1] = strdup("#");

	pairs[HELIX_USER2]=strdup(".. --");
	char_bind[HELIX_USER2] = strdup(" ");

	pairs[HELIX_USER3]=strdup(".-");
	char_bind[HELIX_USER3] = strdup(" ");

	pairs[HELIX_DEFAULT]=strdup("");
	char_bind[HELIX_DEFAULT] = strdup("?");

	for (i=HELIX_NON_STANDART0;i<=HELIX_NON_STANDART9;i++){
		pairs[i] = strdup("");
		char_bind[i] = strdup("");
	}

	pairs[HELIX_NO_MATCH]=strdup("");
	char_bind[HELIX_NO_MATCH] = strdup("|");

#ifdef _USE_AW_WINDOW
	int j;
	for (j=0; helix_awars[j].awar; j++){
		i = helix_awars[j].pair_type;
		sprintf(awar,HELIX_AWAR_PAIR_TEMPLATE, helix_awars[j].awar);
		aw_root->create_var(AW_STRING, awar, &this->pairs[i],AW_ROOT_DEFAULT, this->pairs[i]);
		sprintf(awar,HELIX_AWAR_SYMBOL_TEMPLATE, helix_awars[j].awar);
		aw_root->create_var(AW_STRING, awar, &this->char_bind[i],AW_ROOT_DEFAULT, this->char_bind[i]);
	}
#endif

};

BI_helix::~BI_helix(void){

#ifdef _USE_AW_WINDOW
	aw_message("Internal program error: You cannot delete BI_helix !!","CONTINUE,EXIT");
#endif
	delete entries;
}

long BI_helix_check_error(char *key, long val)
	{
	struct helix_stack *stack = (struct helix_stack *)val;
	if (BI_helix::error[0]) return val;
	if (stack) {
		sprintf(BI_helix::error,"Too many '%c' in Helix '%s' pos %i",
		stack->c, key, stack->pos);
	}
	return val;
}


long BI_helix_free_hash(char *key, long val)
	{
	key = key;
	struct helix_stack *stack = (struct helix_stack *)val;
	struct helix_stack *next;
	for ( ; stack; stack = next) {
		next = stack->next;
		delete stack;
	}
	return 0;
}



char *BI_helix::init(char *helix_nr, char *helix, long size)
	/* helix_nr	string of helix identifiers
	helix		helix
	size		alignment len */
{

	long hash = GBS_create_hash(256);
	long pos;
	char c;
	char ident[256];
	char *sident;
	struct helix_stack *laststack = 0,*stack;
	if (strlen(helix) <size ) {
		sprintf(error,"Your helix is too short !!");
		helix_nr = 0;
		goto helix_end;
	}
	this->size = size;
	error[0] = 0;
	if (helix_nr) helix_nr = strdup(helix_nr);
	strcpy(ident,"0");
	this->entries = (struct BI_helix_entry *)calloc
		(sizeof(struct BI_helix_entry),(size_t)size);
	sident = "-";
	for (pos = 0; pos < size; pos ++ ) {
		if (helix_nr) {
			if ( isdigit(helix_nr[pos])) {
				int j;
				for (j=0;j<3 && pos+j<size;j++) {
					c = helix_nr[pos+j];
					helix_nr[pos+j] = ' ';
					if ( isalnum(c) ) {
						ident[j] = c;
						ident[j+1] = 0;
						if ( isdigit(c) ) continue;
					}
					break;
				}
			}
		}
		c = helix[pos];
		if (strchr(LEFT_HELIX,c) || strchr(LEFT_NONS,c)  ){	// push
			laststack = (struct helix_stack *)GBS_read_hash(hash,ident);
			stack = new struct helix_stack;
			stack->next = laststack;
			stack->pos = pos;
			stack->c = c;
			GBS_write_hash(hash,ident,(long)stack);
		}else if (strchr(RIGHT_HELIX,c) || strchr(RIGHT_NONS,c) ){	// pop
			stack = (struct helix_stack *)GBS_read_hash(hash,ident);
			if (!stack) {
				sprintf(error,"Too many '%c' in Helix '%s' pos %li",c,ident,pos);
				goto helix_end;
			}
			if (strchr(RIGHT_HELIX,c)) {
				entries[pos].pair_type = HELIX_PAIR;
				entries[stack->pos].pair_type = HELIX_PAIR;
			}else{
				c = tolower(c);
				if (stack->c != c) {
					sprintf(error,"Character '%c' pos %i don't mach character '%c' pos %i in Helix '%s'",stack->c,stack->pos,c,pos,ident);
					goto helix_end;
				}
				if (isalpha(c)) {
					entries[pos].pair_type =
						(BI_PAIR_TYPE)(HELIX_NON_STANDART0+c-'a');
					entries[stack->pos].pair_type =
						(BI_PAIR_TYPE)(HELIX_NON_STANDART0+c-'a');
				}else{
					entries[pos].pair_type = HELIX_NO_PAIR;
					entries[stack->pos].pair_type = HELIX_NO_PAIR;
				}
			}
			entries[pos].pair_pos = stack->pos;
			entries[stack->pos].pair_pos = pos;
			GBS_write_hash(hash,ident,(long)stack->next);
			delete stack;

			if (strcmp(sident+1,ident)) {
				sident = new char[strlen(ident)+2];
				sprintf(sident,"-%s",ident);
			}
			entries[pos].helix_nr = sident+1;
			entries[stack->pos].helix_nr = sident;
		}
	}

	GBS_hash_do_loop(hash,BI_helix_check_error);

	helix_end:;
	GBS_hash_do_loop(hash,BI_helix_free_hash);
	GBS_free_hash(hash);
	delete helix_nr;
	if (error[0]) return error;
	return 0;
}


char *BI_helix::init(GBDATA *gb_helix_nr,GBDATA *gb_helix,long size)
	{
	if (gb_helix) GB_push_transaction(gb_helix);
	char *helix_nr = 0;
	char *helix = 0;
	char *err = 0;
	if (!gb_helix) err =  "Cannot find the helix";
	else if (gb_helix_nr) helix_nr = GB_read_string(gb_helix_nr);
	if (!err) {
		helix = GB_read_string(gb_helix);
		err = init(helix_nr,helix,size);
	}
	delete helix_nr;
	delete helix;
	if (gb_helix) GB_pop_transaction(gb_helix);
	return err;
}

char *BI_helix::init(GBDATA *gb_main, char *alignment_name, char *helix_nr_name, char *helix_name)
	{
	char *err = 0;
	GB_push_transaction(gb_main);
	GBDATA *gb_extended_data = GB_search(gb_main,"extended_data",GB_CREATE_CONTAINER);
	long size = GBT_get_alignment_len(gb_main,alignment_name);
	if (size<=0) err = GB_get_error();
	if (!err) {
		GBDATA *gb_helix_nr_con = GBT_find_extended(gb_extended_data, helix_nr_name);
		GBDATA *gb_helix_con = GBT_find_extended(gb_extended_data, helix_name);
		GBDATA *gb_helix = 0;
		GBDATA *gb_helix_nr = 0;
		if (gb_helix_nr_con) gb_helix_nr = GBT_read_sequence(gb_helix_nr_con,alignment_name);
		if (gb_helix_con) gb_helix = GBT_read_sequence(gb_helix_con,alignment_name);
		err = init(gb_helix_nr, gb_helix, size);
	}
	GB_pop_transaction(gb_main);
	return err;
}

char *BI_helix::init(GBDATA *gb_main)
	{
	char *err = 0;
	GB_push_transaction(gb_main);
	char *helix = GBT_get_default_helix(gb_main);
	char *helix_nr = GBT_get_default_helix_nr(gb_main);
	char *use = GBT_get_default_alignment(gb_main);
	err = 	init(gb_main,use,helix_nr,helix);
	GB_pop_transaction(gb_main);
	delete helix;
	delete helix_nr;
	delete use;

	return err;

}

extern "C" {
	char *GB_give_buffer(long size);
	};

int BI_helix::_check_pair(char left, char right, BI_PAIR_TYPE pair_type)
	{
	int i;
	int len = strlen(this->pairs[pair_type])-1;
	char *pai = this->pairs[pair_type];
	for (i=0; i<len;i+=3){
		if ( pai[i] == left && pai[i+1] == right) return 1;
		if ( pai[i] == right && pai[i+1] == left) return 1;
	}
	return 0;
}

int BI_helix::check_pair(char left, char right, BI_PAIR_TYPE pair_type)
	// returns 2 helix 1 weak helix 0 no helix	-1 nothing
{
	left = toupper(left);
	right = toupper(right);
	switch(pair_type) {
		case HELIX_PAIR:
			if (	_check_pair(left,right,HELIX_STRONG_PAIR) ||
				_check_pair(left,right,HELIX_PAIR) ) return 2;
			if (	_check_pair(left,right,HELIX_WEAK_PAIR) ) return 1;
			return 0;
		case HELIX_NO_PAIR:
			if (	_check_pair(left,right,HELIX_STRONG_PAIR) ||
				_check_pair(left,right,HELIX_PAIR) ) return 0;
			return 1;
		default:
			return _check_pair(left,right,pair_type);
	}
}


#ifdef _USE_AW_WINDOW

char BI_helix::get_symbol(char left, char right, BI_PAIR_TYPE pair_type){
	left = toupper(left);
	right = toupper(right);
	int i;
	int erg;
	if (pair_type< HELIX_NON_STANDART0) {
		erg = *this->char_bind[HELIX_DEFAULT];
		for (i=HELIX_STRONG_PAIR; i< HELIX_NON_STANDART0; i++){
			if (_check_pair(left,right,(BI_PAIR_TYPE)i)){
				erg = *this->char_bind[i];
				break;
			}
		}
	}else{
		erg = *this->char_bind[HELIX_NO_MATCH];
		if (_check_pair(left,right,pair_type)) erg =  *this->char_bind[pair_type];
	}
	if (!erg) erg = ' ';
	return erg;
}

int BI_show_helix_on_device(AW_device *device, int gc, char *opt_string, size_t opt_string_size, long start, long size,
			AW_pos x,AW_pos y, AW_pos opt_ascent,AW_pos opt_descent,
			AW_CL cduser, AW_CL cd1, AW_CL cd2){
	AWUSE(opt_ascent);AWUSE(opt_descent);
	BI_helix *THIS = (BI_helix *)cduser;
	char *buffer = GB_give_buffer(size+1);
	register long i,j,k;
	for (k=0; k<size; k++) {
		i = k+start;
		if ( i<THIS->size && THIS->entries[i].pair_type) {
			j = THIS->entries[i].pair_pos;
			char pairing_character = '.';
			if (j < opt_string_size){
			    pairing_character = opt_string[j];
			}
			buffer[k] = THIS->get_symbol(opt_string[i],pairing_character,
				THIS->entries[i].pair_type);
		}else{
			buffer[k] = ' ';
		}
	}
	buffer[size] = 0;
	return device->text(gc,buffer,x,y,0.0,(AW_bitset)-1,cd1,cd2);
}

int BI_helix::show_helix( void *devicei, int gc1 , char *sequence,
		AW_pos x, AW_pos y,
		AW_bitset filter,
		AW_CL cd1, AW_CL cd2){

	if (!this->entries) return 0;
	AW_device *device = (AW_device *)devicei;
	return device->text_overlay( gc1, sequence, 0, x , y, 0.0 , filter, (AW_CL)this, cd1, cd2,
		1.0,1.0, BI_show_helix_on_device);
}



AW_window *create_helix_props_window(AW_root *awr, AW_cb_struct * /*owner*/awcbs){
	AW_window_simple *aws = new AW_window_simple;
	aws->init( awr, "HELIX_PROPERTIES", 100, 100, 400, 300 );

	aws->at           ( 10,10 );
	aws->auto_space(3,3);
	aws->callback     ( AW_POPDOWN );
	aws->create_button( "CLOSE", "CLOSE", "C" );
	aws->at_newline();

	aws->label_length( 18 );
	int i;
	int j;
	int ex,ey;
	char awar[256];
	for (j=0; helix_awars[j].awar; j++){

		aws->label_length( 25 );
		i = helix_awars[j].pair_type;

		if (i != HELIX_DEFAULT && i!= HELIX_NO_MATCH ) {
			sprintf(awar,HELIX_AWAR_PAIR_TEMPLATE, helix_awars[j].awar);
			aws->label(helix_awars[j].awar);
			aws->callback(awcbs);
			aws->create_input_field(awar,20);
		}else{
			aws->create_button(0,helix_awars[j].awar);
			aws->at_x(ex);
		}
		if (!j) aws->get_at_position(&ex,&ey);

		sprintf(awar,HELIX_AWAR_SYMBOL_TEMPLATE, helix_awars[j].awar);
		aws->callback(awcbs);
		aws->create_input_field(awar,3);
		aws->at_newline();
	}
	aws->window_fit();
	return (AW_window *)aws;
}
#endif
