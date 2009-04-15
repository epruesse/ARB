#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arbdb.h>
#include <arbdbt.h>
#include <aw_root.hxx>
#include <aw_device.hxx>
#include <aw_window.hxx>
#include <aw_awars.hxx>
#include <AW_rename.hxx>
#include <awt.hxx>
#include <awt_changekey.hxx>
#include <awt_sel_boxes.hxx>
#include <db_scanner.hxx>
#include "merge.hxx"
#include <inline.h>
#include <GEN.hxx>


#define AWAR1 "tmp/merge1/"
#define AWAR2 "tmp/merge2/"
#define AWAR_SPECIES1 AWAR1"name"
#define AWAR_SPECIES2 AWAR2"name"

#define AWAR_SPECIES1_DEST AWAR1"dest"
#define AWAR_SPECIES2_DEST AWAR2"dest"
#define AWAR_FIELD1 AWAR1"field"
#define AWAR_FIELD2 AWAR2"field"

#define AWAR_TAG1 AWAR1"tag"
#define AWAR_TAG2 AWAR2"tag"

#define AWAR_TAG_DEL1 AWAR1"tagdel"

#define AWAR_APPEND AWAR2"append"

static AW_CL ad_global_scannerid1 = 0;
static AW_CL ad_global_scannerid2 = 0;

const char *MG_left_AWAR_SPECIES_NAME() { return AWAR_SPECIES1; }
const char *MG_right_AWAR_SPECIES_NAME() { return AWAR_SPECIES2; }

MG_remap::MG_remap(){
    in_length = 0;
    out_length = 0;
    remap_tab = 0;
    soft_remap_tab = 0;
    compiled = 0;
}

MG_remap::~MG_remap(){
    delete [] remap_tab;
    delete [] soft_remap_tab;
}

GB_ERROR MG_remap::set(const char *in_reference, const char *out_reference){
    if (compiled){
        GB_internal_error("Cannot set remap structure after being compiled");
        return 0;
    }

    if (!remap_tab){
        in_length = strlen(in_reference);
        out_length = strlen(out_reference);
        remap_tab = new int[in_length];
        int i;
        for (i=0;i<in_length;i++){
            remap_tab[i] = -1;
        }
    }else{
        int inl = strlen (in_reference);
        if (inl > in_length){
            int *new_remap = new int[inl];
            int i;
            for (i=0;i<in_length;i++){
                new_remap[i] = remap_tab[i];
            }
            for (;i<inl;i++){
                new_remap[i] = -1;
            }
        }
        in_length = inl;
    }
    {
        int nol = strlen(out_reference);
        if (nol> out_length){
            out_length = nol;
        }
    }
    int *nremap_tab = new int[in_length];
    {
        for (int i = 0;i< in_length;i++){
            nremap_tab[i] = -1;
        }
    }
    {               // write new map to undefined positions
        const char *spacers = "-. n";
        int ipos = 0;
        int opos = 0;
        while (ipos < in_length && opos < out_length){
            while (strchr(spacers,in_reference[ipos])){
                ipos++; // search next in base
                if (ipos >= in_length ) goto end_of_new_map;
            }
            while (strchr(spacers,out_reference[opos])){
                opos++; // search next in base
                if (opos >= out_length) goto end_of_new_map;
            }
            nremap_tab[ipos] = opos;
            ipos++;
            opos++; // jump to next base
        }
    end_of_new_map:;
    }
    {               // check forward consistency of new map
        int ipos = 0;
        int opos = 0;
        for (ipos=0;ipos < in_length;ipos++){
            if (remap_tab[ipos] > opos){
                opos = remap_tab[ipos];
            }
            if (nremap_tab[ipos] >=0 && nremap_tab[ipos] < opos){ // consistency error
                nremap_tab[ipos] = -1; // sorry, not useable
            }
        }
    }
    {               // check backward consistency of new map
        int ipos;
        int opos = out_length-1;
        for (ipos = in_length-1;ipos>=0;ipos--){
            if (remap_tab[ipos] >=0 && remap_tab[ipos] < opos){
                opos = remap_tab[ipos];
            }
            if (nremap_tab[ipos] > opos){
                nremap_tab[ipos] = -1;
            }
        }
    }
    {               // merge maps
        for (int pos=0;pos<in_length;pos++){
            if (remap_tab[pos] == -1){
                if (nremap_tab[pos] >=0){
                    remap_tab[pos] = nremap_tab[pos];
                }
            }
        }
    }
    delete nremap_tab;
    return NULL;
}

GB_ERROR MG_remap::compile(){
    if (compiled){
        return 0;
    }
    compiled = 1;
    int in_pos;
    soft_remap_tab = new int[in_length];

    int last_mapped_position_source = 0;
    int last_mapped_position_dest = 0;
    for (in_pos=0;in_pos<in_length;in_pos++){
        soft_remap_tab[in_pos] = -1;
    }

    for (in_pos=0;in_pos<in_length;in_pos++){
        int new_dest;

        if ((new_dest = remap_tab[in_pos]) <0){
            continue;
        }
        {   // fill areas between
            int source;
            int dest;
            int dsource = in_pos - last_mapped_position_source;
            int ddest = new_dest - last_mapped_position_dest;

            for (source = last_mapped_position_source, dest = last_mapped_position_dest;
                 source <= last_mapped_position_source + (dsource>1) &&
                     dest <= last_mapped_position_dest + (ddest>1);
                 source++,dest++){
                soft_remap_tab[source] = dest;
            }
            for (source = in_pos, dest = new_dest;
                 source > last_mapped_position_source + (dsource>1) &&
                     dest > last_mapped_position_dest + (ddest>1);
                 source--,dest--){
                soft_remap_tab[source] = dest;
            }
        }
        last_mapped_position_source = in_pos;
        last_mapped_position_dest = new_dest;
    }
    return 0;
}

