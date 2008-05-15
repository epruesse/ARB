#include "phylo.hxx"
#include "phwin.hxx"

#include <cstdlib>
#include <cstring>
#include <cctype>


extern char **filter_text;

PH_filter::PH_filter(void)
{
    memset ((char *)this,0,sizeof(PH_filter));
}

char *PH_filter::init(char *ifilter, char *zerobases, long size)
{
    int             i;

    delete  filter;
    filter = new char[size];
    filter_len = size;
    real_len = 0;
    for (i = 0; i < size; i++) {
        if (zerobases) {
            if (strchr(zerobases, ifilter[i])) {
                filter[i] = 0;
                real_len++;
            } else {
                filter[i] = 1;
            }
        } else {
            if (ifilter[i]) {
                filter[i] = 0;
                real_len++;
            } else {
                filter[i] = 1;
            }
        }
    }
    update = AP_timer();
    return 0;
}

char *PH_filter::init(long size)
{
    int             i;
    delete  filter;
    filter = new char[size];
    real_len = filter_len = size;
    for (i = 0; i < size; i++) {
        filter[i] = 1;
    }
    update = AP_timer();
    return 0;
}

PH_filter::~PH_filter(void)
{
    delete filter;
}

inline void strlwr(char *s) {
    for (int i = 0; s[i]; ++i) {
        s[i] = tolower(s[i]);
    }
}

