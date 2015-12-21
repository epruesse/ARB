// =============================================================== //
//                                                                 //
//   File      : PARS_dtree.cxx                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "PerfMeter.h"
#include "pars_dtree.hxx"
#include "pars_main.hxx"
#include "pars_awars.h"
#include "ap_tree_nlen.hxx"
#include "ap_main.hxx"

#include <AP_seq_dna.hxx>
#include <AP_seq_protein.hxx>
#include <AP_filter.hxx>

#include <TreeCallbacks.hxx>

#include <ColumnStat.hxx>
#include <awt_sel_boxes.hxx>
#include <awt_filter.hxx>

#include <gui_aliview.hxx>

#include <aw_preset.hxx>
#include <aw_awar.hxx>
#include <aw_msg.hxx>
#include <arb_progress.h>
#include <aw_root.hxx>
#include <aw_question.hxx>

static void AWT_graphic_parsimony_root_changed(void *cd, AP_tree *old, AP_tree *newroot) {
    AWT_graphic_tree *agt = (AWT_graphic_tree*)cd;
    UNCOVERED();

    if (old == agt->displayed_root) agt->displayed_root = newroot;
}

static AliView *pars_generate_aliview(WeightedFilter *pars_weighted_filter) {
    GBDATA *gb_main = pars_weighted_filter->get_gb_main();
    char *ali_name;
    {
        GB_transaction ta(gb_main);
        ali_name = GBT_read_string(gb_main, AWAR_ALIGNMENT);
    }
    GB_ERROR  error   = NULL;
    AliView  *aliview = pars_weighted_filter->create_aliview(ali_name, error);
    if (!aliview) aw_popup_exit(error);
    free(ali_name);
    return aliview;
}

void PARS_tree_init(AWT_graphic_parsimony *agt) {
    ap_assert(agt->get_root_node());
    ap_assert(agt == ap_main->get_graphic_tree());

    GBDATA         *gb_main = ap_main->get_gb_main();
    GB_transaction  ta(gb_main);

    const char *use     = ap_main->get_aliname();
    long        ali_len = GBT_get_alignment_len(gb_main, use);
    if (ali_len <= 1) {
        aw_popup_exit("No valid alignment selected! Try again");
    }

    agt->get_tree_root()->set_root_changed_callback(AWT_graphic_parsimony_root_changed, agt);
}

QuadraticThreshold::QuadraticThreshold(KL_DYNAMIC_THRESHOLD_TYPE type, double startx, double maxy, double maxx, double maxDepth, AP_FLOAT pars_start) {
    // set a, b,  and c for quadratic equation y = ax^2 + bx + c
    switch (type) {
        default:
            ap_assert(0);
            // fall-through
        case AP_QUADRAT_START:
            c = startx;
            a = (startx - maxy) / (maxx * maxx);
            b = -2.0 * a * maxx;
            break;

        case AP_QUADRAT_MAX: // unused (experimental)
            a = - maxy / ((maxDepth -  maxx) * (maxDepth - maxx));
            b = -2.0 * a * maxx;
            c = maxy  + a * maxx * maxx;
            break;
    }
    c += pars_start;
}

