#include <stdio.h>
#include <stdlib.h>
/* #include <malloc.h> */
#include <string.h>
#include "aisc.h"
#include "aisc_proto.h"

const int linebufsize = 200000;
char error_buf[256];
struct global_struct *gl;

AD             *read_aisc(char ** in);

#define get_byte(io)  {gl->lastchar = *((*io)++);\
		if (gl->lastchar == '\n') gl->line_cnt++;}

char *read_aisc_file(char *path)
{
    FILE *input;
    int   data_size;
    char *buffer = 0;
    if ((input = fopen(path, "r")) == 0) {
	fprintf(stderr, " file %s not found\n", path);
    }else{
	if (fseek(input,0,2)==-1){
	    fprintf(stderr, "file %s not seekable\n",path);
	}else{
	    data_size = (int)ftell(input) + 1;
	    rewind(input);
	    buffer =  (char *)malloc(data_size+1);
	    data_size = fread(buffer,1,data_size,input);
	    buffer[data_size] = 0;
	}
	fclose(input);
    }
    return buffer;
}


void
aisc_init()
{
    int             i;
    gl = (struct global_struct *)calloc(sizeof(struct global_struct),1);
    gl->linebuf = (char *)calloc(sizeof(char),linebufsize+2);
    gl->line_cnt = 1;
    gl->error_flag = 0;
    gl->lastchar = ' ';
    gl->bufsize = linebufsize;
    gl->out = stdout;
    gl->outs[0] = stdout;
    gl->fouts[0] = strdup("stdout");
    gl->outs[1] = stdout;
    gl->fouts[1] = strdup("*");
    gl->tabstop = 8;
    do_com_push("");
    gl->fns = create_hash(HASHSIZE);
    for (i = 0; i < 256; i++) {
	if ((i == (int) ' ') ||
	    (i == (int) '\t') ||
	    (i == (int) '\n') ||
	    (i == (int) ',') ||
	    (i == (int) ';') ||
	    (i == EOSTR)
	    ) {
	    gl->b_tab[i] = 1;
	} else {
	    gl->b_tab[i] = 0;
	}
	if ( (i == (int) '\n') ||
	     (i == (int) ',') ||
	     (i == (int) ';') ||
	     (i == EOSTR)
	     ) {
	    gl->c_tab[i] = 1;
	} else {
	    gl->c_tab[i] = 0;
	}
	if ((i == (int) ' ') ||
	    (i == (int) '\t') ||
	    (i == (int) '\n') ||
	    (i == EOSTR)
	    ) {
	    gl->s2_tab[i] = 1;
	} else {
	    gl->s2_tab[i] = 0;
	}
	if ((i == (int) ' ') ||
	    (i == (int) '\t') ||
	    (i == (int) '\n')
	    ) {
	    gl->s3_tab[i] = 1;
	} else {
	    gl->s3_tab[i] = 0;
	}

	if ((i == (int) ' ') ||
	    (i == (int) '\t') ||
	    (i == EOSTR)
	    ) {
	    gl->s_tab[i] = 1;
	} else {
	    gl->s_tab[i] = 0;
	}

	gl->outtab[i] = i;
    }
    gl->outtab['n'] = '\n';
    gl->outtab['t'] = '\t';
    gl->outtab['0'] = 0;
    gl->outtab['1'] = 0;
    gl->outtab['2'] = 0;
    gl->outtab['3'] = 0;
    gl->outtab['4'] = 0;
    gl->outtab['5'] = 0;
    gl->outtab['6'] = 0;
    gl->outtab['7'] = 0;
    gl->outtab['8'] = 0;
    gl->outtab['9'] = 0;
    gl->outtab['\\'] = 0;
}
void
p_err_eof(void)
{
    fprintf(stderr, "Unexpected end of file seen in line %i file %s\n",
	    gl->line_cnt,gl->line_path);
    gl->error_flag = 1;
}
void
p_error_brih(void)
{
    fprintf(stderr, "You tried to insert a bracket in a named field: line %i  file %s\n",
	    gl->line_cnt,gl->line_path);
    gl->error_flag = 1;
}
void
p_error_nobr(void)
{
    fprintf(stderr, "{} found; missing contents: line %i  file %s\n",
	    gl->line_cnt,gl->line_path);
    gl->error_flag = 1;
}
void
p_error_nocbr(void)
{
    fprintf(stderr, "missing '}': line %i  file %s\n",
	    gl->line_cnt,gl->line_path);
    gl->error_flag = 1;
}
void
p_error_emwbr(void)
{
    fprintf(stderr, "string expected ',' found: line %i  file %s\n",
	    gl->line_cnt,gl->line_path);
    gl->error_flag = 1;
}
void
p_error_hewnoid(void)
{
    fprintf(stderr, "string expected ';' found: line %i  file %s\n",
	    gl->line_cnt,gl->line_path);
    gl->error_flag = 1;
}
void
p_error_mixhnh(void)
{
    fprintf(stderr, "you cannot use the symbol @ in this line (or it mussed be the thirst symbol in a line): line %i  file %s\n",
	    gl->line_cnt,gl->line_path);
    gl->error_flag = 1;
}
void
p_error_misscom()
{
    fprintf(stderr, "missing ';': line %i  file %s\n",
	    gl->line_cnt,gl->line_path);
    gl->error_flag = 1;
}
void
p_error_missco()
{
    fprintf(stderr, "missing ',' or ';' or 'newline': line %i  file %s\n",
	    gl->line_cnt,gl->line_path);
    gl->error_flag = 1;
}
void p_error_exp_string(void)
{
    fprintf(stderr, "Error: Expecting string: line %i  file %s\n",
	    gl->line_cnt,gl->line_path);
    gl->error_flag = 1;
}