float *PH_filter::calculate_column_homology(void){
    long           i,j,max,num_all_chars;
    AW_BOOL        mask[256];
    char           delete_when_max[100],get_maximum_from[100],all_chars[100],max_char;
    long           reference_table[256],**chars_counted;
    char           real_chars[100], low_chars[100];
    char           rest_chars[40], upper_rest_chars[20], low_rest_chars[20];
    unsigned char *sequence_buffer;
    AW_root       *aw_root;
    float         *mline = 0;
    double         gauge;
    if(!PHDATA::ROOT) return 0; // nothing loaded yet

    GB_transaction dummy(PHDATA::ROOT->gb_main);
    AW_BOOL isNUC = AW_TRUE;     // rna oder dna sequence : nur zum testen und Entwicklung
    if (GBT_is_alignment_protein(PHDATA::ROOT->gb_main,PHDATA::ROOT->use)){
        isNUC = AW_FALSE;
    }

    aw_root=PH_used_windows::windowList->phylo_main_window->get_root();

    if(isNUC){
        strcpy(real_chars, "ACGTU");
        strcpy(upper_rest_chars, "MRWSYKVHDBXN");
    }
    else {
        strcpy(real_chars, "ABCDEFGHIKLMNPQRSTVWYZ");
        strcpy(upper_rest_chars, "X");
    }


    strcpy(low_chars, real_chars); strlwr(low_chars); // build lower chars
    { // create low_rest_chars and rest_chars
        strcpy(low_rest_chars, upper_rest_chars);
        strlwr(low_rest_chars);
        strcpy(rest_chars, upper_rest_chars);
        strcat(rest_chars, low_rest_chars);
    }

    strcpy(all_chars, real_chars);
    strcat(all_chars, low_chars);
    strcat(all_chars, rest_chars);

    strcpy(get_maximum_from, real_chars); // get maximum from markerline from these characters
    strcat(all_chars, ".-");

    num_all_chars = strlen(all_chars);

    // initialize variables
    delete mline;
    delete options_vector;
    mline = (float *) calloc((int) PHDATA::ROOT->get_seq_len(),sizeof(float));

    // options_vector = (long *) calloc((isNUC ? 8 : 7),sizeof(long));
    options_vector    = (long *) calloc(8,sizeof(long));
    
    options_vector[OPT_START_COL]                  = aw_root->awar("phyl/filter/startcol")->read_int();
    options_vector[OPT_STOP_COL]                   = aw_root->awar("phyl/filter/stopcol")->read_int();
    options_vector[OPT_MIN_HOM]                    = aw_root->awar("phyl/filter/minhom")->read_int();
    options_vector[OPT_MAX_HOM]                    = aw_root->awar("phyl/filter/maxhom")->read_int();
    options_vector[OPT_FILTER_POINT]               = aw_root->awar("phyl/filter/point")->read_int(); // '.' in column
    options_vector[OPT_FILTER_MINUS]               = aw_root->awar("phyl/filter/minus")->read_int(); // '-' in column
    options_vector[OPT_FILTER_AMBIG]               = aw_root->awar("phyl/filter/rest")->read_int(); // 'MNY....' in column
    /*if(isNUC)*/ options_vector[OPT_FILTER_LOWER] = aw_root->awar("phyl/filter/lower")->read_int(); // 'acgtu' in column

    delete_when_max[0] = '\0';

    long startcol = options_vector[OPT_START_COL];
    long stopcol  = options_vector[OPT_STOP_COL];
    long len      = stopcol - startcol;

    // chars_counted[column][index] counts the occurrances of single characters per column
    // index = num_all_chars   -> count chars which act as column stopper ( = forget whole column if char occurs)
    // index = num_all_chars+1 -> count masked characters ( = don't count)

    chars_counted=(long **) calloc((int) (len),sizeof(long *));
    for(i=0;i<len;i++){
        chars_counted[i]=(long *) calloc((int) num_all_chars+2,sizeof(long));
        for(j=0;j<num_all_chars+2;j++) chars_counted[i][j]=0;
    }

    for(i=0;i<PHDATA::ROOT->get_seq_len();i++) mline[i]=-1.0;  // all columns invalid
    for(i=0;i<256;i++){
        mask[i]=AW_FALSE;
        reference_table[i]=num_all_chars;    // invalid and synonyme characters
    }

    // set valid characters
    for(i=0;i<num_all_chars;i++){
        mask[(unsigned char)all_chars[i]]            = AW_TRUE;
        reference_table[(unsigned char)all_chars[i]] = i;
    }

    // rna or dna sequence: set synonymes
    if(isNUC){
        reference_table[(unsigned char)'U'] = reference_table[(unsigned char)'T']; /* T=U */
        reference_table[(unsigned char)'u'] = reference_table[(unsigned char)'t'];
        reference_table[(unsigned char)'N'] = reference_table[(unsigned char)'X'];
        reference_table[(unsigned char)'n'] = reference_table[(unsigned char)'x'];
    }

    // set mappings according to options
    // be careful the elements of rest and low are mapped to 'X' and 'a'
    switch(options_vector[OPT_FILTER_POINT]){       // '.' in column
        case DONT_COUNT: 
            mask[(unsigned char)'.']=AW_FALSE;
            break;
            
        case SKIP_COLUMN_IF_MAX: 
            strcat(delete_when_max,".");
            strcat(get_maximum_from,".");
            break;
            
        case SKIP_COLUMN_IF_OCCUR: 
            reference_table[(unsigned char)'.']=num_all_chars;    // map to invalid position
            break;
            
        case COUNT_DONT_USE_MAX: // use like another valid base/acid while not maximal
            // do nothing: don't get maximum of this charcater
            // but use character ( AW_TRUE in mask )
            break;

        default : ph_assert(0); break; // illegal value!
    }

    switch(options_vector[OPT_FILTER_MINUS]){       // '-' in column
        case DONT_COUNT: 
            mask[(unsigned char)'-']=AW_FALSE;
            break;
            
        case SKIP_COLUMN_IF_MAX: 
            strcat(delete_when_max,"-");
            strcat(get_maximum_from,"-");
            break;
            
        case SKIP_COLUMN_IF_OCCUR: 
            reference_table[(unsigned char)'-']=num_all_chars;
            break;
            
        case COUNT_DONT_USE_MAX: // use like another valid base/acid while not maximal
            // do nothing: don't get maximum of this charcater
            // but use character ( AW_TRUE in mask )
            break;

        default : ph_assert(0); break; // illegal value!
    }
    // 'MNY....' in column
    bool mapRestToX = false;
    switch(options_vector[OPT_FILTER_AMBIG]) // all rest characters counted to 'X' (see below)
    {
        case DONT_COUNT:
            for(i=0; rest_chars[i]; i++) mask[(unsigned char)rest_chars[i]] = AW_FALSE;
            break;

        case SKIP_COLUMN_IF_MAX:
            strcat(delete_when_max,"X");
            strcat(get_maximum_from,"X");
            mapRestToX = true;
            break;

        case SKIP_COLUMN_IF_OCCUR:
            reference_table[(unsigned char)'X'] = num_all_chars;
            mapRestToX = true;
            break;

        case COUNT_DONT_USE_MAX: // use like another valid base/acid while not maximal
            // do nothing: don't get maximum of this charcater
            // but use character ( AW_TRUE in mask )
            break;

        case TREAT_AS_REGULAR:
            strcat(get_maximum_from, upper_rest_chars); // add uppercase rest chars to maximas
            // lowercase rest chars are handled together with normal lowercase chars (see below)
            break;

        default : ph_assert(0); break; // illegal value!
    }

    if (mapRestToX) {
        // map all rest_chars to 'X'
        for(i=0;rest_chars[i];i++){
            reference_table[(unsigned char)rest_chars[i]] = reference_table[(unsigned char)'X'];
        }
    }

    switch(options_vector[OPT_FILTER_LOWER]){   // 'acgtu' in column
        case DONT_COUNT: 
            for(i=0; low_chars[i]; i++) mask[(unsigned char)low_chars[i]] = AW_FALSE;
            break;

        case SKIP_COLUMN_IF_MAX: 
            // count all low_chars to 'a'
            for(i=0; low_chars[i]; i++) reference_table[(unsigned char)low_chars[i]] = reference_table[(unsigned char)'a'];
            strcat(delete_when_max,"a"); 
            strcat(get_maximum_from,"a");
            break;

        case SKIP_COLUMN_IF_OCCUR: 
            for(i=0; low_chars[i]; i++) reference_table[(unsigned char)low_chars[i]] = num_all_chars;
            break;

        case COUNT_DONT_USE_MAX:  // use like another valid base/acid while not maximal
            // do nothing: don't get maximum of this charcater
            // but use character ( AW_TRUE in mask )
            break;

        case TREAT_AS_UPPERCASE: // use like corresponding uppercase characters
            for(i=0; low_chars[i]; i++)      reference_table[(unsigned char)low_chars[i]]      = reference_table[toupper(low_chars[i])];
            for(i=0; low_rest_chars[i]; i++) reference_table[(unsigned char)low_rest_chars[i]] = reference_table[toupper(low_rest_chars[i])];
            break;

        default : ph_assert(0); break; // illegal value!
    }

    aw_openstatus("Calculating Filter");
    aw_status("Counting");
    // counting routine
    for(i=0;i<long(PHDATA::ROOT->nentries);i++){
        gauge = (double)i/(double)PHDATA::ROOT->nentries;
        if (aw_status(gauge*gauge)) return 0;
        sequence_buffer = (unsigned char*)GB_read_char_pntr(PHDATA::ROOT->hash_elements[i]->gb_species_data_ptr);
        long send = stopcol;
        long slen = GB_read_string_count(PHDATA::ROOT->hash_elements[i]->gb_species_data_ptr);
        if (slen< send) send = slen;
        for(j=startcol;j<send;j++){
            if(mask[sequence_buffer[j]]){
                chars_counted[j-startcol][reference_table[sequence_buffer[j]]]++;
            }else{
                chars_counted[j-startcol][num_all_chars+1]++;
            }
        }
    }

    // calculate similarity
    aw_status("Calculate similarity");
    for(i=0;i<len;i++){
        if (aw_status(i/(double)len)) return 0;
        if(chars_counted[i][num_all_chars]==0)  // else: forget whole column
        {
            max=0; max_char=' ';
            for(j=0;get_maximum_from[j]!='\0';j++){
                if (max<chars_counted[i][reference_table[(unsigned char)get_maximum_from[j]]]){
                    max_char = get_maximum_from[j];
                    max      = chars_counted[i][reference_table[(unsigned char)max_char]];
                }
            }
            if((max!=0) && !strchr(delete_when_max,max_char)){
                // delete SKIP_COLUMN_IF_MAX classes for counting
                for(j=0;delete_when_max[j]!='\0';j++){
                    chars_counted[i][num_all_chars+1]+= chars_counted[i][reference_table[(unsigned char)delete_when_max[j]]];
                    chars_counted[i][reference_table[(unsigned char)delete_when_max[j]]]=0;
                }
                mline[i+startcol]= ( max/
                                     ((float) PHDATA::ROOT->nentries -
                                      (float) chars_counted[i][num_all_chars+1]))*100.0;
                // (maximum in column / number of counted positions) * 100
            } 
        } 
    } 

    for(i=0;i<len;i++){
        free(chars_counted[i]);
    }
    free(chars_counted);
    aw_closestatus();


    ///////////////////////////////
    // debugging
    ///////////////////////////////
    /*
       FILE *f_ptr;
       int used;

       f_ptr=fopen("test.mli","w");
       for(i=0;i<PHDATA::ROOT->get_seq_len();i++)
       { if (i%10 == 9)
       { putc('\n',f_ptr);
       }
       fprintf(f_ptr,"%6.2f ",mline[i]);
       }
       fprintf(f_ptr,"\n");
       for(i=0;i<PHDATA::ROOT->get_seq_len();i++)
       { if(mline[i]<0.0)
       { putc('-',f_ptr);
       }
       else
       { putc('A'+ (int)( (109.9-mline[i])*0.1 ),f_ptr);
       }
       if(i%50==49)
       { putc('\n',f_ptr);
       }
       }
       fprintf(f_ptr,"\n");

       fprintf(f_ptr,"\n");
       used=0;
       fprintf(f_ptr,"\n\n\nstatistics:\n");
       fprintf(f_ptr,"total number of columns: %d\n",PHDATA::ROOT->get_seq_len());
       for(i=0;i<PHDATA::ROOT->get_seq_len();i++)
       {  used += ((options_vector[OPT_MIN_HOM]<=mline[i])
       &&(options_vector[OPT_MAX_HOM]>=mline[i])) ? 1 : 0;
       }
       fprintf(f_ptr,"number of used columns: %d  (%3.1f %%)\n",used,((100.0/(float)PHDATA::ROOT->get_seq_len())*(float)used));
       fprintf(f_ptr,"number of unused columns: %d  (%3.1f %%)\n",(PHDATA::ROOT->get_seq_len()-used),((100.0/(float)PHDATA::ROOT->get_seq_len())*
       ((float)PHDATA::ROOT->get_seq_len()-(float)used)));


       fclose(f_ptr);
    */
    ////////////////////////////////////
    // end debugging
    ////////////////////////////////////


    char *filt=(char *)calloc((int) PHDATA::ROOT->get_seq_len()+1,sizeof(char));
    for(i=0;i<PHDATA::ROOT->get_seq_len();i++)
        filt[i]=((options_vector[OPT_MIN_HOM]<=mline[i])&&(options_vector[OPT_MAX_HOM]>=mline[i])) ? '1' : '0';
    filt[i]='\0';
    aw_root->awar("phyl/filter/filter")->write_string(filt);
    free(filt);

    return mline;

}