void ArbParsimony::kernighan_optimize_tree(AP_tree_nlen *at, const KL_Settings& settings, const AP_FLOAT *pars_global_start, bool dumpPerf) {
    AP_tree_nlen   *oldrootleft  = get_root_node()->get_leftson();
    AP_tree_nlen   *oldrootright = get_root_node()->get_rightson();
    AP_FLOAT        pars_curr    = get_root_node()->costs();
    const AP_FLOAT  pars_org     = pars_curr;

    OptiPerfMeter performance("KL-recursion", pars_curr);

    // setup KL recursion parameters:
    KL_params KL;
    {
        KL.max_rec_depth = settings.maxdepth; ap_assert(KL.max_rec_depth>0);
        KL.inc_rec_depth = settings.incdepth;
        KL.thresFunctor  = QuadraticThreshold(settings.Dynamic.type, settings.Dynamic.start, settings.Dynamic.maxy, settings.Dynamic.maxx, KL.max_rec_depth, pars_curr);
        KL.rec_type      = KL_RECURSION_TYPE((settings.Dynamic.enabled*AP_DYNAMIK)|(settings.Static.enabled*AP_STATIC));

        for (int x = 0; x<CUSTOM_DEPTHS; ++x) {
            KL.rec_width[x] = settings.Static.depth[x];
        }
        KL.stopAtFoldedGroups = settings.whichEdges&SKIP_FOLDED_EDGES;
    }

    AP_tree_edge *startEdge     = NULL;
    AP_tree_nlen *skipNode      = NULL;
    bool          skipStartEdge = true;

    if (!at->father) {
        startEdge     = rootEdge();
        skipStartEdge = false;
    }
    else if (!at->father->father) {
        startEdge = rootEdge();
        skipNode  = startEdge->otherNode(at);
    }
    else {
        skipNode  = at->get_father();
        startEdge = at->edgeTo(skipNode);
    }
    ap_assert(startEdge);

    EdgeChain    chain(startEdge, EdgeSpec(SKIP_LEAF_EDGES|settings.whichEdges), true, skipNode, skipStartEdge);
    arb_progress progress(chain.size());

    if (pars_global_start) {
        progress.subtitle(GBS_global_string("best=%.1f (gain=%.1f)", pars_curr, *pars_global_start-pars_curr));
    }
    else {
        progress.subtitle(GBS_global_string("best=%.1f", pars_curr));
    }

    if (skipStartEdge) startEdge->set_visited(true); // avoid traversal beyond startEdge

    while (chain && !progress.aborted()) {
        AP_tree_edge *edge = *chain; ++chain;

        ap_assert(!edge->is_leaf_edge());
        ap_assert(implicated(KL.stopAtFoldedGroups, !edge->next_to_folded_group()));

        ap_main->remember();

        bool better_tree_found = edge->kl_rec(KL, 0, pars_curr);

        if (better_tree_found) {
            ap_main->accept();
            AP_FLOAT pars_new = get_root_node()->costs();
            KL.thresFunctor.change_parsimony_start(pars_new-pars_curr);
            pars_curr = pars_new;
            if (pars_global_start) {
                progress.subtitle(GBS_global_string("best=%.1f (gain=%.1f, KL=%.1f)", pars_curr, *pars_global_start-pars_curr, pars_org-pars_curr));
            }
            else {
                progress.subtitle(GBS_global_string("best=%.1f (gain=%.1f)", pars_curr, pars_org-pars_curr));
            }
        }
        else {
            ap_main->revert();
        }
        progress.inc();
    }

    if (skipStartEdge) startEdge->set_visited(false); // reallow traversal beyond startEdge

    if (dumpPerf) performance.dump(stdout, pars_curr);

    if (oldrootleft->father == oldrootright) {
        oldrootleft->set_root();
    }
    else {
        oldrootright->set_root();
    }
}



