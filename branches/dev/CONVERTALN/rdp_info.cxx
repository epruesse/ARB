// =============================================================== //
//                                                                 //
//   File      : rdp_info.cxx                                      //
//   Purpose   :                                                   //
//                                                                 //
// =============================================================== //

#include "rdp_info.h"
#include "defs.h"
#include "reader.h"

bool parse_RDP_comment(RDP_comments& comments, RDP_comment_parser one_comment_entry, const char *key, int index, Reader& reader) {
    OrgInfo& orginf = comments.orginf;
    SeqInfo& seqinf = comments.seqinf;

    if      (str_equal(key, "Source of strain:"))            one_comment_entry(orginf.source,   index, reader);
    else if (str_equal(key, "Culture collection:"))          one_comment_entry(orginf.cultcoll, index, reader);
    else if (str_equal(key, "Former name:"))                 one_comment_entry(orginf.formname, index, reader);
    else if (str_equal(key, "Alternate name:"))              one_comment_entry(orginf.nickname, index, reader);
    else if (str_equal(key, "Common name:"))                 one_comment_entry(orginf.commname, index, reader);
    else if (str_equal(key, "Host organism:"))               one_comment_entry(orginf.hostorg,  index, reader);
    else if (str_equal(key, "RDP ID:"))                      one_comment_entry(seqinf.RDPid,    index, reader);
    else if (str_equal(key, "Corresponding GenBank entry:")) one_comment_entry(seqinf.gbkentry, index, reader);
    else if (str_equal(key, "Sequencing methods:"))          one_comment_entry(seqinf.methods,  index, reader);
    else if (str_equal(key, "5' end complete:")) {
        char flag[TOKENSIZE];
        scan_token_or_die(flag, reader, index);
        if (flag[0] == 'Y')  seqinf.comp5 = 'y';
        else                 seqinf.comp5 = 'n';
        ++reader;
    }
    else if (str_equal(key, "3' end complete:")) {
        char flag[TOKENSIZE];
        scan_token_or_die(flag, reader, index);
        if (flag[0] == 'Y')  seqinf.comp3 = 'y';
        else                 seqinf.comp3 = 'n';
        ++reader;
    }
    else if (str_equal(key, "Sequence information ")) ++reader;
    else if (str_equal(key, "Organism information")) ++reader;
    else {
        return false;
    }
    return true;
}