char *MG_remap::remap(const char *sequence){
    const char *gap_chars = "- .";
    int slen = strlen(sequence);
    int len = slen;
    GBS_strstruct *outs = GBS_stropen(slen- in_length + out_length + 100);

    if (in_length < len) {
        len = in_length;
    }
    int lastposset = 0;     // last position written
    int lastposread = 0;    // last position that is read and written
    int skippedgaps = 0;    // number of gaps not written
    int skippedchar = '.';  // last gap character not written

    GB_BOOL within_sequence = GB_FALSE;
    int i;
    for (i=0;i<len;i++){
        int c = sequence[i];
        if (c == '.' || c == '-'){ // dont write gaps, maybe we have to compress the alignment
            skippedchar = c;
            skippedgaps ++;
            continue;
        }
        lastposread = i;
        int new_pos = this->remap_tab[i];
        if (new_pos<0){     // no remap, try soft remap
            new_pos = soft_remap_tab[i];
        }
        if (new_pos >=0){   // if found a map than force map
            while (lastposset < new_pos){ // insert gaps
                if (within_sequence){
                    GBS_chrcat(outs,'-');
                }else{
                    GBS_chrcat(outs,'.');
                }
                lastposset ++;
            }
        }else{          // insert not written gaps
            while(skippedgaps>0){
                GBS_chrcat(outs,skippedchar);
                lastposset ++;
                skippedgaps--;
            }
        }
        skippedgaps = 0;
        if (c != '.') {
            within_sequence = GB_TRUE;
        }else{
            within_sequence = GB_FALSE;
        }
        GBS_chrcat(outs,c);
        lastposset++;
    }
    for (i = lastposread+1;i < slen;i++){   // fill overlength rest of sequence
        int c = sequence[i];
        if (strchr(gap_chars,c)) continue; // dont fill with gaps
        GBS_chrcat(outs,c);
    }
    return GBS_strclose(outs);
}

void MG_create_species_awars(AW_root *aw_root, AW_default aw_def)
{
    aw_root->awar_string( AWAR_SPECIES1, "" ,   aw_def);
    aw_root->awar_string( AWAR_SPECIES2, "" ,   aw_def);
    aw_root->awar_string( AWAR_SPECIES1_DEST, "" ,  aw_def);
    aw_root->awar_string( AWAR_SPECIES2_DEST, "" ,  aw_def);
    aw_root->awar_string( AWAR_FIELD1, "" , aw_def);
    aw_root->awar_int( AWAR_APPEND );
}


void AD_map_species1(AW_root *aw_root)
{
    GB_push_transaction(GLOBAL_gb_merge);
    char   *source     = aw_root->awar(AWAR_SPECIES1)->read_string();
    GBDATA *gb_species = GBT_find_species(GLOBAL_gb_merge,source);

    if (gb_species) awt_map_arbdb_scanner(ad_global_scannerid1,gb_species,0, CHANGE_KEY_PATH);
    GB_pop_transaction(GLOBAL_gb_merge);
    delete source;
}

void AD_map_species2(AW_root *aw_root)
{
    char *source = aw_root->awar(AWAR_SPECIES2)->read_string();
    GB_push_transaction(GLOBAL_gb_dest);
    GBDATA *gb_species = GBT_find_species(GLOBAL_gb_dest,source);
    if (gb_species) {
        awt_map_arbdb_scanner(ad_global_scannerid2,gb_species,0, CHANGE_KEY_PATH);
    }
    GB_pop_transaction(GLOBAL_gb_dest);
    delete source;
}

static GB_ERROR MG_transfer_fields_info(char *fieldname = NULL) {
    GBDATA   *gb_key_data = GB_search(GLOBAL_gb_merge, CHANGE_KEY_PATH, GB_CREATE_CONTAINER);
    GB_ERROR  error       = 0;

    if (!gb_key_data) {
        error = GB_get_error();
    }
    else {
        if (!GB_search(GLOBAL_gb_dest, CHANGE_KEY_PATH, GB_CREATE_CONTAINER)) {
            error = GB_get_error();
        }
        else {
            for (GBDATA *gb_key = GB_entry(gb_key_data,CHANGEKEY);
                 gb_key && !error;
                 gb_key = GB_nextEntry(gb_key))
            {
                GBDATA *gb_key_name = GB_entry(gb_key, CHANGEKEY_NAME);
                if (gb_key_name) {
                    GBDATA *gb_key_type = GB_entry(gb_key, CHANGEKEY_TYPE);
                    if (gb_key_type) {
                        char *name = GB_read_string(gb_key_name);

                        if (!fieldname || strcmp(fieldname,name) == 0) {
                            error = awt_add_new_changekey(GLOBAL_gb_dest,name,(int)GB_read_int(gb_key_type));
                        }
                        free(name);
                    }
                }
            }
        }
    }
    return error;
}

MG_remap *MG_create_remap(GBDATA *gb_left, GBDATA *gb_right, const char *reference_species_names, const char *alignment_name){
    char *tok;
    MG_remap *rem = new MG_remap();
    char *ref_list = strdup(reference_species_names);
    for (tok = strtok(ref_list," \n,;");tok;tok = strtok(NULL," \n,;")){
        bool    is_SAI           = strncmp(tok, "SAI:", 4) == 0;
        GBDATA *gb_species_left  = 0;
        GBDATA *gb_species_right = 0;

        if (is_SAI) {
            gb_species_left  = GBT_find_SAI(gb_left, tok+4);
            gb_species_right = GBT_find_SAI(gb_right, tok+4);
        }
        else {
            gb_species_left  = GBT_find_species(gb_left,tok);
            gb_species_right = GBT_find_species(gb_right,tok);
        }

        if (!gb_species_left || !gb_species_right) {
            aw_message(GBS_global_string("Warning: Couldn't find %s'%s' in %s DB.\nPlease read ADAPT ALIGNMENT section in help file!",
                                         is_SAI ? "" : "species ",
                                         tok,
                                         gb_species_left ? "right" : "left"));
            continue;
        }

        // look for sequence/SAI "data"
        GBDATA *gb_seq_left  = GBT_read_sequence(gb_species_left,alignment_name);
        if (!gb_seq_left) continue;
        GBDATA *gb_seq_right = GBT_read_sequence(gb_species_right,alignment_name);
        if (!gb_seq_right) continue;

        GB_TYPES type_left  = GB_read_type(gb_seq_left);
        GB_TYPES type_right = GB_read_type(gb_seq_right);

        if (type_left != type_right) {
            aw_message(GBS_global_string("Warning: data type of '%s' differs in both databases", tok));
            continue;
        }

        if (type_right == GB_STRING) {
            rem->set(GB_read_char_pntr(gb_seq_left), GB_read_char_pntr(gb_seq_right));
        }
        else {
            char *sleft  = GB_read_as_string(gb_seq_left);
            char *sright = GB_read_as_string(gb_seq_right);
            rem->set(sleft,sright);
            free(sleft);
            free(sright);
        }
    }
    rem->compile();
    delete ref_list;
    return rem;
}