void ArbParsimony::optimize_tree(AP_tree_nlen *at, const KL_Settings& settings, arb_progress& progress) {
    AP_tree_nlen   *oldrootleft  = get_root_node()->get_leftson();
    AP_tree_nlen   *oldrootright = get_root_node()->get_rightson();
    const AP_FLOAT  org_pars     = get_root_node()->costs();
    AP_FLOAT        prev_pars    = org_pars;

    OptiPerfMeter overallPerf("global optimization", org_pars);

    progress.subtitle(GBS_global_string("best=%.1f", org_pars));

    // define available heuristics
    enum Heuristic {
        RECURSIVE_NNI,
        CUSTOM_KL,

        NO_FURTHER_HEURISTIC,
        HEURISTIC_COUNT = NO_FURTHER_HEURISTIC,
    } heuristic = RECURSIVE_NNI;

    struct H_Settings {
        const char        *name;      // name shown in OptiPerfMeter
        const KL_Settings *kl;        // ==NULL -> NNI; else KL with these settings
        bool               repeat;    // retry same heuristic when tree improved
        Heuristic          onImprove; // continue with this heuristic if improved (repeated or not)
        Heuristic          onFailure; // continue with this heuristic if NOT improved
    } heuristic_setting[HEURISTIC_COUNT] = {
        { "recursive NNIs", NULL,      true,  CUSTOM_KL,     CUSTOM_KL },
        { "KL-optimizer",   &settings, false, RECURSIVE_NNI, NO_FURTHER_HEURISTIC },
    };

    AP_FLOAT       heu_start_pars = prev_pars;
    OptiPerfMeter *heuPerf        = NULL;

#if defined(ASSERTION_USED)
    bool at_is_root = at == rootNode();
#endif

    while (heuristic != NO_FURTHER_HEURISTIC && !progress.aborted()) {
        const H_Settings& hset = heuristic_setting[heuristic];
        if (!heuPerf) {
            ap_assert(heu_start_pars == prev_pars);
            heuPerf = new OptiPerfMeter(hset.name, heu_start_pars);
        }

        AP_FLOAT this_pars = -1;
        if (hset.kl) {
            kernighan_optimize_tree(at, *hset.kl, &org_pars, false);
            this_pars = get_root_node()->costs();
        }
        else {
            this_pars = at->nn_interchange_rec(settings.whichEdges, AP_BL_NNI_ONLY);
        }
        ap_assert(this_pars>=0); // ensure this_pars was set
        ap_assert(this_pars<=prev_pars); // otherwise heuristic worsened the tree

        ap_assert(at_is_root == (at == rootNode()));

        bool      dumpOverall   = false;
        Heuristic nextHeuristic = heuristic;
        if (this_pars<prev_pars) { // found better tree
            prev_pars = this_pars;
            progress.subtitle(GBS_global_string("best=%.1f (gain=%.1f)", this_pars, org_pars-this_pars));
            if (!hset.repeat) {
                nextHeuristic = hset.onImprove;
                dumpOverall   = heuristic == CUSTOM_KL;
            }
        }
        else { // last step did not find better tree
            nextHeuristic = this_pars<heu_start_pars ? hset.onImprove : hset.onFailure;
        }

        if (nextHeuristic != heuristic) {
            heuristic      = nextHeuristic;
            heu_start_pars = this_pars;

            heuPerf->dump(stdout, this_pars);
            delete heuPerf; heuPerf = NULL;
        }
        if (dumpOverall) overallPerf.dumpCustom(stdout, this_pars, "overall (so far)");
    }

    if (oldrootleft->father == oldrootright) {
        oldrootleft->set_root();
    }
    else {
        oldrootright->set_root();
    }

    overallPerf.dump(stdout, prev_pars);
}

AWT_graphic_parsimony::AWT_graphic_parsimony(ArbParsimony& parsimony_, GBDATA *gb_main_, AD_map_viewer_cb map_viewer_cb_)
    : AWT_graphic_tree(AW_root::SINGLETON, gb_main_, map_viewer_cb_),
      parsimony(parsimony_)
{}

AP_tree_root *AWT_graphic_parsimony::create_tree_root(AliView *aliview, AP_sequence *seq_prototype, bool insert_delete_cbs) {
    return new AP_pars_root(aliview, seq_prototype, insert_delete_cbs);
}


void ArbParsimony::generate_tree(WeightedFilter *pars_weighted_filter) {
    AliView     *aliview   = pars_generate_aliview(pars_weighted_filter);
    AP_sequence *seq_templ = 0;

    GBDATA *gb_main = aliview->get_gb_main();
    {
        GB_transaction ta(gb_main);
        bool           is_aa = GBT_is_alignment_protein(gb_main, aliview->get_aliname());

        if (is_aa) seq_templ = new AP_sequence_protein(aliview);
        else seq_templ       = new AP_sequence_parsimony(aliview);
    }

    AWT_graphic_parsimony *new_tree = new AWT_graphic_parsimony(*this, aliview->get_gb_main(), PARS_map_viewer);
    new_tree->init(aliview, seq_templ, true, false);
    set_tree(new_tree);
}

