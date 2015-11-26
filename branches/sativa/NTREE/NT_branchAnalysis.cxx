// ============================================================== //
//                                                                //
//   File      : NT_branchAnalysis.cxx                            //
//   Purpose   :                                                  //
//                                                                //
//   Coded by Ralf Westram (coder@reallysoft.de) in August 2012   //
//   Institute of Microbiology (Technical University Munich)      //
//   http://www.arb-home.de/                                      //
//                                                                //
// ============================================================== //

#include "NT_local.h"
#include <TreeCallbacks.hxx>
#include <aw_awar.hxx>
#include <awt_canvas.hxx>
#include <aw_msg.hxx>
#include <aw_root.hxx>
#include <awt_config_manager.hxx>

#define AWAR_BRANCH_ANALYSIS     "branch_analysis"
#define AWAR_BRANCH_ANALYSIS_TMP "tmp/" AWAR_BRANCH_ANALYSIS

#define AWAR_BA_MIN_REL_DIFF AWAR_BRANCH_ANALYSIS "/min_rel_diff"
#define AWAR_BA_MIN_ABS_DIFF AWAR_BRANCH_ANALYSIS "/min_abs_diff"
#define AWAR_BA_MIN_DEPTH    AWAR_BRANCH_ANALYSIS "/min_depth"
#define AWAR_BA_MIN_ROOTDIST AWAR_BRANCH_ANALYSIS "/min_rootdist"
#define AWAR_BA_DEGENERATION AWAR_BRANCH_ANALYSIS "/degeneration"

// AISC_MKPT_PROMOTE:class AWT_canvas;

class BranchWindow : virtual Noncopyable {
    AWT_canvas       *ntw;
    char             *suffix;
    AW_awar          *awar_info;
    AW_window_simple *aws;

    static char *get_suffix(AWT_canvas *ntw) {
        // suffix depends on canvas
        return GBS_global_string_copy("_%i", NT_get_canvas_id(ntw));
    }

    const char *local_awar_name (const char *prefix, const char *name) { return GBS_global_string("%s%s/%s", prefix, suffix, name); }

    void create_awars(AW_root *aw_root);
    void create_window(AW_root *aw_root);

public:
    BranchWindow(AW_root *aw_root, AWT_canvas *ntw_)
        : ntw(ntw_),
          suffix(get_suffix(ntw))
    {
        create_awars(aw_root);
        create_window(aw_root);
    }

    ~BranchWindow() {
        free(suffix);
    }

    AW_window *get_window() const { return aws; }

private:
    AW_root *get_awroot() const { return get_window()->get_root(); }
    AWT_canvas *get_canvas() const { return ntw; }
    AP_tree *get_tree() const { return AWT_TREE(ntw)->get_root_node(); }
    GBDATA *get_gbmain() const { return get_canvas()->gb_main; }
    AW_awar *awar(const char *name) { return get_awroot()->awar(name); }

    void postmark_action() const {
        get_tree()->compute_tree();
        get_canvas()->refresh();
    }

    bool have_tree() {
        if (get_tree()) return true;
        set_info("No tree selected");
        return false;
    }

public:
    void set_info(const char *msg) const { awar_info->write_string(msg); }
    void unmark_all() const { NT_mark_all_cb(NULL, get_canvas(), 0); }

    void markDegeneratedBranches() {
        if (have_tree()) {
            GB_transaction ta(get_gbmain());
            double         degeneration_factor = awar(AWAR_BA_DEGENERATION)->read_float();

            unmark_all();
            set_info(get_tree()->mark_degenerated_branches(degeneration_factor));
            postmark_action();
        }
    }
    void markDeepLeafs() {
        if (have_tree()) {
            GB_transaction ta(get_gbmain());

            int    min_depth    = awar(AWAR_BA_MIN_DEPTH)->read_int();
            double min_rootdist = awar(AWAR_BA_MIN_ROOTDIST)->read_float();

            unmark_all();
            set_info(get_tree()->mark_deep_leafs(min_depth, min_rootdist));
            postmark_action();
        }
    }
    void markLongBranches() {
        if (have_tree()) {
            GB_transaction ta(get_gbmain());

            float min_rel_diff = awar(AWAR_BA_MIN_REL_DIFF)->read_float()/100.0;
            float min_abs_diff = awar(AWAR_BA_MIN_ABS_DIFF)->read_float();

            unmark_all();
            set_info(get_tree()->mark_long_branches(min_rel_diff, min_abs_diff));
            postmark_action();
        }
    }

    void analyseDistances() {
        if (have_tree()) {
            GB_transaction ta(get_gbmain());
            set_info(get_tree()->analyse_distances());
        }
    }
};

// --------------------------------------------------------------------------------