MG_remaps::MG_remaps(GBDATA *gb_left,GBDATA *gb_right,AW_root *awr){
    memset((char *)this,0,sizeof(*this));
    int ref_enable = awr->awar(AWAR_REMAP_ENABLE)->read_int();
    if (!ref_enable) return;

    char *reference_species_names = awr->awar(AWAR_REMAP_SPECIES_LIST)->read_string();
    this->alignment_names = GBT_get_alignment_names(gb_left);
    for (n_remaps = 0;alignment_names[n_remaps];n_remaps++) ;
    this->remaps = (MG_remap **)GB_calloc(sizeof(MG_remap *),n_remaps);
    int i;
    for (i=0;i<n_remaps;i++){
        this->remaps[i] = MG_create_remap(gb_left,gb_right,reference_species_names,alignment_names[i]);
    }
    delete reference_species_names;
}

MG_remaps::~MG_remaps(){
    int i;
    for (i=0;i<n_remaps;i++){
        delete remaps[i];
        delete alignment_names[i];
    }
    delete alignment_names;
    delete remaps;
}

GB_ERROR MG_transfer_sequence(MG_remap *remap, GBDATA *source_species, GBDATA *destination_species,const char *alignment_name){ // align sequence after copy !!!
    if (!remap) return 0;   // no remap
    GBDATA *gb_seq_left = GBT_read_sequence(source_species,alignment_name);
    GBDATA *gb_seq_right = GBT_read_sequence(destination_species,alignment_name);
    if (!gb_seq_right || !gb_seq_left) return 0; // one has no sequence -> not copied;
    char *ls = GB_read_string(gb_seq_left);
    char *rs = GB_read_string(gb_seq_right);
    if (strcmp(ls,rs)){
        delete ls;
        delete rs;
        return 0;           // sequences differ -> not copied
    }
    delete rs;
    rs = remap->remap(ls);
    GB_ERROR error = 0;
    if (rs){
        long old_check = GBS_checksum(ls,0,".- ");
        long new_check = GBS_checksum(rs,0,".- ");
        if (old_check == new_check){
            error = GB_write_string(gb_seq_right,rs);
        }else{
            error = GB_export_error("Error in aligning sequences");
        }
    }else{
        error = GB_get_error();
    }
    delete rs;
    delete ls;
    return error;
}

GB_ERROR MG_transfer_sequence(MG_remaps *remaps, GBDATA *source_species, GBDATA *destination_species){
    int i;
    GB_ERROR error = 0;
    if (!remaps->remaps) return NULL; // not remapped
    for (i=0;i<remaps->n_remaps;i++){
        error = MG_transfer_sequence(remaps->remaps[i],source_species,destination_species,remaps->alignment_names[i]);
        if (error) break;
    }
    return error;
}


static GB_ERROR MG_transfer_one_species(AW_root *aw_root, MG_remaps& remap, 
                                        GBDATA *gb_species_data1, GBDATA *gb_species_data2,
                                        bool is_genome_db1, bool is_genome_db2, 
                                        GBDATA  *gb_species1, const char *name1,
                                        GB_HASH *source_organism_hash, GB_HASH *dest_species_hash,
                                        GB_HASH *error_suppressor)
{
    // copies one species from source to destination DB
    //
    // either 'gb_species1' or 'name1' (and 'gb_species_data1') has to be given
    // 'source_organism_hash' may be NULL, otherwise it's used to search for source organisms (when merging from genome DB)
    // 'dest_species_hash' may be NULL, otherwise created species is stored there

    GB_ERROR error = 0;
    if (gb_species1) {
        name1 = GBT_read_name(gb_species1);
    }
    else {
        mg_assert(name1);
        gb_species1 = GBT_find_species_rel_species_data(gb_species_data1, name1);
        if (!gb_species1) {
            error = GBS_global_string("Could not find species '%s'", name1);
        }
    }

    bool transfer_fields = false;
    if (is_genome_db1) {
        if (is_genome_db2) { // genome -> genome
            if (GEN_is_pseudo_gene_species(gb_species1)) {
                const char *origin        = GEN_origin_organism(gb_species1);
                GBDATA     *dest_organism = dest_species_hash
                    ? (GBDATA*)GBS_read_hash(dest_species_hash, origin)
                    : GEN_find_organism(GLOBAL_gb_dest, origin);

                if (dest_organism) transfer_fields = true;
                else {
                    error = GBS_global_string("Destination DB does not contain '%s's origin-organism '%s'",
                                              name1, origin);
                }
            }
            // else: merge organism ok
        }
        else { // genome -> non-genome
            if (GEN_is_pseudo_gene_species(gb_species1)) transfer_fields = true;
            else {
                error = GBS_global_string("You can't merge organisms (%s) into a non-genome DB.\n"
                                          "Only pseudo-species are possible", name1);
            }
        }
    }
    else {
        if (is_genome_db2) { // non-genome -> genome
            error = GBS_global_string("You can't merge non-genome species (%s) into a genome DB", name1);
        }
        // else: non-genome -> non-genome ok
    }

    GBDATA *gb_species2 = 0;
    if (!error) {
        gb_species2 = dest_species_hash
            ? (GBDATA*)GBS_read_hash(dest_species_hash, name1)
            : GBT_find_species_rel_species_data(gb_species_data2, name1);
        
        if (gb_species2) error = GB_delete(gb_species2);
    }
    if (!error) {
        gb_species2 = GB_create_container(gb_species_data2, "species");
        if (!gb_species2) error = GB_get_error();
    }
    if (!error) error = GB_copy(gb_species2, gb_species1);
    if (!error && transfer_fields) {
        mg_assert(is_genome_db1);
        error = MG_export_fields(aw_root, gb_species1, gb_species2, error_suppressor, source_organism_hash);
    }
    if (!error) GB_write_flag(gb_species2, 1);
    if (!error) error = MG_transfer_sequence(&remap, gb_species1, gb_species2);
    if (!error && dest_species_hash) GBS_write_hash(dest_species_hash, name1, (long)gb_species2);

    return error;
}



void MG_transfer_selected_species(AW_window *aww) {
    if (MG_check_alignment(aww,1)) return;

    AW_root  *aw_root = aww->get_root();
    char     *source  = aw_root->awar(AWAR_SPECIES1)->read_string();
    GB_ERROR  error   = NULL;

    if (!source || !source[0]) {
        error = "Please select a species in the left list";
    }
    else {
        aw_openstatus("Transferring selected species");

        error             = GB_begin_transaction(GLOBAL_gb_merge);
        if (!error) error = GB_begin_transaction(GLOBAL_gb_dest);

        if (!error) {
            MG_remaps rm(GLOBAL_gb_merge,GLOBAL_gb_dest,aw_root);

            GBDATA *gb_species_data1 = GB_search(GLOBAL_gb_merge,"species_data",GB_CREATE_CONTAINER);
            GBDATA *gb_species_data2 = GB_search(GLOBAL_gb_dest,"species_data",GB_CREATE_CONTAINER);

            bool is_genome_db1 = GEN_is_genome_db(GLOBAL_gb_merge, -1);
            bool is_genome_db2 = GEN_is_genome_db(GLOBAL_gb_dest, -1);

            error = MG_transfer_one_species(aw_root, rm,
                                            gb_species_data1, gb_species_data2,
                                            is_genome_db1, is_genome_db2,
                                            NULL, source,
                                            NULL, NULL,
                                            NULL);
        }

        if (!error) error = MG_transfer_fields_info();

        error = GB_end_transaction(GLOBAL_gb_merge, error);
        error = GB_end_transaction(GLOBAL_gb_dest, error);

        aw_closestatus();
    }

    if (error) aw_message(error);
}