AW_gc_manager AWT_graphic_parsimony::init_devices(AW_window *aww, AW_device *device, AWT_canvas* ntw) {
    AW_init_color_group_defaults("arb_pars");

    AW_gc_manager gc_manager =
        AW_manage_GC(aww,
                     ntw->get_gc_base_name(),
                     device, AWT_GC_CURSOR, AWT_GC_MAX, /* AWT_GC_CURSOR+7, */ AW_GCM_DATA_AREA,
                     makeGcChangedCallback(TREE_GC_changed_cb, ntw), // TREE_recompute_cb
                     true,      // uses color groups
                     "#AAAA55",

                     // Important note :
                     // Many gc indices are shared between ABR_NTREE and ARB_PARSIMONY
                     // e.g. the tree drawing routines use same gc's for drawing both trees
                     // (check AWT_dtree.cxx AWT_graphic_tree::init_devices)

                     "Cursor$#FFFFFF",
                     "Branch remarks$#DBE994",
                     "+-Bootstrap$#DBE994",    "-B.(limited)$white",
                     "--unused$#ff0000",
                     "Marked$#FFFF00",
                     "Some marked$#eeee88",
                     "Not marked$black",
                     "Zombies etc.$#cc5924",

                     "--unused", "--unused", // these reserve the numbers which are used for probe colors in ARB_NTREE
                                             // (this is necessary because ARB_PARS and ARB_NTREE use the same tree painting routines)
                     "--unused", "--unused", "--unused",
                     "--unused", "--unused", "--unused",
                     "--unused", "--unused", "--unused",
                     "--unused", "--unused", "--unused",

                     NULL);
    return gc_manager;
}

void AWT_graphic_parsimony::show(AW_device *device) {
    AP_tree_nlen *root_node = parsimony.get_root_node();
    AW_awar      *awar_pars = aw_root->awar(AWAR_PARSIMONY);
    AW_awar      *awar_best = aw_root->awar(AWAR_BEST_PARSIMONY);

    long parsval = root_node ? root_node->costs() : 0;
    awar_pars->write_int(parsval);

    long best_pars = awar_best->read_int();
    if (parsval < best_pars || 0==best_pars) awar_best->write_int(parsval);

    AWT_graphic_tree::show(device);
}

void AWT_graphic_parsimony::handle_command(AW_device *device, AWT_graphic_event& event) {
    ClickedTarget clicked(this, event.best_click());
    bool          recalc_branchlengths_on_structure_change = true;

    switch (event.cmd()) {
        case AWT_MODE_NNI:
        case AWT_MODE_KERNINGHAN:
        case AWT_MODE_OPTIMIZE: {
            const char *what  = NULL;
            const char *where = NULL;

            switch (event.cmd()) {
                case AWT_MODE_NNI:        what = "Recursive NNI on"; break;
                case AWT_MODE_KERNINGHAN: what = "K.L. optimize";    break;
                case AWT_MODE_OPTIMIZE:   what = "Global optimize";  break;
                default:                  break;
            }

            AP_tree_nlen *startNode  = NULL;
            bool          repeatOpti = true;

            if (event.type() == AW_Mouse_Press) {
                switch (event.button()) {
                    case AW_BUTTON_LEFT:
                        repeatOpti = false;
                        // fall-through
                    case AW_BUTTON_RIGHT:
                        startNode = DOWNCAST(AP_tree_nlen*, clicked.node());
                        where = (startNode == get_root_node()) ? "tree" : "subtree";
                        break;

                    default:
                        break;
                }
            }

            if (what && where) {
                arb_progress progress(GBS_global_string("%s %s", what, where));

                AP_FLOAT start_pars = get_root_node()->costs();
                AP_FLOAT curr_pars  = start_pars;

                KL_Settings KL(aw_root);

                do {
                    AP_FLOAT prev_pars  = curr_pars;

                    switch (event.cmd()) {
                        case AWT_MODE_NNI:
                            startNode->nn_interchange_rec(KL.whichEdges, AP_BL_NNI_ONLY);
                            break;
                        case AWT_MODE_KERNINGHAN:
                            parsimony.kernighan_optimize_tree(startNode, KL, &start_pars, true);
                            break;
                        case AWT_MODE_OPTIMIZE:
                            parsimony.optimize_tree(startNode, KL, progress);
                            repeatOpti = false;   // never loop here (optimize_tree already loops until no further improvement)
                            break;
                        default:
                            repeatOpti = false;
                            break;
                    }

                    curr_pars  = get_root_node()->costs();
                    repeatOpti = repeatOpti && curr_pars<prev_pars;
                } while (repeatOpti);

                exports.save = 1;
                ASSERT_VALID_TREE(get_root_node());
            }
            break;
        }

        default:
            recalc_branchlengths_on_structure_change = false;
            // fall-through (unlisted modes trigger branchlength calculation internally when needed)
        case AWT_MODE_MOVE:
            AWT_graphic_tree::handle_command(device, event);
            break;
    }

    if (exports.save == 1 && recalc_branchlengths_on_structure_change) {
        arb_progress progress("Recalculating branch lengths");
        rootEdge()->calc_branchlengths();
        reorder_tree(BIG_BRANCHES_TO_TOP); // beautify after recalc_branch_lengths
    }
}


