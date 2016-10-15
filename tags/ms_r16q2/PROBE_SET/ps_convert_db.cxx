// =============================================================== //
//                                                                 //
//   File      : ps_convert_db.cxx                                 //
//   Purpose   :                                                   //
//                                                                 //
//   Coded by Wolfram Foerster in October 2002                     //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "ps_tools.hxx"
#include "ps_database.hxx"
#include "ps_pg_tree_functions.hxx"
#include "ps_pg_specmap.hxx"

#include <sys/times.h>


//  GLOBALS

static PS_NodePtr __ROOT;
static int        __PROBE_LENGTH;
static SpeciesID  __MIN_ID;
static SpeciesID  __MAX_ID;

static void PS_detect_probe_length(GBDATA *_ARB_node) {
    //  recursively walk through database to first probe and get its length
    __PROBE_LENGTH = -1;

    // search for a probe
    while (__PROBE_LENGTH < 0) {
        GBDATA *ARB_group = GB_entry(_ARB_node, "group");
        if (ARB_group) {                                    // ps_node has probes
            GBDATA *probe = PG_get_first_probe(ARB_group);
            __PROBE_LENGTH = strlen(PG_read_probe(probe));
        }
        else {                                            // ps_node has no probes .. check its children
            GBDATA *ARB_child = PS_get_first_node(_ARB_node);
            while (ARB_child && (__PROBE_LENGTH < 0)) {
                PS_detect_probe_length(ARB_child);
                ARB_child = PS_get_next_node(ARB_child);
            }
        }
    }
}

PS_NodePtr PS_assert_inverse_path(const int  _max_depth, const int  _caller_ID, IDVector  *_path) {
    //  walk down the 'inverse path' creating empty nodes as necessary

    PS_NodePtr current_node = __ROOT;
    SpeciesID  current_ID;

    // handle given path
    int c = 0;
    for (IDVectorCIter i = _path->begin(); i != _path->end(); ++i, ++c) {
        current_ID   = *i;
        current_node = current_node->assertChild(current_ID);
    }

    // handle implicit path
    c = 0;
    for (current_ID = _caller_ID+1; current_ID <= _max_depth; ++current_ID, ++c) {
        current_node = current_node->assertChild(current_ID);
    }

    return current_node;
}

PS_NodePtr PS_assert_path(const int  _caller_ID, IDVector *_path) {
    //  walk down the 'path' creating empty nodes as necessary

    PS_NodePtr current_node = __ROOT;
    SpeciesID  next_path_ID;

    // handle given path
    int c = 0;
    IDVectorCIter i = _path->begin();
    next_path_ID = (i == _path->end()) ? -1 : *i;
    for (SpeciesID current_ID = __MIN_ID; current_ID <= _caller_ID; ++current_ID, ++c) {
        if (current_ID != next_path_ID) {
            current_node = current_node->assertChild(current_ID);
        }
        else {
            ++i;
            next_path_ID = (i == _path->end()) ? -1 : *i;
        }
    }

    return current_node;
}

void PS_extract_probe_data(GBDATA *_ARB_node,                // position in ARB database
                               int  _max_depth,              // count of species in ARB database
                               int  _depth,                  // current depth in tree
                         const int  _parent_ID,              // SpeciesID of parent node
                          IDVector *_inverse_path) {         // list with IDs of the 'inverse path'

    //  recursively walk through ARB-database and extract probe-data to own tree format
    //
    //  * Insertion of nodes takes place after a branch is completed (that is
    //    when ive reached a leaf in the ARB-database and im going 'back up'
    //    out of the recursion).
    //
    //  * Branches below _max_depth/2 will be moved up top by inserting nodes with
    //    'inverse' probes in the  'inverse' branch, therefore the _inverse_path
    //    list is maintained with the SpeciesIDs of the 'inverse path'.
    //    - SpeciesIDs between _parent_ID and current ID are 'missing' in the path
    //      and are appended to the _inverse_path list
    //    - SpeciesIDs greater than the current ID are implicit in the
    //      'inverse path' list and therefore not stored

    //
    // get SpeciesID
    //
    GBDATA     *data   = GB_entry(_ARB_node, "num");
    const char *buffer = GB_read_char_pntr(data);
    SpeciesID   id     = atoi(buffer);

    //
    // get probe(s)
    //
    PS_ProbeSetPtr  probes    = 0;
    GBDATA         *ARB_group = GB_entry(_ARB_node, "group"); // access probe-group
    if (ARB_group) {
        data = PG_get_first_probe(ARB_group);                         // get first probe if exists

        if (data) probes = new PS_ProbeSet;                           // new probe set if probes exist

        while (data) {
            buffer                = PG_read_probe(data);              // get probe string
            PS_ProbePtr new_probe(new PS_Probe);                      // make new probe
            new_probe->length     = __PROBE_LENGTH;                   // set probe length
            new_probe->quality    = 100;                              // set probe quality
            new_probe->GC_content = 0;                                // eval probe for GC-content
            for (int i=0; i < __PROBE_LENGTH; ++i) {
                if ((buffer[i] == 'C') || (buffer[i] == 'G')) ++(new_probe->GC_content);
            }
            probes->insert(new_probe);                                // append probe to probe set
            data = PG_get_next_probe(data);                           // get next probe
        }
    }

    //
    // enlarge inverse path
    //
    for (int i=_parent_ID+1; ((i < id) && (i >= 0)); ++i) {
        _inverse_path->push_back(i);
    }

    //
    // insertion if ARB_node had probes
    //
    if (probes) {
        if (_depth <= (_max_depth >> 1)) {
            //
            // insert if 'above' half depth
            //
            PS_NodePtr current_node = PS_assert_path(id, _inverse_path);
            current_node->addProbes(probes->begin(), probes->end());
        }
        else {
            //
            // insert if 'below' half depth
            //
            PS_NodePtr current_node = PS_assert_inverse_path(_max_depth, id, _inverse_path);
            current_node->addProbesInverted(probes->begin(), probes->end());
        }
    }

    //
    // child(ren)
    //
    GBDATA *ARB_child = PS_get_first_node(_ARB_node);                 // get first child if exists
    while (ARB_child) {
        PS_extract_probe_data(ARB_child, _max_depth, _depth+1, id, _inverse_path);
        ARB_child = PS_get_next_node(ARB_child);
    }

    //
    // shrink inverse path
    //
    while ((_inverse_path->back() > _parent_ID) && (!_inverse_path->empty())) {
        _inverse_path->pop_back();
    }
}