#undef IS_QUERIED
#define IS_QUERIED(gb_species) (1 & GB_read_usr_private(gb_species))

void MG_transfer_species_list(AW_window *aww) {
    if (MG_check_alignment(aww,1)) return;

    GB_ERROR error = NULL;
    aw_openstatus("Transferring species");
    GB_begin_transaction(GLOBAL_gb_merge);
    GB_begin_transaction(GLOBAL_gb_dest);

    bool is_genome_db1 = GEN_is_genome_db(GLOBAL_gb_merge, -1);
    bool is_genome_db2 = GEN_is_genome_db(GLOBAL_gb_dest, -1);

    GB_HASH *error_suppressor     = GBS_create_hash(50, GB_IGNORE_CASE);
    GB_HASH *dest_species_hash    = GBT_create_species_hash(GLOBAL_gb_dest);
    GB_HASH *source_organism_hash = is_genome_db1 ? GBT_create_organism_hash(GLOBAL_gb_merge) : 0;

    MG_remaps rm(GLOBAL_gb_merge,GLOBAL_gb_dest,aww->get_root());

    GBDATA *gb_species1;
    int     queried = 0;
    int     count   = 0;

    for (gb_species1 = GBT_first_species(GLOBAL_gb_merge); gb_species1; gb_species1 = GBT_next_species(gb_species1)) {
        if (IS_QUERIED(gb_species1)) queried++;
    }

    for (gb_species1 = GBT_first_species(GLOBAL_gb_merge); gb_species1; gb_species1 = GBT_next_species(gb_species1)) {
        if (IS_QUERIED(gb_species1)) {
            GBDATA *gb_species_data2 = GB_search(GLOBAL_gb_dest,"species_data",GB_CREATE_CONTAINER);

            error = MG_transfer_one_species(aww->get_root(), rm,
                                            NULL, gb_species_data2,
                                            is_genome_db1, is_genome_db2,
                                            gb_species1, NULL,
                                            source_organism_hash, dest_species_hash,
                                            error_suppressor);
            aw_status(++count/double(queried));
        }
    }

    GBS_free_hash(dest_species_hash);
    if (source_organism_hash) GBS_free_hash(source_organism_hash);
    GBS_free_hash(error_suppressor);

    if (!error) error = MG_transfer_fields_info();

    error = GB_end_transaction(GLOBAL_gb_merge, error);
    GB_end_transaction_show_error(GLOBAL_gb_dest, error, aw_message);

    aw_closestatus();
}

void MG_transfer_fields_cb(AW_window *aww){
    if (MG_check_alignment(aww,1)) {
        return;
    }
    
    char     *field  = aww->get_root()->awar(AWAR_FIELD1)->read_string();
    long      append = aww->get_root()->awar(AWAR_APPEND)->read_int();
    GB_ERROR  error  = 0;

    if (field[0] == 0) {
        error = "Please select a field you want to transfer";
    }
    else if (strcmp(field,"name") == 0) {
        error = "Transferring the 'name' field is forbidden";
    }
    else {
        aw_openstatus("Transferring fields");
        GB_begin_transaction(GLOBAL_gb_merge);
        GB_begin_transaction(GLOBAL_gb_dest);

        GBDATA    *gb_dest_species_data  = GB_search(GLOBAL_gb_dest,"species_data",GB_CREATE_CONTAINER);
        GB_BOOL    transfer_of_alignment = GBS_string_matches(field,"ali_*/data",GB_MIND_CASE);
        MG_remaps  rm(GLOBAL_gb_merge,GLOBAL_gb_dest,aww->get_root());
        
        for (GBDATA *gb_species1 = GBT_first_species(GLOBAL_gb_merge);
             gb_species1 && !error;
             gb_species1 = GBT_next_species(gb_species1))
        {
            if (IS_QUERIED(gb_species1)) {
                const char *name1       = GBT_read_name(gb_species1);
                GBDATA     *gb_species2 = GB_find_string(gb_dest_species_data,"name", name1, GB_IGNORE_CASE, down_2_level);
                
                if (!gb_species2) {
                    gb_species2 = GB_create_container(gb_dest_species_data,"species");
                    if (!gb_species2) {
                        error = GB_get_error();
                    }
                    else {
                        GBDATA *gb_name2 = GB_search(gb_species2,"name",GB_STRING);
                        if (!gb_name2) {
                            error = GB_get_error();
                        }
                        else {
                            error = GB_write_string(gb_name2, name1);
                        }
                    }
                }
                else {
                    gb_species2 = GB_get_father(gb_species2);
                }

                if (!error) {
                    GBDATA *gb_field1 = GB_search(gb_species1,field,GB_FIND);
                    GBDATA *gb_field2 = GB_search(gb_species2,field,GB_FIND);
                    int     type1;
                    int     type2;
                    bool    use_copy   = true;

                    if (gb_field2 && gb_field1){
                        type1 = GB_read_type(gb_field1);
                        type2 = GB_read_type(gb_field2);
                        if ((type1==type2) && (GB_DB != type1)) {
                            if (append && type1 == GB_STRING) {
                                char *s1 = GB_read_string(gb_field1);
                                char *s2 = GB_read_string(gb_field2);

                                if (!s1 || !s2) error = GB_get_error();
                                else {
                                    if (!GBS_find_string(s2,s1,0)) {
                                        error             = GB_write_string(gb_field2, GBS_global_string("%s %s", s2, s1));
                                        if (!error) error = GB_write_flag(gb_species2,1);
                                    }
                                }

                                free(s1);
                                free(s2);
                            }
                            else { // not GB_STRING
                                error             = GB_copy(gb_field2,gb_field1);
                                if (!error) error = GB_write_flag(gb_species2, 1);
                                if (transfer_of_alignment && !error){
                                    error = MG_transfer_sequence(&rm,gb_species1,gb_species2);
                                }
                            }
                            use_copy = false;
                        }
                    }

                    if (use_copy) {
                        if (gb_field2) {
                            if (gb_field1 && !append) error = GB_delete(gb_field2);
                        }
                        if (gb_field1 && !error) {
                            type1                 = GB_read_type(gb_field1);
                            gb_field2             = GB_search(gb_species2,field,type1);
                            if (!gb_field2) error = GB_get_error();
                            else error            = GB_copy(gb_field2,gb_field1);
                        }
                    }
                }
            }
        }

        if (!error) error = MG_transfer_fields_info(field);

        error = GB_end_transaction(GLOBAL_gb_merge, error);
        error = GB_end_transaction(GLOBAL_gb_dest, error);
        aw_closestatus();
    }

    if (error) aw_message(error);
    free(field);
}