// --------------------------------------------------------------------------------

#ifdef UNIT_TESTS
#include <arb_diff.h>
#include <test_unit.h>
#include "test_env.h"

template<typename SEQTYPE>
PARSIMONY_testenv<SEQTYPE>::PARSIMONY_testenv(const char *dbname, const char *aliName)
    : parsimony(),
      klSettings(NULL)
{
    common_init(dbname);
    GBDATA         *gb_main   = ap_main->get_gb_main();
    GB_transaction  ta(gb_main);
    size_t          aliLength = GBT_get_alignment_len(gb_main, aliName);

    klSettings = new KL_Settings();

    AP_filter filter(aliLength);
    if (!filter.is_invalid()) {
        AP_weights weights(&filter);
        agt->init(new AliView(gb_main, filter, weights, aliName));
    }
}

template PARSIMONY_testenv<AP_sequence_protein>::PARSIMONY_testenv(const char *dbname, const char *aliName);   // explicit instanciation (otherwise link error in unittest)
template PARSIMONY_testenv<AP_sequence_parsimony>::PARSIMONY_testenv(const char *dbname, const char *aliName); // explicit instanciation (as above, but only happens with gcc 4.6.3/NDEBUG)


void TEST_basic_tree_modifications() {
    PARSIMONY_testenv<AP_sequence_parsimony> env("TEST_trees.arb");
    TEST_EXPECT_NO_ERROR(env.load_tree("tree_test"));

    {
        AP_tree_nlen *root = env.root_node();

        // first check initial state:
        {
            AP_tree_members& root_info = root->gr;

            TEST_EXPECT_EQUAL(root_info.grouped,  0);
            TEST_EXPECT_EQUAL(root_info.hidden,   0);
            TEST_EXPECT_EQUAL(root_info.mark_sum, 6);
            TEST_EXPECT_EQUAL(root_info.leaf_sum, 15);

            TEST_EXPECT_SIMILAR(root_info.max_tree_depth, 1.624975, 0.000001);
            TEST_EXPECT_SIMILAR(root_info.min_tree_depth, 0.341681, 0.000001);

            GB_transaction ta(env.gbmain());
            GBT_mark_all(env.gbmain(), 0); // unmark all species
            root->compute_tree();
            TEST_EXPECT_EQUAL(root_info.mark_sum, 0);
        }


#define B1_TOP "(((((CloTyro3:1.046,CloTyro4:0.061):0.026,CloTyro2:0.017):0.017,CloTyrob:0.009):0.274,CloInnoc:0.371):0.057,CloBifer:0.388):0.124"
#define B1_BOT "(CloBifer:0.388,(CloInnoc:0.371,(CloTyrob:0.009,(CloTyro2:0.017,(CloTyro3:1.046,CloTyro4:0.061):0.026):0.017):0.274):0.057):0.124"
#define B2_TOP "(((CloButy2:0.009,CloButyr:0.000):0.564,CloCarni:0.120):0.010,CloPaste:0.179):0.131"
#define B2_BOT "(CloPaste:0.179,(CloCarni:0.120,(CloButy2:0.009,CloButyr:0.000):0.564):0.010):0.131"


#define B3_LEFT_TOP_SONS "(((CorAquat:0.084,CurCitre:0.058):0.103,CorGluta:0.522):0.053,CelBiazo:0.059)"
#define B3_TOP_SONS      B3_LEFT_TOP_SONS ":0.207,CytAquat:0.711"
#define B3_TOP_SONS_CCR  "((CorAquat:0.187,CorGluta:0.522):0.053,CelBiazo:0.059):0.207,CytAquat:0.711" // CCR = CurCitre removed
#define B3_TOP           "(" B3_TOP_SONS "):0.081"
#define B3_BOT           "(CytAquat:0.711,(CelBiazo:0.059,(CorGluta:0.522,(CorAquat:0.084,CurCitre:0.058):0.103):0.053):0.207):0.081"


        const char *top_topo    = "((" B1_TOP "," B2_TOP "):0.081," B3_TOP ");";
        const char *edge_topo   = "((" B1_TOP "," B2_BOT "):0.081," B3_BOT ");";
        const char *bottom_topo = "(" B3_BOT ",(" B2_BOT "," B1_BOT "):0.081);";

        const char *radial_topo  =
            "(((CloPaste:0.179,((CloButy2:0.009,CloButyr:0.000):0.564,CloCarni:0.120):0.010):0.131,"
            "((CloInnoc:0.371,((CloTyro2:0.017,(CloTyro3:1.046,CloTyro4:0.061):0.026):0.017,CloTyrob:0.009):0.274):0.057,CloBifer:0.388):0.124):0.081,"
            "((CelBiazo:0.059,((CorAquat:0.084,CurCitre:0.058):0.103,CorGluta:0.522):0.053):0.207,CytAquat:0.711):0.081);";
        const char *radial_topo2 =
            "(((CloBifer:0.388,(CloInnoc:0.371,(((CloTyro3:1.046,CloTyro4:0.061):0.026,CloTyro2:0.017):0.017,CloTyrob:0.009):0.274):0.057):0.124," B2_TOP "):0.081,"
            "(CytAquat:0.711," B3_LEFT_TOP_SONS ":0.207):0.081);";

        // expect that no mode reproduces another mode:
        TEST_EXPECT_DIFFERENT(top_topo,    edge_topo);
        TEST_EXPECT_DIFFERENT(top_topo,    bottom_topo);
        TEST_EXPECT_DIFFERENT(top_topo,    radial_topo);
        TEST_EXPECT_DIFFERENT(top_topo,    radial_topo2);
        TEST_EXPECT_DIFFERENT(edge_topo,   bottom_topo);
        TEST_EXPECT_DIFFERENT(edge_topo,   radial_topo);
        TEST_EXPECT_DIFFERENT(edge_topo,   radial_topo2);
        TEST_EXPECT_DIFFERENT(bottom_topo, radial_topo);
        TEST_EXPECT_DIFFERENT(bottom_topo, radial_topo2);
        TEST_EXPECT_DIFFERENT(radial_topo, radial_topo2);

        env.push(); // 1st stack level (=top_topo)

        TEST_EXPECT_VALID_TREE(root);

        TEST_EXPECT_NEWICK(nLENGTH, root, top_topo);
        // test reorder_tree:
        root->reorder_tree(BIG_BRANCHES_TO_EDGE);     TEST_EXPECT_NEWICK(nLENGTH, root, edge_topo);    env.push(); // 2nd stack level (=edge_topo)
        root->reorder_tree(BIG_BRANCHES_TO_BOTTOM);   TEST_EXPECT_NEWICK(nLENGTH, root, bottom_topo);  env.push(); // 3rd stack level (=bottom_topo)
        root->reorder_tree(BIG_BRANCHES_TO_CENTER);   TEST_EXPECT_NEWICK(nLENGTH, root, radial_topo);
        root->reorder_tree(BIG_BRANCHES_ALTERNATING); TEST_EXPECT_NEWICK(nLENGTH, root, radial_topo2);
        root->reorder_tree(BIG_BRANCHES_TO_TOP);      TEST_EXPECT_NEWICK(nLENGTH, root, top_topo);

        TEST_EXPECT_VALID_TREE(root);

        // test set root:
        AP_tree_nlen *CloTyrob = root->findLeafNamed("CloTyrob");
        TEST_REJECT_NULL(CloTyrob);

        ARB_edge rootEdge(root->get_leftson(), root->get_rightson());
        CloTyrob->set_root();

        TEST_EXPECT_VALID_TREE(root);

        const char *rootAtCloTyrob_topo =
            "(CloTyrob:0.004,"
            "(((CloTyro3:1.046,CloTyro4:0.061):0.026,CloTyro2:0.017):0.017,"
            "((((" B3_TOP_SONS "):0.162," B2_TOP "):0.124,CloBifer:0.388):0.057,CloInnoc:0.371):0.274):0.004);";

        TEST_EXPECT_NEWICK(nLENGTH, root, rootAtCloTyrob_topo);
        env.push(); // 4th stack level (=rootAtCloTyrob_topo)

        TEST_EXPECT_VALID_TREE(root);

        AP_tree_nlen *CelBiazoFather = root->findLeafNamed("CelBiazo")->get_father();
        TEST_REJECT_NULL(CelBiazoFather);
        CelBiazoFather->set_root();

        const char *rootAtCelBiazoFather_topo = "(" B3_LEFT_TOP_SONS ":0.104,((" B1_TOP "," B2_TOP "):0.162,CytAquat:0.711):0.104);";
        TEST_EXPECT_NEWICK(nLENGTH, root, rootAtCelBiazoFather_topo);

        TEST_EXPECT_VALID_TREE(root);

        ARB_edge oldRootEdge(rootEdge.source(), rootEdge.dest());
        DOWNCAST(AP_tree_nlen*,oldRootEdge.son())->set_root();

        const char *rootSetBack_topo = top_topo;
        TEST_EXPECT_NEWICK(nLENGTH, root, rootSetBack_topo);
        env.push(); // 5th stack level (=rootSetBack_topo)

        TEST_EXPECT_VALID_TREE(root);

        // test remove:
        AP_tree_nlen *CurCitre = root->findLeafNamed("CurCitre");
        TEST_REJECT_NULL(CurCitre);
        TEST_REJECT_NULL(CurCitre->get_father());

        CurCitre->REMOVE();
        const char *CurCitre_removed_topo = "((" B1_TOP "," B2_TOP "):0.081,(" B3_TOP_SONS_CCR "):0.081);";
        // ------------------------------------------------------------------- ^^^ = B3_TOP_SONS minus CurCitre
        TEST_EXPECT_NEWICK(nLENGTH, root, CurCitre_removed_topo);

        TEST_EXPECT_VALID_TREE(root);
        TEST_EXPECT_VALID_TREE(CurCitre);

        TEST_EXPECT_EQUAL(root->gr.leaf_sum, 15); // out of date
        root->compute_tree();
        TEST_EXPECT_EQUAL(root->gr.leaf_sum, 14);

        env.push(); // 6th stack level (=CurCitre_removed_topo)

        TEST_EXPECT_VALID_TREE(root);

        // test insert:
        AP_tree_nlen *CloCarni = root->findLeafNamed("CloCarni");
        TEST_REJECT_NULL(CloCarni);
        CurCitre->insert(CloCarni); // this creates two extra edges (not destroyed by destroy() below) and one extra node

        const char *CurCitre_inserted_topo = "((" B1_TOP ",(((CloButy2:0.009,CloButyr:0.000):0.564,(CurCitre:0.060,CloCarni:0.060):0.060):0.010,CloPaste:0.179):0.131):0.081,(" B3_TOP_SONS_CCR "):0.081);";
        TEST_EXPECT_NEWICK(nLENGTH, root, CurCitre_inserted_topo);

        TEST_EXPECT_VALID_TREE(root);

        // now check pops:
        env.pop(); TEST_EXPECT_NEWICK(nLENGTH, root, CurCitre_removed_topo); TEST_EXPECT_VALID_TREE(root);
        env.pop(); TEST_EXPECT_NEWICK(nLENGTH, root, rootSetBack_topo);      TEST_EXPECT_VALID_TREE(root);
        env.pop(); TEST_EXPECT_NEWICK(nLENGTH, root, rootAtCloTyrob_topo);   TEST_EXPECT_VALID_TREE(root);
        env.pop(); TEST_EXPECT_NEWICK(nLENGTH, root, bottom_topo);           TEST_EXPECT_VALID_TREE(root);
        env.pop(); TEST_EXPECT_NEWICK(nLENGTH, root, edge_topo);             TEST_EXPECT_VALID_TREE(root);
        env.pop(); TEST_EXPECT_NEWICK(nLENGTH, root, top_topo);              TEST_EXPECT_VALID_TREE(root);
    }
}