void create_filter_variables(AW_root *aw_root,AW_default default_file)
{
    // filter awars
    aw_root->awar_int("phyl/filter/startcol",0,default_file);
    aw_root->awar_int("phyl/filter/stopcol",99999,default_file);
    aw_root->awar_int("phyl/filter/minhom",0,default_file);
    aw_root->awar_int("phyl/filter/maxhom",100,default_file);

    aw_root->awar_int("phyl/filter/point", DONT_COUNT, default_file); // '.' in column
    aw_root->awar_int("phyl/filter/minus", DONT_COUNT, default_file); // '-' in column
    aw_root->awar_int("phyl/filter/rest",  DONT_COUNT, default_file); // 'MNY....' in column
    aw_root->awar_int("phyl/filter/lower", DONT_COUNT, default_file); // 'acgtu' in column

    // matrix awars (das gehoert in ein anderes file norbert !!!)
    aw_root->awar_int("phyl/matrix/point", DONT_COUNT, default_file); // '.' in column
    aw_root->awar_int("phyl/matrix/minus", DONT_COUNT, default_file); // '-' in column
    aw_root->awar_int("phyl/matrix/rest",  DONT_COUNT, default_file); // 'MNY....' in column
    aw_root->awar_int("phyl/matrix/lower", DONT_COUNT, default_file); // 'acgtu' in column

    aw_root->awar("phyl/filter/startcol")->add_callback((AW_RCB1)display_status,(AW_CL)aw_root);
    aw_root->awar("phyl/filter/stopcol")->add_callback((AW_RCB1)display_status,(AW_CL)aw_root);
    aw_root->awar("phyl/filter/minhom")->add_callback((AW_RCB1)display_status,(AW_CL)aw_root);
    aw_root->awar("phyl/filter/maxhom")->add_callback((AW_RCB1)display_status,(AW_CL)aw_root);

    aw_root->awar("phyl/filter/startcol")->add_callback((AW_RCB0)expose_callb);
    aw_root->awar("phyl/filter/stopcol")->add_callback((AW_RCB0)expose_callb);
    aw_root->awar("phyl/filter/minhom")->add_callback((AW_RCB0)expose_callb);
    aw_root->awar("phyl/filter/maxhom")->add_callback((AW_RCB0)expose_callb);

    aw_root->awar("phyl/filter/point")->add_callback((AW_RCB1)display_status,(AW_CL)aw_root);
    aw_root->awar("phyl/filter/minus")->add_callback((AW_RCB1)display_status,(AW_CL)aw_root);
    aw_root->awar("phyl/filter/rest")->add_callback((AW_RCB1)display_status,(AW_CL)aw_root);
    aw_root->awar("phyl/filter/lower")->add_callback((AW_RCB1)display_status,(AW_CL)aw_root);

    aw_root->awar("phyl/matrix/point")->add_callback((AW_RCB1)display_status,(AW_CL)aw_root);
    aw_root->awar("phyl/matrix/minus")->add_callback((AW_RCB1)display_status,(AW_CL)aw_root);
    aw_root->awar("phyl/matrix/rest")->add_callback((AW_RCB1)display_status,(AW_CL)aw_root);
    aw_root->awar("phyl/matrix/lower")->add_callback((AW_RCB1)display_status,(AW_CL)aw_root);

}