AW_window *MG_transfer_fields(AW_root *aw_root)
{
    GB_transaction dummy(GLOBAL_gb_merge);
    //  awt_selection_list_rescan(gb_merge,AWT_NDS_FILTER);

    AW_window_simple *aws = new AW_window_simple;
    aws->init( aw_root, "MERGE_TRANSFER_FIELD", "TRANSFER FIELD");
    aws->load_xfig("merge/mg_transfield.fig");
    aws->button_length(13);

    aws->callback( AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("go");
    aws->callback(MG_transfer_fields_cb);
    aws->create_button("GO","GO");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)"mg_spec_sel_field.hlp");
    aws->create_button("HELP","HELP");

    aws->at("append");
    aws->create_toggle(AWAR_APPEND);

    awt_create_selection_list_on_scandb(GLOBAL_gb_merge,
                                        (AW_window*)aws,AWAR_FIELD1,
                                        AWT_NDS_FILTER,
                                        "scandb","rescandb", &AWT_species_selector, 20, 10);

    return (AW_window*)aws;
}

void MG_move_field_cb(AW_window *aww){
    if (MG_check_alignment(aww,1)) {
        return;
    }

    AW_root  *aw_root = aww->get_root();
    char     *field   = aww->get_root()->awar(AWAR_FIELD1)->read_string();
    GB_ERROR  error   = 0;

    if (field[0] == 0) {
        error = "Please select a field to transfer";
    }
    else if (strcmp(field,"name") == 0) {
        error = "You are not allowed to transfer the 'name' field";
    }
    else {
        aw_openstatus("Cross Move field");
        error             = GB_begin_transaction(GLOBAL_gb_merge);
        if (!error) error = GB_begin_transaction(GLOBAL_gb_dest);

        if (!error) {
            GBDATA *gb_species1;
            GBDATA *gb_species2;
            {
                char *source = aw_root->awar(AWAR_SPECIES1)->read_string();
                char *dest   = aw_root->awar(AWAR_SPECIES2)->read_string();

                gb_species1 = GBT_find_species(GLOBAL_gb_merge, source);
                gb_species2 = GBT_find_species(GLOBAL_gb_dest, dest);

                free(dest);
                free(source);
            }

            if (!gb_species1) error = "Please select a species in left hitlist";
            if (!gb_species2) error = "Please select a species in right hitlist";

            if (!error) {
                GBDATA *gb_field1 = GB_search(gb_species1, field, GB_FIND);
                if (!gb_field1) error = GBS_global_string("Species 1 has no field '%s'",field);

                if (!error) {
                    int     type1     = GB_read_type(gb_field1);
                    GBDATA *gb_field2 = GB_search(gb_species2, field, GB_FIND);

                    if (gb_field2) {
                        int type2 = GB_read_type(gb_field2);

                        if ((type1==type2) && (GB_DB != type1)) {
                            error = GB_copy(gb_field2,gb_field1);
                        }
                        else { // remove dest. if type mismatch or container
                            error     = GB_delete(gb_field2);
                            gb_field2 = 0; // trigger copy
                        }
                    }

                    if (!error && !gb_field2) { // destination missing or removed 
                        gb_field2             = GB_search(gb_species2,field,type1);
                        if (!gb_field2) error = GB_get_error();
                        else error            = GB_copy(gb_field2,gb_field1);
                    }
                }
                if (!error) error = GB_write_flag(gb_species2,1);
            }
        }
        if (!error) error = MG_transfer_fields_info(field);

        error = GB_end_transaction(GLOBAL_gb_merge, error);
        error = GB_end_transaction(GLOBAL_gb_dest, error);

        aw_closestatus();
    }

    if (error) aw_message(error);
    
    free(field);
}

AW_window *create_mg_move_fields(AW_root *aw_root)
{
    GB_transaction dummy(GLOBAL_gb_merge);

    AW_window_simple *aws = new AW_window_simple;
    aws->init( aw_root, "MERGE_CROSS_MOVE_FIELD", "CROSS MOVE FIELD");
    aws->load_xfig("merge/mg_movefield.fig");
    aws->button_length(13);

    aws->callback( AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("go");
    aws->callback(MG_move_field_cb);
    aws->create_button("GO","GO");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)"movefield.hlp");
    aws->create_button("HELP","HELP");


    awt_create_selection_list_on_scandb(GLOBAL_gb_merge,
                                        (AW_window*)aws,AWAR_FIELD1,
                                        AWT_NDS_FILTER,
                                        "scandb","rescandb", &AWT_species_selector, 20, 10);

    return (AW_window*)aws;
}

void MG_merge_tagged_field_cb(AW_window *aww) {
    GB_transaction ta_merge(GLOBAL_gb_merge);
    GB_ERROR       error = GB_begin_transaction(GLOBAL_gb_dest);

    if (!error) {
        AW_root *awr      = aww->get_root();
        char    *f1       = awr->awar(AWAR_FIELD1)->read_string();
        char    *f2       = awr->awar(AWAR_FIELD2)->read_string();
        char    *tag1     = awr->awar(AWAR_TAG1)->read_string();
        char    *tag2     = awr->awar(AWAR_TAG2)->read_string();
        char    *tag_del1 = awr->awar(AWAR_TAG_DEL1)->read_string();
        
        GBDATA *gb_dest_species_data = GB_search(GLOBAL_gb_dest, "species_data", GB_CREATE_CONTAINER);

        if (!gb_dest_species_data) error = GB_get_error();
        else {
            for (GBDATA * gb_species1 = GBT_first_species(GLOBAL_gb_merge);
                 gb_species1 && !error;
                 gb_species1 = GBT_next_species(gb_species1))
            {
                if (IS_QUERIED(gb_species1)) {
                    char *name = GBT_read_string(gb_species1, "name");

                    if (!name) error = GB_get_error();
                    else {
                        GBDATA *gb_species2 = GBT_find_or_create_species_rel_species_data(gb_dest_species_data, name);

                        if (!gb_species2) error = GB_get_error();
                        else {
                            char *s1 = GBT_readOrCreate_string(gb_species1, f1, "");
                            char *s2 = GBT_readOrCreate_string(gb_species2, f2, "");

                            if (s1 && s2) {
                                char *sum = GBS_merge_tagged_strings(s1, tag1, tag_del1,
                                                                     s2, tag2, tag_del1);

                                if (!sum) error = GB_get_error();
                                else error      = GBT_write_string(gb_species2, f2, sum);

                                free(sum);
                            }
                            else error = GB_get_error();

                            free(s2);
                            free(s1);
                        }
                    }
                    free(name);
                }
            }
        }

        free(tag_del1);
        free(tag2);
        free(tag1);
        free(f2);
        free(f1);
    }
    GB_end_transaction_show_error(GLOBAL_gb_dest, error, aw_message);
}