void TEST_calc_bootstraps() {
    PARSIMONY_testenv<AP_sequence_parsimony> env("TEST_trees.arb", "ali_5s");
    TEST_EXPECT_NO_ERROR(env.load_tree("tree_test"));

    const char *bs_origi_topo = "(((((((CloTyro3,CloTyro4)'40%',CloTyro2)'0%',CloTyrob)'97%',CloInnoc)'0%',CloBifer)'53%',(((CloButy2,CloButyr)'100%',CloCarni)'33%',CloPaste)'97%')'100%',((((CorAquat,CurCitre)'100%',CorGluta)'17%',CelBiazo)'40%',CytAquat)'100%');";
    const char *bs_limit_topo = "(((((((CloTyro3,CloTyro4)'87%',CloTyro2)'0%',CloTyrob)'100%',CloInnoc)'87%',CloBifer)'83%',(((CloButy2,CloButyr)'99%',CloCarni)'17%',CloPaste)'56%')'61%',((((CorAquat,CurCitre)'78%',CorGluta)'0%',CelBiazo)'59%',CytAquat)'61%');";
    const char *bs_estim_topo = "(((((((CloTyro3,CloTyro4)'75%',CloTyro2)'0%',CloTyrob)'100%',CloInnoc)'75%',CloBifer)'78%',(((CloButy2,CloButyr)'99%',CloCarni)'13%',CloPaste)'32%')'53%',((((CorAquat,CurCitre)'74%',CorGluta)'0%',CelBiazo)'56%',CytAquat)'53%');";

    {
        AP_tree_nlen *root      = env.root_node();
        AP_tree_edge *root_edge = rootEdge();

        TEST_EXPECT(root && root_edge);

        root->reorder_tree(BIG_BRANCHES_TO_TOP); TEST_EXPECT_NEWICK(nREMARK, root, bs_origi_topo);

        TEST_EXPECT_EQUAL(env.combines_performed(), 0);

        root_edge->nni_rec(ANY_EDGE, AP_BL_MODE(AP_BL_BL_ONLY|AP_BL_BOOTSTRAP_LIMIT),    NULL, true);
        root->reorder_tree(BIG_BRANCHES_TO_TOP);
        TEST_EXPECT_NEWICK(nREMARK, root, bs_limit_topo);
        TEST_EXPECT_EQUAL(env.combines_performed(), 170);

        root_edge->nni_rec(ANY_EDGE, AP_BL_MODE(AP_BL_BL_ONLY|AP_BL_BOOTSTRAP_ESTIMATE), NULL, true);
        root->reorder_tree(BIG_BRANCHES_TO_TOP);
        TEST_EXPECT_NEWICK(nREMARK, root, bs_estim_topo);
        TEST_EXPECT_EQUAL(env.combines_performed(), 156);

        TEST_EXPECT_EQUAL(env.root_node(), root);
    }

}