char           *
read_aisc_string(char ** in, int *is_m)
{
    char           *cp;
    char            buf[1024];
    cp = &buf[0];
 read_spaces:
    while ((gl->lastchar != EOSTR) && (gl->s2_tab[gl->lastchar]))
	get_byte(in);	/* spaces */
    if (gl->lastchar == '#') {
	while ((gl->lastchar != EOSTR) && (gl->lastchar != '\n'))
	    get_byte(in);	/* comment */
	goto read_spaces;
    }
    if (gl->lastchar == EOSTR) {
	*is_m = 5;
	return 0;
    }
    if (gl->lastchar == '}') {
	*is_m = 2;
	return 0;
    }
    if (gl->lastchar == '{') {
	*is_m = 3;
	return 0;
    }
    if (gl->lastchar == ',') {
	*is_m = 4;
	return 0;
    }
    if (gl->lastchar == ';') {
	*is_m = 5;
	return 0;
    }
    if (gl->lastchar == '@') {
	*is_m = 1;
	get_byte(in);
	while ((gl->lastchar != EOSTR) && (gl->s_tab[gl->lastchar]))
	    get_byte(in);
	if (gl->lastchar == EOSTR) {
	    p_err_eof();
	    return 0;
	}
    } else {
	*is_m = 0;
    }
    if (gl->lastchar == BEG_STR1) {
	get_byte(in);
	if (gl->lastchar == BEG_STR2) {
	    get_byte(in);
	read_next_char:
	    while ( (gl->lastchar != EOSTR) && (gl->lastchar != END_STR1)) {
		*(cp++) = gl->lastchar;
		get_byte(in);
	    }
	    if (gl->lastchar == END_STR1) {
		get_byte(in);
		if (gl->lastchar == END_STR2 ) {
		    get_byte(in);
		}else{
		    *(cp++) = END_STR1;
		    goto read_next_char;
		}
	    }

	} else {
	    *(cp++) = BEG_STR1;
	    while ((!gl->b_tab[gl->lastchar]) && (gl->lastchar != EOSTR) ) {
		*(cp++) = gl->lastchar;
		get_byte(in);
	    }
	}
    }else{
	while ((!gl->b_tab[gl->lastchar]) && (gl->lastchar != EOSTR) ) {
	    *(cp++) = gl->lastchar;
	    get_byte(in);
	}
    }
    if (gl->lastchar == EOSTR) {
	p_err_eof();
	return 0;
    }

 read_spaces2:
    while ((gl->lastchar != EOSTR) && (gl->s_tab[gl->lastchar]))
	get_byte(in);
    if (gl->lastchar == '#') {
	while ((gl->lastchar != EOSTR) && (gl->lastchar != '\n'))
	    get_byte(in);	/* comment */
	goto read_spaces2;
    }

    if (gl->lastchar == EOSTR) {
	p_err_eof();
	return 0;
    }
    *cp = 0;
    if (!buf[0])
	return 0;	/* emtpy string */
    return (char *) strdup(buf);
}
AD *make_AD(void)
{
    return (AD *) calloc(sizeof(AD), 1);
}
HS *make_HS(void)
{
    return (HS *) calloc(sizeof(HS), 1);
}
CL *make_CL(void)
{
    return (CL *) calloc(sizeof(CL), 1);
}

#define	ITEM_MAKE	hitem = make_AD();if (item) {\
					item->next_item = hitem;\
				}else{\
					first_item = hitem;\
				}; item  = hitem; item->first_item = first_item;