AW_window *create_mg_merge_tagged_fields(AW_root *aw_root)
{
    static AW_window_simple *aws = 0;
    if (aws) return aws;

    GB_transaction dummy(GLOBAL_gb_merge);

    aw_root->awar_string( AWAR_FIELD1,"full_name");
    aw_root->awar_string( AWAR_FIELD2,"full_name");

    aw_root->awar_string( AWAR_TAG1,"S");
    aw_root->awar_string( AWAR_TAG2,"D");

    aw_root->awar_string( AWAR_TAG_DEL1,"S*");

    aws = new AW_window_simple;
    aws->init( aw_root, "MERGE_TAGGED_FIELDS", "MERGE TAGGED FIELDS");
    aws->load_xfig("merge/mg_mergetaggedfield.fig");
    aws->button_length(13);

    aws->callback( AW_POPDOWN);
    aws->at("close");
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("go");
    aws->callback(MG_merge_tagged_field_cb);
    aws->create_button("GO","GO");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)"mergetaggedfield.hlp");
    aws->create_button("HELP","HELP");

    aws->at("tag1");    aws->create_input_field(AWAR_TAG1,5);
    aws->at("tag2");    aws->create_input_field(AWAR_TAG2,5);

    aws->at("del1");    aws->create_input_field(AWAR_TAG_DEL1,5);

    awt_create_selection_list_on_scandb(GLOBAL_gb_merge, (AW_window*)aws,AWAR_FIELD1,AWT_NDS_FILTER,"fields1",0, &AWT_species_selector, 20, 10);
    awt_create_selection_list_on_scandb(GLOBAL_gb_dest, (AW_window*)aws,AWAR_FIELD2,AWT_NDS_FILTER,"fields2",0, &AWT_species_selector, 20, 10);

    return (AW_window*)aws;
}

GB_ERROR MG_equal_alignments(bool autoselect_equal_alignment_name) {
    /** First big job:  Make the alignment names equal */
    char **   M_alignment_names = GBT_get_alignment_names(GLOBAL_gb_merge);
    char **   D_alignment_names = GBT_get_alignment_names(GLOBAL_gb_dest);
    GB_ERROR  error             = 0;
    char     *dest              = 0;

    if (M_alignment_names[0] == 0) {
        error =  GB_export_error("No source sequences found");
    }
    else {
        char *type = GBT_get_alignment_type_string(GLOBAL_gb_merge,M_alignment_names[0]);
        int   s;
        int   d;

        for (s=0,d=0;D_alignment_names[s];s++){
            char *type2 = GBT_get_alignment_type_string(GLOBAL_gb_dest,D_alignment_names[s]);
            if (strcmp(type, type2) == 0) {
                D_alignment_names[d] = D_alignment_names[s];
                if (d != s) D_alignment_names[s] = 0;
                ++d;
            }
            else freeset(D_alignment_names[s], 0);
            free(type2);
        }

        GBS_strstruct *str;
        char          *b;
        int            aliid;

        switch (d) {
            case 0:
                error = GB_export_error("Cannot find a target alignment with a type of '%s'\n"
                                        "You should create one first or select a different alignment type\n"
                                        "during sequence import",type);
                break;
            case 1:
                dest = D_alignment_names[0];
                break;

            default:
                int i;

                if (autoselect_equal_alignment_name) {
                    for (i = 0; i<d; ++i) {
                        if (ARB_stricmp(M_alignment_names[0], D_alignment_names[i]) == 0) {
                            dest = D_alignment_names[i];
                            break;
                        }
                    }
                }

                if (!dest) {
                    str = GBS_stropen(100);

                    for (i=0;i<d;i++){
                        GBS_strcat(str,D_alignment_names[i]);
                        GBS_chrcat(str,',');
                    }
                    GBS_strcat(str,"ABORT");

                    b = GBS_strclose(str);
                    aliid = aw_question("There are more than one possible alignment targets\n"
                                        "Choose one destination alignment or ABORT",b);
                    free(b);
                    if (aliid >= d) {
                        error = "Operation Aborted";
                        break;
                    }
                    dest = D_alignment_names[aliid];
                }
                break;
        }
        if (!error && dest && strcmp(M_alignment_names[0], dest) != 0) {
            error = GBT_rename_alignment(GLOBAL_gb_merge,M_alignment_names[0], dest, 1, 1);
            if (error) {
                error = GBS_global_string("Failed to rename alignment '%s' to '%s' (Reason: %s)",
                                          M_alignment_names[0], dest, error);
            }
            else {
                awt_add_new_changekey(GLOBAL_gb_merge, GBS_global_string("%s/data",dest),GB_STRING);
            }
        }
        free(type);
    }
    GBT_free_names(M_alignment_names);
    GBT_free_names(D_alignment_names);

    return error;
}