void TEST_tree_remove_add_all() {
    // reproduces crash as described in #527
    PARSIMONY_testenv<AP_sequence_parsimony> env("TEST_trees.arb", "ali_5s");
    TEST_EXPECT_NO_ERROR(env.load_tree("tree_nj"));

    const int     LEAFS     = 6;
    AP_tree_nlen *leaf[LEAFS];
    const char *name[LEAFS] = {
        "CloButy2",
        "CloButyr",
        "CytAquat",
        "CorAquat",
        "CurCitre",
        "CorGluta",
    };

    AP_tree_nlen *root = env.root_node();

    for (int i = 0; i<LEAFS; ++i) {
        leaf[i] = root->findLeafNamed(name[i]);
        TEST_REJECT_NULL(leaf[i]);
    }

    TEST_EXPECT_VALID_TREE(root);

    AP_pars_root *troot = leaf[0]->get_tree_root();
    TEST_REJECT_NULL(troot);

    for (int i = 0; i<LEAFS-1; ++i) {
        // Note: removing the second to last leaf, "removes" both remaining
        // leafs (but only destroys their father node)

        TEST_EXPECT_VALID_TREE(root);
        leaf[i]->REMOVE();
        TEST_EXPECT_VALID_TREE(leaf[i]);
    }

    leaf[0]->initial_insert(leaf[1], troot);
    for (int i = 2; i<LEAFS; ++i) {
        TEST_EXPECT_VALID_TREE(leaf[i-1]);
        TEST_EXPECT_VALID_TREE(leaf[i]);
        leaf[i]->insert(leaf[i-1]);
    }
}

#endif // UNIT_TESTS

// --------------------------------------------------------------------------------