AW_window *create_filter_window(AW_root *aw_root)
{
    AW_window_simple *aws = new AW_window_simple;
    aws->init(aw_root,"PHYL_FILTER", "PHYL FILTER");
    aws->load_xfig("phylo/filter.fig");
    aws->button_length(10);

    aws->at("close");
    aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("startcol");
    aws->create_input_field("phyl/filter/startcol",6);

    aws->at("stopcol");
    aws->create_input_field("phyl/filter/stopcol",6);

    aws->at("minhom");
    aws->create_input_field("phyl/filter/minhom",3);

    aws->at("maxhom");
    aws->create_input_field("phyl/filter/maxhom",3);

    aws->at("point_opts");
    aws->create_option_menu("phyl/filter/point","'.'","0");  
    aws->insert_option(filter_text[DONT_COUNT],           "0", DONT_COUNT);
    aws->insert_option(filter_text[SKIP_COLUMN_IF_MAX],   "0", SKIP_COLUMN_IF_MAX);
    aws->insert_option(filter_text[SKIP_COLUMN_IF_OCCUR], "0", SKIP_COLUMN_IF_OCCUR);
    aws->insert_option(filter_text[COUNT_DONT_USE_MAX],   "0", COUNT_DONT_USE_MAX);
    aws->update_option_menu();

    aws->at("minus_opts");
    aws->create_option_menu("phyl/filter/minus","'-'","0");
    aws->insert_option(filter_text[DONT_COUNT],           "0", DONT_COUNT);
    aws->insert_option(filter_text[SKIP_COLUMN_IF_MAX],   "0", SKIP_COLUMN_IF_MAX);
    aws->insert_option(filter_text[SKIP_COLUMN_IF_OCCUR], "0", SKIP_COLUMN_IF_OCCUR);
    aws->insert_option(filter_text[COUNT_DONT_USE_MAX],   "0", COUNT_DONT_USE_MAX);
    aws->update_option_menu();

    aws->at("rest_opts");
    aws->create_option_menu("phyl/filter/rest", "ambiguity codes","0");
    aws->insert_option(filter_text[DONT_COUNT],           "0", DONT_COUNT);
    aws->insert_option(filter_text[SKIP_COLUMN_IF_MAX],   "0", SKIP_COLUMN_IF_MAX);
    aws->insert_option(filter_text[SKIP_COLUMN_IF_OCCUR], "0", SKIP_COLUMN_IF_OCCUR);
    aws->insert_option(filter_text[COUNT_DONT_USE_MAX],   "0", COUNT_DONT_USE_MAX);
    aws->insert_option(filter_text[TREAT_AS_REGULAR],     "0", TREAT_AS_REGULAR);
    aws->update_option_menu();

    aws->at("lower_opts");
    aws->create_option_menu("phyl/filter/lower","lowercase chars","0");
    aws->insert_option(filter_text[DONT_COUNT],           "0", DONT_COUNT);
    aws->insert_option(filter_text[SKIP_COLUMN_IF_MAX],   "0", SKIP_COLUMN_IF_MAX);
    aws->insert_option(filter_text[SKIP_COLUMN_IF_OCCUR], "0", SKIP_COLUMN_IF_OCCUR);
    aws->insert_option(filter_text[COUNT_DONT_USE_MAX],   "0", COUNT_DONT_USE_MAX);
    aws->insert_option(filter_text[TREAT_AS_UPPERCASE],   "0", TREAT_AS_UPPERCASE);
    aws->update_option_menu();

    return (AW_window *)aws;
}



