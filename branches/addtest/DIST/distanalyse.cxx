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

using std::min;
using std::max;
using std::string;

DI_TRANSFORMATION DI_MATRIX::detect_transformation(string& msg) {
    di_assert(nentries>0);

    DI_TRANSFORMATION result = DI_TRANSFORMATION_NONE_DETECTED;
    if (is_AA) {
        if (nentries> 100) {
            msg    = "A lot of sequences!\n   ==> fast Kimura selected! (instead of PAM)";
            result = DI_TRANSFORMATION_KIMURA;
        }
        else {
            msg    = "Only limited number of sequences!\n ==> slow PAM selected! (instead of Kimura)";
            result = DI_TRANSFORMATION_PAM;
        }
    }
    else {
        long  mean_len = 0;
        float min_gc   = 9999.9;
        float max_gc   = 0.0;
        long  min_len  = 9999999;
        long  max_len  = 0;

        // calculate meanvalue of sequencelength:
        for (size_t row=0; row<nentries; row++) {
            const char *sequ = entries[row]->get_nucl_seq()->get_sequence();

            size_t flen = aliview->get_length();

            long act_gci = 0;
            long act_len = 0;

            for (size_t pos=0; pos<flen; pos++) {
                char ch = sequ[pos];
                if (ch == AP_C || ch == AP_G) act_gci++;
                if (ch == AP_A || ch == AP_C || ch == AP_G || ch == AP_T) act_len++;
            }

            mean_len += act_len;

            float act_gc = ((float) act_gci) / act_len;

            min_gc = min(min_gc, act_gc);
            max_gc = max(max_gc, act_gc);

            min_len = min(min_len, act_len);
            max_len = max(max_len, act_len);
        }

        string warn;
        if (min_len * 1.3 < max_len) {
            warn = "Warning: The length of sequences differs significantly!\n"
                "Be careful: Neighbour Joining is sensitive to this kind of \"error\"";
        }

        mean_len /= nentries;

        if (mean_len < 100) {
            msg    = "Too short sequences!\n   ==> No correction selected!";
            result = DI_TRANSFORMATION_NONE;
        }
        else if (mean_len < 300) {
            msg    = "Meanlength shorter than 300\n   ==> Jukes Cantor selected!";
            result = DI_TRANSFORMATION_JUKES_CANTOR;
        }
        else if ((mean_len < 1000) || ((max_gc / min_gc) < 1.2)) {
            const char *reason;
            if (mean_len < 1000) reason = "Sequences are too short for Olsen!";
            else                 reason = GBS_global_string("Maximal GC (%f) : Minimal GC (%f) < 1.2", max_gc, min_gc);

            msg    = GBS_global_string("%s  ==> Felsenstein selected!", reason);
            result = DI_TRANSFORMATION_FELSENSTEIN;
        }
        else {
            msg    = "Olsen selected!";
            result = DI_TRANSFORMATION_OLSEN;
        }

        if (!warn.empty()) {
            di_assert(!msg.empty());
            msg = warn+'\n'+msg;
        }
    }

    di_assert(!msg.empty());
    di_assert(result != DI_TRANSFORMATION_NONE_DETECTED);

    return result;
}