static void mark_long_branches_cb       (AW_window*, BranchWindow *bw) { bw->markLongBranches(); }
static void mark_deep_leafs_cb          (AW_window*, BranchWindow *bw) { bw->markDeepLeafs(); }
static void mark_degenerated_branches_cb(AW_window*, BranchWindow *bw) { bw->markDegeneratedBranches(); }
static void unmark_branches_cb          (AW_window*, BranchWindow *bw) { bw->unmark_all(); }
static void distance_analysis_cb        (AW_window*, BranchWindow *bw) { bw->analyseDistances(); }

static void tree_changed_cb             (AW_root*,   BranchWindow *bw) { bw->set_info("<tree has changed>"); }

void BranchWindow::create_awars(AW_root *aw_root) {
    awar_info = aw_root->awar_string(local_awar_name(AWAR_BRANCH_ANALYSIS_TMP, "info"), "<No analysis performed yet>");
    aw_root->awar(ntw->user_awar)->add_callback(makeRootCallback(tree_changed_cb, this));

    aw_root->awar_float(AWAR_BA_MIN_REL_DIFF, 75);
    aw_root->awar_float(AWAR_BA_MIN_ABS_DIFF, 0.01);

    aw_root->awar_int(AWAR_BA_MIN_DEPTH, 0);
    aw_root->awar_float(AWAR_BA_MIN_ROOTDIST, 0.9);

    aw_root->awar_float(AWAR_BA_DEGENERATION, 30);
}

static AWT_config_mapping_def branch_analysis_config_mapping[] = {
    { AWAR_BA_MIN_REL_DIFF, "minreldiff" },
    { AWAR_BA_MIN_ABS_DIFF, "minabsdiff" },
    { AWAR_BA_MIN_DEPTH,    "mindepth" },
    { AWAR_BA_MIN_ROOTDIST, "minrootdist" },
    { AWAR_BA_DEGENERATION, "degeneration" },

    { 0, 0 }
};

void BranchWindow::create_window(AW_root *aw_root) {
    aws = new AW_window_simple;

    aws->init(aw_root, GBS_global_string("BRANCH_ANALYSIS_%s", suffix), "Branch analysis");
    aws->load_xfig("ad_branch.fig");

    aws->at("close");
    aws->callback(AW_POPDOWN);
    aws->create_button("CLOSE", "CLOSE", "C");

    aws->at("help");
    aws->callback(makeHelpCallback("branch_analysis.hlp"));
    aws->create_button("HELP", "HELP", "H");

    aws->at("sel");
    aws->create_button(0, ntw->user_awar, 0, "+");

    aws->at("info");
    aws->create_text_field(awar_info->awar_name);

    aws->button_length(28);
    
    aws->at("dist_analyse");
    aws->callback(makeWindowCallback(distance_analysis_cb, this));
    aws->create_button("ANALYSE", "Analyse distances in tree");

    aws->at("unmark");
    aws->callback(makeWindowCallback(unmark_branches_cb, this));
    aws->create_button("UNMARK", "Unmark all species");

    const int WIDTH = 10;

    aws->at("mark_long");
    aws->callback(makeWindowCallback(mark_long_branches_cb, this));
    aws->create_button("MARK_LONG", "Mark long branches");

    aws->at("min_rel"); aws->create_input_field(AWAR_BA_MIN_REL_DIFF, WIDTH);
    aws->at("min_abs"); aws->create_input_field(AWAR_BA_MIN_ABS_DIFF, WIDTH);


    aws->at("mark_deep");
    aws->callback(makeWindowCallback(mark_deep_leafs_cb, this));
    aws->create_button("MARK_DEEP", "Mark deep leafs");

    aws->at("tree_depth");   aws->create_input_field(AWAR_BA_MIN_DEPTH, WIDTH);
    aws->at("branch_depth"); aws->create_input_field(AWAR_BA_MIN_ROOTDIST, WIDTH);

    aws->at("mark_degen");
    aws->callback(makeWindowCallback(mark_degenerated_branches_cb, this));
    aws->create_button("MARK_DEGENERATED", "Mark degenerated branches");

    aws->at("degen"); aws->create_input_field(AWAR_BA_DEGENERATION, WIDTH);

    aws->at("config");
    AWT_insert_config_manager(aws, AW_ROOT_DEFAULT, "branch_analysis", branch_analysis_config_mapping);
}

AW_window *NT_create_branch_analysis_window(AW_root *aw_root, AWT_canvas *ntw) {
    static BranchWindow *bw[MAX_NT_WINDOWS] = { MAX_NT_WINDOWS_NULLINIT };

    int ntw_id = NT_get_canvas_id(ntw);
    if (!bw[ntw_id]) {
        bw[ntw_id] = new BranchWindow(aw_root, ntw);
    }
    nt_assert(bw[ntw_id]);
    return bw[ntw_id]->get_window();
}

