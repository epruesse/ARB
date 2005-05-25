#include "phylo.hxx"
#include "phwin.hxx"

#include <stdlib.h>
#include <string.h>

PHDATA::PHDATA(AW_root *awr)
{ memset((char *)this,0,sizeof(PHDATA));
 aw_root = awr;
}

char *PHDATA::unload(void)
{ struct PHENTRY *phentry;

 free(use);
 use = 0;
 for(phentry=entries;phentry;phentry=phentry->next) 
 { free(phentry->name);
 free(phentry->full_name);
 free((char *) phentry);  
 }
 entries = 0;
 nentries = 0;
 return 0;	
}

PHDATA::~PHDATA(void)
{ unload();
 delete matrix;
}


char *PHDATA::load(char *usei)
{
    GBDATA *gb_species,*gb_ali,*gb_name,*gb_full_name;
    struct PHENTRY *phentry,*hentry;

    this->use = strdup(usei);
    this->gb_main = ::gb_main;
    last_key_number=0;
  
    GB_push_transaction(gb_main);
    seq_len = GBT_get_alignment_len(gb_main,use);
    entries=NULL;
    phentry=NULL;
    nentries = 0;
    for (	gb_species = GBT_first_marked_species(gb_main);
            gb_species;
            gb_species = GBT_next_marked_species(gb_species)) {
        gb_ali = GB_find(gb_species, use, 0, down_level);
        if (!gb_ali)	continue;
        //no existing alignmnet for this species
        hentry = new PHENTRY;
        hentry->next = NULL;
        hentry->gb_species_data_ptr = GB_find(gb_ali, "data", 0, down_level);
        hentry->key = last_key_number++;
        if (hentry->gb_species_data_ptr) {
            gb_name = GB_find(gb_species, "name", 0, down_level);
            hentry->name = GB_read_string(gb_name);
            gb_full_name = GB_find(gb_species, "full_name", 0, down_level);
            if (gb_full_name)	hentry->full_name = GB_read_string(gb_full_name);
            else			hentry->full_name = NULL;
            if (!entries) {
                entries = hentry;
                entries->prev = NULL;
                phentry = entries;
            } else {
                phentry->next = hentry;
                hentry->prev = phentry;
                phentry = hentry;
            }
            nentries++;
        } else {
            delete          hentry;
        }
    }
    GB_pop_transaction(gb_main);
    hash_elements=(struct PHENTRY **) calloc(nentries,sizeof(struct PHENTRY *));
    phentry=entries;
    {
        unsigned int i;
        for (i = 0; i < nentries; i++) {
            hash_elements[i] = phentry;
            phentry = phentry->next;
        }
    }

    return 0;
}

GB_ERROR PHDATA::save(char *filename)
{
    FILE *out;

    out = fopen(filename,"w");
    if (!out) {
        return "Cannot save your File";
    }
    unsigned row,col;
    fprintf(out,"%i\n",nentries);
    for (row = 0; row<nentries;row++){
        fprintf(out,"%-13s",hash_elements[row]->name);
        for (col=0; col<=row; col++) {
            fprintf(out,"%7.4f ",matrix->get(row,col)*100.0);
        }
        fprintf(out,"\n");
    }
    fclose(out);
    return 0;
}

void PHDATA::print()
{
    unsigned row,col;
    printf("    %i\n",nentries);
    for (row = 0; row<nentries;row++){
        printf("%-10s ",hash_elements[row]->name);
        for (col=0; col<row; col++) {
            printf("%6f ",matrix->get(row,col));
        }
        printf("\n");
    }
    printf("\n");
}


