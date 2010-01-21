// =============================================================== //
//                                                                 //
//   File      : distanalyse.cxx                                   //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "di_matr.hxx"
#include <AP_seq_dna.hxx>
#include <AP_filter.hxx>


void DI_MATRIX::analyse() {
    long  row;

    long    act_gci, mean_gci=0;
    float   act_gc, mean_gc, min_gc=9999.9, max_gc=0.0;
    long    act_len, mean_len=0, min_len=9999999, max_len=0;

    if (is_AA) {
        if (nentries> 100) {
            aw_message("A lot of sequences!\n   ==> fast Kimura selected! (instead of PAM)");
            aw_root->awar(AWAR_DIST_CORR_TRANS)->write_int(DI_TRANSFORMATION_KIMURA);
        }
        else {
            aw_message("Only limited number of sequences!\n"
                       "   ==> slow PAM selected! (instead of Kimura)");
            aw_root->awar(AWAR_DIST_CORR_TRANS)->write_int(DI_TRANSFORMATION_PAM);
        }
    }
    else {
        // calculate meanvalue of sequencelength:
        for (row=0; row<nentries; row++) {
            act_gci = 0;
            act_len = 0;
        
            const char *sequ = entries[row]->sequence_parsimony->get_sequence();

            size_t flen = aliview->get_length();
            for (size_t pos=0; pos<flen; pos++) {
                char ch = sequ[pos];
                if (ch == AP_C || ch == AP_G) act_gci++;
                if (ch == AP_A || ch == AP_C || ch == AP_G || ch == AP_T) act_len++;
            }
            mean_gci += act_gci;
            act_gc = ((float) act_gci) / act_len;
            if (act_gc < min_gc) min_gc = act_gc;
            if (act_gc > max_gc) max_gc = act_gc;
            mean_len += act_len;
            if (act_len < min_len) min_len = act_len;
            if (act_len > max_len) max_len = act_len;
        }

        if (min_len * 1.3 < max_len) {
            aw_message("Warning: The length of sequences differs significantly!\n"
                       "        Be careful: Neighbour Joining is sensitive to\n"
                       "        this kind of \"error\"");
        }
        mean_gc = ((float) mean_gci) / mean_len / nentries;
        mean_len /= nentries;

        if (mean_len < 100) {
            aw_message("Too short sequences!\n   ==> No correction selected!");
            aw_root->awar(AWAR_DIST_CORR_TRANS)->write_int(DI_TRANSFORMATION_NONE);
        }
        else if (mean_len < 300) {
            aw_message("Meanlength shorter than 300\n   ==> Jukes Cantor selected!");
            aw_root->awar(AWAR_DIST_CORR_TRANS)->write_int(DI_TRANSFORMATION_JUKES_CANTOR);
        }
        else if ((mean_len < 1000) || ((max_gc / min_gc) < 1.2)) {
            const char *reason;
            if (mean_len < 1000) reason = "Sequences are too short for Olsen!";
            else                reason = GBS_global_string("Maximal GC (%f) : Minimal GC (%f) < 1.2", max_gc, min_gc);

            reason = GBS_global_string("%s  ==> Felsenstein selected!", reason);
            aw_message(reason);
            aw_root->awar(AWAR_DIST_CORR_TRANS)->write_int(DI_TRANSFORMATION_FELSENSTEIN);
        }
        else {
            aw_message("Olsen selected!");
            aw_root->awar(AWAR_DIST_CORR_TRANS)->write_int(DI_TRANSFORMATION_OLSEN);
        }
    }
}