/** Merge the sequences of two databases */
GB_ERROR MG_simple_merge(AW_root *awr) {
    static char *m_name         = 0;
    GB_ERROR     error          = 0;
    GBDATA      *M_species_data = 0;
    GBDATA      *D_species_data = 0;
    int          overwriteall   = 0;
    int          autorenameall  = 0;
    GB_HASH     *D_species_hash = 0;

    GB_push_my_security(GLOBAL_gb_merge);
    GB_push_my_security(GLOBAL_gb_dest);
    GB_begin_transaction(GLOBAL_gb_merge);
    GB_begin_transaction(GLOBAL_gb_dest);

    error = MG_equal_alignments(true);
    if (error) goto end;

    M_species_data = GB_search(GLOBAL_gb_merge,"species_data",GB_CREATE_CONTAINER);
    D_species_data = GB_search(GLOBAL_gb_dest,"species_data",GB_CREATE_CONTAINER);

    GBDATA *M_species;
    GBDATA *D_species;
    freeset(m_name, 0);

    {
        long M_species_count = GB_number_of_subentries(M_species_data);
        long D_species_count = GB_number_of_subentries(D_species_data);

        // create hash containing all species from gb_dest,
        // but sized to hold all species from both DBs: 
        D_species_hash = GBT_create_species_hash_sized(GLOBAL_gb_dest, M_species_count+D_species_count);
    }

    for (M_species = GB_entry(M_species_data,"species"); M_species; M_species = GB_nextEntry(M_species)) {
        GBDATA *M_name       = GB_search(M_species,"name",GB_STRING);
        free(m_name); m_name = GB_read_string(M_name);

        int count = 1;

    new_try:

        count++;
        
        D_species = (GBDATA*)GBS_read_hash(D_species_hash, m_name);
        if (D_species){
            if (overwriteall) {
                error = GB_delete(D_species);
            }
            else if (autorenameall) {
                GB_ERROR  dummy;
                char     *newname = AWTC_create_numbered_suffix(D_species_hash, m_name, dummy);

                mg_assert(newname);
                freeset(m_name, newname);
            }
            else {
                switch (aw_question(GBS_global_string("Warning:  There is a name conflict for species '%s'\n"
                                                      "  You may:\n"
                                                      "  - Overwrite existing species\n"
                                                      "  - Overwrite all species with name conflicts\n"
                                                      "  - Not import species\n"
                                                      "  - Rename imported species\n"
                                                      "  - Automatically rename species (append .NUM)\n"
                                                      "  - Abort everything", m_name),
                                    "overwrite, overwrite all, don't import, rename, auto-rename, abort")){
                    case 1:
                        overwriteall = 1;
                    case 0:
                        GB_delete(D_species);
                        break;
                        
                    case 2:
                        continue;

                    case 3: {
                        GB_ERROR  warning;          // duplicated species warning (does not apply here)
                        char     *autoname = AWTC_create_numbered_suffix(D_species_hash, m_name, warning);

                        if (!autoname) autoname = strdup(m_name);
                        freeset(m_name, aw_input("Rename species", "Enter new name of species", autoname));
                        free(autoname);
                        goto new_try;
                    }
                    case 4:
                        autorenameall = 1;
                        goto new_try;
                        
                    case 5:
                        error = "Operation aborted";
                        goto end;
                }
            }
        }

        if (!error) {
            D_species      = GB_create_container(D_species_data,"species");
            if (!D_species) {
                error = GB_get_error();
            }
            else {
                error             = GB_copy(D_species, M_species);
                if (!error) error = GB_write_flag(D_species,1); // mark species
                if (!error) error = GB_write_usr_private(D_species,255); // put in hitlist
                if (!error) {
                    GBDATA *D_name = GB_search(D_species,"name",GB_STRING);
                    if (!D_name) {
                        error = GB_get_error();
                    }
                    else {
                        error = GB_write_string(D_name,m_name);
                    }
                }
            }

            GBS_write_hash(D_species_hash, m_name, (long)D_species);
        }

        if (error) break;
    }

 end:

    if (D_species_hash) GBS_free_hash(D_species_hash);

    if (!error) error = MG_transfer_fields_info();
    if (!error) awr->awar(AWAR_SPECIES_NAME)->write_string(m_name);
    
    error = GB_end_transaction(GLOBAL_gb_merge, error);
    GB_end_transaction_show_error(GLOBAL_gb_dest, error, aw_message);

    GB_pop_my_security(GLOBAL_gb_merge);
    GB_pop_my_security(GLOBAL_gb_dest);
    
    return error;
}

//  ---------------------------
//      MG_species_selector
//  ---------------------------

static void mg_select_species1(GBDATA* , AW_root *aw_root, const char *item_name) {
    aw_root->awar(AWAR_SPECIES1)->write_string(item_name);
}
static void mg_select_species2(GBDATA* , AW_root *aw_root, const char *item_name) {
    aw_root->awar(AWAR_SPECIES2)->write_string(item_name);
}

static GBDATA *mg_get_first_species_data1(GBDATA *, AW_root *, AWT_QUERY_RANGE) {
    return GBT_get_species_data(GLOBAL_gb_merge);
}
static GBDATA *mg_get_first_species_data2(GBDATA *, AW_root *, AWT_QUERY_RANGE) {
    return GBT_get_species_data(GLOBAL_gb_dest);
}

static GBDATA *mg_get_selected_species1(GBDATA */*gb_main*/, AW_root *aw_root) {
    GB_transaction dummy(GLOBAL_gb_merge);
    char   *species_name            = aw_root->awar(AWAR_SPECIES1)->read_string();
    GBDATA *gb_species              = 0;
    if (species_name[0]) gb_species = GBT_find_species(GLOBAL_gb_merge, species_name);
    free(species_name);
    return gb_species;
}
static GBDATA *mg_get_selected_species2(GBDATA */*gb_main*/, AW_root *aw_root) {
    GB_transaction dummy(GLOBAL_gb_dest);
    char   *species_name            = aw_root->awar(AWAR_SPECIES2)->read_string();
    GBDATA *gb_species              = 0;
    if (species_name[0]) gb_species = GBT_find_species(GLOBAL_gb_dest, species_name);
    free(species_name);
    return gb_species;
}

static struct ad_item_selector MG_species_selector[2];

static void mg_initialize_species_selectors() {
    static int initialized = 0;
    if (!initialized) {
        MG_species_selector[0] = AWT_species_selector;
        MG_species_selector[1] = AWT_species_selector;

        for (int s = 0; s <= 1; ++s) {
            ad_item_selector& sel = MG_species_selector[s];

            sel.update_item_awars        = s ? mg_select_species2 : mg_select_species1;
            sel.get_first_item_container = s ? mg_get_first_species_data2 : mg_get_first_species_data1;
            sel.get_selected_item = s ? mg_get_selected_species2 : mg_get_selected_species1;
        }

        initialized = 1;
    }
}