GB_ERROR PHDATA::calculate_matrix(const char */*cancel*/,double /*alpha*/,PH_TRANSFORMATION /*transformation*/)
{
    if(nentries<=1) return "There are no species selected";
  
    char *filter;
    matrix = new AP_smatrix(nentries);
    long i,j,column,reference_table[256];
    long options_vector[4];
    const char *real_chars,*low_chars,*rest_chars;
    char all_chars[100],*sequence_bufferi,*sequence_bufferj;
    AW_BOOL compare[256];
    AP_FLOAT number_of_comparisons;
    double gauge;

    AW_BOOL bases_used;     // rna oder dna sequence : nur zum testen und Entwicklung

    bases_used=AW_TRUE;
    if (!PHDATA::ROOT) return "nothing loaded yet";
    aw_root=PH_used_windows::windowList->phylo_main_window->get_root();
    if (bases_used) {
        real_chars="ACGTU";
        low_chars="acgtu";
        rest_chars="MRWSYKVHDBXNmrwsykvhdbxn";
	
        strcpy(all_chars,real_chars);
        strcat(all_chars,low_chars);
        strcat(all_chars,rest_chars);
    }
    else {
        real_chars="ABCDEFGHIKLMNPQRSTVWYZ";
        low_chars=0;
        rest_chars="X";
	
        strcpy(all_chars,real_chars);
        strcat(all_chars,rest_chars);
    }
    strcat(all_chars,".-");
  
    // initialize variables

    options_vector[0]=aw_root->awar("phyl/matrix/point")->read_int();
    options_vector[1]=aw_root->awar("phyl/matrix/minus")->read_int();
    options_vector[2]=aw_root->awar("phyl/matrix/rest")->read_int();
    options_vector[3]=aw_root->awar("phyl/matrix/lower")->read_int();

  
    for(i=0;i<256;i++) compare[i]=AW_FALSE;
    for(i=0;i<long(strlen(real_chars));i++) compare[(unsigned char)real_chars[i]]=AW_TRUE;
    for(i=0;i<long(strlen(all_chars));i++) reference_table[(unsigned char)all_chars[i]]=i;
    
    // rna or dna sequence: set synonymes
    if(bases_used) {
        reference_table[(unsigned char)'U'] = reference_table[(unsigned char)'T']; /* T=U */
        reference_table[(unsigned char)'u'] = reference_table[(unsigned char)'t'];
        reference_table[(unsigned char)'N'] = reference_table[(unsigned char)'X'];
        reference_table[(unsigned char)'n'] = reference_table[(unsigned char)'x'];
    }

    distance_table = new AP_smatrix(strlen(all_chars));
    for(i=0;i<long(strlen(all_chars));i++) {
        for(j=0;j<long(strlen(all_chars));j++) {
            distance_table->set(i,j,(reference_table[i]==reference_table[j]) ? 0.0 : 1.0);
        }
    }

    if(bases_used)  /* set substitutions T = U ... */
    {
        distance_table->set(reference_table[(unsigned char)'N'],reference_table[(unsigned char)'X'],0.0);
        distance_table->set(reference_table[(unsigned char)'n'],reference_table[(unsigned char)'x'],0.0);
    }
    distance_table->set(reference_table[(unsigned char)'.'],reference_table[(unsigned char)'-'],0.0);

    filter=strdup(aw_root->awar("phyl/filter/filter")->read_string());

    // set compare-table according to options_vector
    switch(options_vector[0]) // '.' in column
    {
        case 0:  // forget pair
            // do nothing: compare stays AW_FALSE
            break;
        case 1:
            compare[(unsigned char)'.']=AW_TRUE;
            break;
    }
    switch(options_vector[1]) // '-' in column
    {
        case 0:  // forget pair
            // do nothing: compare stays AW_FALSE
            break;
        case 1:
            compare[(unsigned char)'-']=AW_TRUE;
            break;
    }
    switch(options_vector[2]) // '.' in column
    {
        case 0:                 // forget pair
            // do nothing: compare stays AW_FALSE
            break;
        case 1:
            for(i=0;i<long(strlen(rest_chars));i++) compare[(unsigned char)rest_chars[i]] = AW_TRUE;
            break;
    }
    if(bases_used) {
        switch(options_vector[1]) // '-' in column
        {
            case 0:             // forget pair
                // do nothing: compare stays AW_FALSE
                break;
            case 1:
                for(i=0;i<long(strlen(low_chars));i++) compare[(unsigned char)low_chars[i]] = AW_TRUE;
                break;
        }
    }


    // counting routine
    aw_openstatus("Calculating Matrix");
    aw_status("Calculate the matrix");
    sequence_bufferi = 0;
    sequence_bufferj = 0;
    GB_transaction dummy(PHDATA::ROOT->gb_main);

    for (i = 0; i < long(nentries); i++) {
        gauge = (double) i / (double) nentries;
        if (aw_status(gauge * gauge)) return 0;
        delete sequence_bufferi;
        sequence_bufferi = GB_read_string(hash_elements[i]->gb_species_data_ptr);
        for (j = i + 1; j < long(nentries); j++) {
            number_of_comparisons = 0.0;
            delete sequence_bufferj;
            sequence_bufferj = GB_read_string(hash_elements[j]->gb_species_data_ptr);
            for (column = 0; column < seq_len; column++) {
                if (compare[(unsigned char)sequence_bufferi[column]] &&
                    compare[(unsigned char)sequence_bufferj[column]] &&
                    filter[column]) {
                    matrix->set(i, j,
                                matrix->get(i, j) +
                                distance_table->get(reference_table[(unsigned char)sequence_bufferi[column]],
                                                    reference_table[(unsigned char)sequence_bufferj[column]]));
                    number_of_comparisons++;
                } //if
            } //for column
            if (number_of_comparisons) {
                matrix->set(i, j, (matrix->get(i, j) / number_of_comparisons));
            }
        } //for j
    } //for i
    delete sequence_bufferi;
    delete          sequence_bufferj;
    aw_closestatus();

    return 0;
}