AD             *
read_aisc_line(char ** in, HS ** hs)
{				/* lese bis zum ;/} */
    static int      is_m;
    char           *str;
    AD             *first_item, *item, *hitem;
    HS             *first_header, *header, *hheader;
    first_item = item = 0;
    get_byte(in);
    first_header = header = *hs;
    while (1) {
	str = read_aisc_string(in, &is_m);
	if (gl->error_flag)
	    return 0;
	switch (is_m) {
	    case 2:
		return first_item;	/* } */
	    case 3:	/* { */
		ITEM_MAKE;
		hitem = read_aisc(in);
		if (gl->error_flag)
		    return 0;
		if ((header) && (header->key)) {
		    item->key = header->key;
		}else{
		    item->key = "{";
		}
		item->sub = hitem;
		if (hitem)	hitem->father = item;
		if (gl->lastchar != '}') {
		    p_error_nocbr();
		    return 0;
		}
		get_byte(in);
		break;
	    case 4:	/* , */
		if ((header) && (header->key)) {
		} else {
		    p_error_emwbr();
		    fprintf(stderr,"There is a header line: %s\n",header->key);
		    return 0;
		};
		get_byte(in);
		break;
	    case 5:	/* ; */
		/*if (header) {
		  p_error_hewnoid();
		  return 0;
		  }
		*/
		return first_item;
	    case 1:	/* @ */
		if (header != first_header) {
		    p_error_mixhnh();
		    return 0;
		}
		first_header = header = make_HS();
		header->key = str;
		while (gl->lastchar == ',') {
		    get_byte(in);
		    str = read_aisc_string(in, &is_m);
		    if (is_m != 1) {
			p_error_mixhnh();
			return 0;
		    };
		    hheader = make_HS();
		    header->next = hheader;
		    header = hheader;
		    header->key = str;
		}
		if (gl->lastchar != ';') {
		    p_error_misscom();
		    return 0;
		};
		if (first_header->key) {
		    *hs = first_header;
		} else {
		    *hs = 0;
		}
		get_byte(in);
		return 0;
	    case 0:
		ITEM_MAKE
		    if (header && header->key) {
			item->key = header->key;
			item->val = str;
			if (!gl->c_tab[gl->lastchar]) {
			    p_error_missco();
			    return 0;
			}
		    } else {
			item->key = str;
			str = read_aisc_string(in, &is_m);
			switch (is_m) {
			    case 3:
				hitem = read_aisc(in);
				if (gl->error_flag)
				    return 0;
				if (!hitem) {
				    p_error_nobr();
				    return 0;
				}
				item->sub = hitem;
				hitem->father = item;
				if (gl->lastchar != '}') {
				    p_error_nocbr();
				    return 0;
				}
				get_byte(in);
				break;
			    case 0:
				item->val = str;
				if (!gl->c_tab[gl->lastchar]) {
				    p_error_missco();
				    return 0;
				}
				break;
			    case 4:
			    case 5:
				item->val = strdup("");
				break;
			    default:
				p_error_exp_string();
				return 0;
			}
		    };
		if (gl->lastchar == ';') {
		    get_byte(in);
		    if (header && header->next) {
			p_error_hewnoid();
			fprintf(stderr,"There is a header line: %s; %s\n",header->next->key, str);
			return 0;
		    }
		    return first_item;
		};
		get_byte(in);
		break;
	}
	if (header)
	    header = header->next;
    }

}

AD             *
read_aisc(char ** in)
{
    AD             *first, *a, *data = 0;
    HS             *header;
    first = 0;
    header = 0;
    while ((EOSTR != gl->lastchar) && (gl->lastchar != '}')) {
	a = read_aisc_line(in, &header);
	if (gl->error_flag)
	    return 0;
	if (a) {
	    if (data) {
		data->next_line = a;
	    } else {
		first = a;
	    };
	    data = a;
	    data->first_line = first;
	}
    };
    return first;
}

CL *read_prog(char ** in,char *file)
{
    char *p;
    CL *hcl,*cl,*first_cl;
    first_cl = cl = 0;
    gl->line_cnt = 0;
    while (gl->lastchar != EOSTR) {
    read_spaces3:
	while ((gl->lastchar != EOSTR) && (gl->s2_tab[gl->lastchar]))
	    get_byte(in);
	if (gl->lastchar == '#') {
	    while ((gl->lastchar != EOSTR) && (gl->lastchar != '\n'))
		get_byte(in);	/* comment */
	    goto read_spaces3;
	}
	if (gl->lastchar == EOSTR) break;
	p = (*in)-1;
	while ((gl->lastchar != EOSTR) && (gl->lastchar != '\n'))
	    get_byte(in);
	(*in)[-1] = 0;
	hcl = make_CL();
	if (!cl) first_cl = cl = hcl;
	else{
	    cl->next = hcl;
	    cl = hcl;
	}
	cl->str = (char *)strdup(p);
	cl->path = file;
	cl->linenr = gl->line_cnt;
    }
    return first_cl;
}



int main(int argc,char ** argv)
{
    char		*buf;
    CL		*co;
    int		i;
    char		abuf[20];
    int erg;
    if (argc < 2) {
	fprintf(stderr, "Specify File name\n");
	exit(-1);
    }
    aisc_init();
    for (i=0;i<argc;i++) {
	sprintf(abuf,"argv[%i]",i);
	write_hash(gl->st->hs,abuf,argv[i]);
    }
    sprintf(abuf,"%i",argc);
    write_hash(gl->st->hs,"argc",abuf);

    buf = read_aisc_file(argv[1]);
    if (!buf) exit(-1);
    gl->prg = read_prog(&buf,argv[1]);
    if (gl->prg) {
	aisc_calc_special_commands();
	co = aisc_calc_blocks(gl->prg,0,0,0);
	if (co) {
	    print_error("unexpected end of file");
	    return 1;
	}
	erg = run_prg();
        /* fprintf(stderr, "run_prg() returns %i\n", erg); */
	if (erg) {
	    aisc_remove_files();
            fflush(stdout);
	    exit (-1);
	}
    }
    return 0;
}