AW_window *MG_merge_species_cb(AW_root *awr){
    static AW_window_simple_menu *aws = 0;
    if (aws) return (AW_window *)aws;

    awr->awar_string(AWAR_REMAP_SPECIES_LIST, "ecoli",GLOBAL_gb_dest);
    awr->awar_int(AWAR_REMAP_ENABLE, 0, GLOBAL_gb_dest);

    aws = new AW_window_simple_menu;
    aws->init( awr, "MERGE_TRANSFER_SPECIES", "TRANSFER SPECIES");
    aws->load_xfig("merge/species.fig");

    aws->at("close");aws->callback((AW_CB0)AW_POPDOWN);
    aws->create_button("CLOSE","CLOSE","C");

    aws->at("help");
    aws->callback(AW_POPUP_HELP,(AW_CL)"mg_species.hlp");
    aws->create_button("HELP","HELP","H");

    awt_query_struct awtqs;
    aws->create_menu(0,"DB_I_Expert","D");

    awtqs.gb_main                = GLOBAL_gb_merge;
    awtqs.gb_ref                 = GLOBAL_gb_dest;
    awtqs.expect_hit_in_ref_list = AW_FALSE;
    awtqs.species_name           = AWAR_SPECIES1;
    awtqs.tree_name              = 0;               // no selected tree here -> can't use tree related ACI commands without specifying a tree
    awtqs.select_bit             = 1;
    awtqs.ere_pos_fig            = "ere1";
    awtqs.by_pos_fig             = "by1";
    awtqs.qbox_pos_fig           = "qbox1";
    awtqs.rescan_pos_fig         = "rescan1";
    awtqs.key_pos_fig            = 0;
    awtqs.query_pos_fig          = "content1";
    awtqs.result_pos_fig         = "result1";
    awtqs.count_pos_fig          = "count1";
    awtqs.do_query_pos_fig       = "doquery1";
    awtqs.config_pos_fig         = "doconfig1";
    awtqs.do_mark_pos_fig        = 0;
    awtqs.do_unmark_pos_fig      = 0;
    awtqs.do_delete_pos_fig      = "dodelete1";
    awtqs.do_set_pos_fig         = "doset1";
    awtqs.do_refresh_pos_fig     = "dorefresh1";
    awtqs.open_parser_pos_fig    = "openparser1";
    awtqs.use_menu               = 1;

    mg_initialize_species_selectors();
    awtqs.selector = &(MG_species_selector[0]);

    awt_create_query_box((AW_window*)aws,&awtqs);

    AW_CL scannerid      = awt_create_arbdb_scanner(GLOBAL_gb_merge, aws, "box1",0,0,0,AWT_SCANNER,0,0,AWT_NDS_FILTER, awtqs.selector);
    ad_global_scannerid1 = scannerid;
    aws->get_root()->awar(AWAR_SPECIES1)->add_callback(AD_map_species1);

    aws->create_menu(0,"DB_II_Expert","B");

    awtqs.gb_main                = GLOBAL_gb_dest;
    awtqs.gb_ref                 = GLOBAL_gb_merge;
    awtqs.expect_hit_in_ref_list = AW_TRUE;
    awtqs.species_name           = AWAR_SPECIES2;
    awtqs.select_bit             = 1;
    awtqs.ere_pos_fig            = "ere2";
    awtqs.by_pos_fig             = "by2";
    awtqs.qbox_pos_fig           = "qbox2";
    awtqs.rescan_pos_fig         = "rescan2";
    awtqs.key_pos_fig            = 0;
    awtqs.query_pos_fig          = "content2";
    awtqs.result_pos_fig         = "result2";
    awtqs.count_pos_fig          = "count2";
    awtqs.do_query_pos_fig       = "doquery2";
    awtqs.config_pos_fig         = "doconfig2";
    awtqs.do_mark_pos_fig        = 0;
    awtqs.do_unmark_pos_fig      = 0;
    awtqs.do_delete_pos_fig      = "dodelete2";
    awtqs.do_set_pos_fig         = "doset2";
    awtqs.do_refresh_pos_fig     = "dorefresh2";
    awtqs.open_parser_pos_fig    = "openparser2";

    awtqs.selector = &(MG_species_selector[1]);

    awt_create_query_box((AW_window*)aws,&awtqs);

    scannerid            = awt_create_arbdb_scanner(GLOBAL_gb_dest, aws, "box2",0,0,0,AWT_SCANNER,0,0,AWT_NDS_FILTER, awtqs.selector);
    ad_global_scannerid2 = scannerid;
    aws->get_root()->awar(AWAR_SPECIES2)->add_callback(AD_map_species2);

    // big transfer buttons:
    aws->button_length(13);
    {
        aws->shadow_width(3);

        aws->at("transsspec");
        aws->callback(MG_transfer_selected_species);
        aws->create_button("TRANSFER_SELECTED_DELETE_DUPLICATED",
                           "TRANSFER\nSELECTED\nSPECIES\n\nDELETE\nDUPLICATE\nIN DB II","T");

        aws->at("translspec");
        aws->callback(MG_transfer_species_list);
        aws->create_button("TRANSFER_LISTED_DELETE_DUPLI",
                           "TRANSFER\nLISTED\nSPECIES\n\nDELETE\nDUPLICATES\nIN DB II","T");

        aws->at("transfield");
        aws->callback(AW_POPUP,(AW_CL)MG_transfer_fields,0);
        aws->create_button("TRANSFER_FIELD_OF_LISTED_DELETE_DUPLI",
                           "TRANSFER\nFIELD\nOF LISTED\nSPECIES\n\nDELETE\nDUPLICATES\nIN DB II","T");

        aws->shadow_width(1);
    }

    // adapt alignments
    aws->button_length(7);
    aws->at("adapt");
    aws->create_toggle(AWAR_REMAP_ENABLE);

    aws->at("reference");
    aws->create_text_field(AWAR_REMAP_SPECIES_LIST);

    aws->at("pres_sel");
    aws->callback((AW_CB1)AW_POPUP,(AW_CL)MG_select_preserves_cb);
    aws->create_button("SELECT", "SELECT", "S");

    // top icon
    aws->button_length(0);
    aws->at("icon");
    aws->callback(AW_POPUP_HELP,(AW_CL)"mg_species.hlp");
    aws->create_button("HELP_MERGE", "#merge/icon.bitmap");

    aws->create_menu(0,"DB1->DB2","-");
    aws->insert_menu_topic( "compare_field_of_listed",  "Compare a field of listed species ...","C","checkfield.hlp",   AWM_ALL, AW_POPUP,(AW_CL)create_mg_check_fields,0);
    aws->insert_menu_topic( "move_field_of_selected",   "Move one field of selected left species to same field of selected right species","M",
                            "movefield.hlp", AWM_ALL, AW_POPUP,(AW_CL)create_mg_move_fields,0);
    aws->insert_menu_topic( "merge_field_of_listed_to_new_field", "Merge field of listed species of DB1 with different fields of same species of DB2 ","D",
                            "mergetaggedfield.hlp", AWM_ALL, AW_POPUP,(AW_CL)create_mg_merge_tagged_fields,0);

    aws->insert_separator();
    aws->insert_menu_topic("def_gene_species_field_xfer", "Define field transfer for gene-species", "g", "gene_species_field_transfer.hlp",
                           AWM_ALL, AW_POPUP, (AW_CL)MG_gene_species_create_field_transfer_def_window, 0);

    return (AW_window *)aws;
}