int main(int  _argc,
          char *_argv[]) {

    GBDATA   *gb_main = 0;
    GB_ERROR  error    = 0;

    // open probe-group-database
    if (_argc < 2) {
        printf("Missing arguments\n Usage %s <input database name>\n", _argv[0]);
        printf("output database will be named like input database but with the suffix '.wf' instead of '.arb'\n");
        exit(1);
    }

    const char *DB_name  = _argv[1];

    //
    // open and check ARB database
    //
    struct tms before;
    times(&before);
    printf("Opening probe-group-database '%s'..\n  ", DB_name);
    gb_main = GB_open(DB_name, "rwcN");
    if (!gb_main) {
        error = GB_await_error();
        GB_warning(error);
        exit(1);
    }
    printf("..loaded database (enter to continue)  ");
    PS_print_time_diff(&before);

    GB_transaction ta(gb_main);
    GBDATA *group_tree = GB_entry(gb_main, "group_tree");
    if (!group_tree) {
        printf("no 'group_tree' in database\n");
        error = GB_export_error("no 'group_tree' in database"); // @@@ error is unused
        exit(1);
    }
    GBDATA *first_level_node = PS_get_first_node(group_tree);
    if (!first_level_node) {
        printf("no 'node' found in group_tree\n");
        error = GB_export_error("no 'node' found in group_tree"); // @@@ error is unused
        exit(1);
    }

    //
    // read Name <-> ID mappings
    //
    times(&before);
    printf("init Species <-> ID - Map\n");
    PG_initSpeciesMaps(gb_main);
    int species_count = PG_NumberSpecies();
    printf("%i species in the map ", species_count);
    if (species_count >= 10) {
        printf("\nhere are the first 10 :\n");
        int count = 0;
        for (ID2NameMapCIter i=__ID2NAME_MAP.begin(); count<10; ++i, ++count) {
            printf("[ %2i ] %s\n", i->first, i->second.c_str());
        }
    }
    __MIN_ID = __ID2NAME_MAP.begin()->first;
    __MAX_ID = __ID2NAME_MAP.rbegin()->first;
    printf("IDs %i .. %i\n(enter to continue)  ", __MIN_ID, __MAX_ID);
    PS_print_time_diff(&before);

    //
    // create output database
    //
    string output_DB_name(DB_name);
    size_t suffix_pos = output_DB_name.rfind(".arb");
    if (suffix_pos != string::npos) {
        output_DB_name.erase(suffix_pos);
    }
    output_DB_name.append(".wf");
    if (suffix_pos == string::npos) {
        printf("cannot find suffix '.arb' in database name '%s'\n", DB_name);
        printf("output file will be named '%s'\n", output_DB_name.c_str());
    }
    PS_Database *ps_db = new PS_Database(output_DB_name.c_str(), PS_Database::WRITEONLY);

    //
    // copy mappings
    //
    ps_db->setMappings(__NAME2ID_MAP, __ID2NAME_MAP);

    //
    // extract data from ARB database
    //
    times(&before);
    printf("extracting probe-data...\n");
    PS_detect_probe_length(group_tree);
    printf("probe_length = %d\n", __PROBE_LENGTH);

    __ROOT           = ps_db->getRootNode();
    first_level_node = PS_get_first_node(group_tree);
    unsigned int  c  = 0;
    IDVector *inverse_path = new IDVector;
    struct tms before_first_level_node;
    for (; first_level_node; ++c) {
        if (c % 100 == 0) {
            times(&before_first_level_node);
            printf("1st level node #%u  ", c+1);
        }
        PS_extract_probe_data(first_level_node, species_count, 0, __MIN_ID-1, inverse_path);
        first_level_node = PS_get_next_node(first_level_node);
        if (c % 100 == 0) {
            PS_print_time_diff(&before_first_level_node, "this node ", "  ");
            PS_print_time_diff(&before, "total ", "\n");
        }
    }
    printf("done after %u 1st level nodes\n", c);
    printf("(enter to continue)  ");
    PS_print_time_diff(&before);

    //
    // write database to file
    //
    times(&before);
    printf("writing probe-data to %s..\n", output_DB_name.c_str());
    ps_db->save();
    printf("..done saving (enter to continue)  ");
    PS_print_time_diff(&before);

    delete inverse_path;
    delete ps_db;
    before.tms_utime = 0;
    before.tms_stime = 0;
    printf("total ");
    PS_print_time_diff(&before);
    return 0;
}
