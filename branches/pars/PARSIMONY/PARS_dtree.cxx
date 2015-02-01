// =============================================================== //
//                                                                 //
//   File      : PARS_dtree.cxx                                    //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#include "pars_dtree.hxx"
#include "pars_main.hxx"
#include "pars_awars.h"
#include "ap_tree_nlen.hxx"
#include "ap_main.hxx"

#include <AP_seq_dna.hxx>
#include <AP_seq_protein.hxx>
#include <AP_filter.hxx>

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

struct TimedCombines {
    clock_t ticks;
    long    combines;

    TimedCombines()
        : ticks(clock()),
          combines(AP_sequence::combine_count())
    {}
};

class OptiPerfMeter {
    std::string   what;
    TimedCombines start;
    AP_FLOAT      start_pars;

public:
    OptiPerfMeter(std::string what_, AP_FLOAT start_pars_)
        : what(what_),
          start_pars(start_pars_)
    {}

    void dump(FILE *out, AP_FLOAT end_pars) {
        TimedCombines end;

        ap_assert(end_pars<=start_pars);

        double   seconds      = double(end.ticks-start.ticks)/CLOCKS_PER_SEC;
        AP_FLOAT pars_improve = start_pars-end_pars;
        long     combines     = end.combines-start.combines;

        double combines_per_second  = combines/seconds;
        double combines_per_improve = combines/pars_improve;
        double improve_per_second   = pars_improve/seconds;

        fprintf(out, "%-27s took %7.2f sec,  improve=%9.1f,  combines=%12li  (comb/sec=%10.2f,  comb/impr=%10.2f,  impr/sec=%10.2f)\n",
                what.c_str(),
                seconds,
                pars_improve,
                combines,
                combines_per_second,
                combines_per_improve,
                improve_per_second);
    }
};

static void AWT_graphic_parsimony_root_changed(void *cd, AP_tree *old, AP_tree *newroot) {
    AWT_graphic_tree *agt = (AWT_graphic_tree*)cd; // @@@ dynacast?
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

void ArbParsimony::kernighan_optimize_tree(AP_tree_nlen *at, const KL_Settings& settings, const AP_FLOAT *pars_global_start) {
    AP_FLOAT       pars_curr = get_root_node()->costs();
    const AP_FLOAT pars_org  = pars_curr;

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
    }

    int            anzahl = settings.random_nodes*at->count_leafs(); // @@@ rename
    AP_tree_nlen **list   = at->getRandomNodes(anzahl);

    arb_progress progress(anzahl);

    if (pars_global_start) {
        progress.subtitle(GBS_global_string("best=%.1f (gain=%.1f)", pars_curr, *pars_global_start-pars_curr));
    }
    else {
        progress.subtitle(GBS_global_string("best=%.1f", pars_curr));
    }

    for (int i=0; i<anzahl && !progress.aborted(); i++) {
        AP_tree_nlen *tree_elem = list[i];

        bool in_folded_group = tree_elem->gr.hidden ||
            (tree_elem->father && tree_elem->get_father()->gr.hidden);

        if (!in_folded_group) { // @@@ unwanted hardcoded check for group
            ap_main->remember();

            bool better_tree_found = tree_elem->kernighan_rec(KL, 0, pars_curr);

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
        }
        progress.inc();
    }
    delete [] list;
    performance.dump(stdout, pars_curr);
}



void ArbParsimony::optimize_tree(AP_tree_nlen *at, const KL_Settings& settings, arb_progress& progress) {
    AP_tree_nlen   *oldrootleft  = get_root_node()->get_leftson();
    AP_tree_nlen   *oldrootright = get_root_node()->get_rightson();
    const AP_FLOAT  org_pars     = get_root_node()->costs();
    AP_FLOAT        prev_pars    = org_pars;

    OptiPerfMeter overallPerf("global optimization", org_pars);

    progress.subtitle(GBS_global_string("best=%.1f", org_pars));

    OptiPerfMeter *nniPerf = NULL;

    while (!progress.aborted()) {
        if (!nniPerf) nniPerf = new OptiPerfMeter("(Multiple) recursive NNIs", prev_pars);
        AP_FLOAT nni_pars = at->nn_interchange_rec(UNLIMITED, ANY_EDGE, AP_BL_NNI_ONLY);

        if (nni_pars == prev_pars) { // NNI did not reduce costs -> kern-lin
            nniPerf->dump(stdout, nni_pars);
            delete nniPerf;
            nniPerf = NULL;

            // @@@ perform a hardcoded kernighan_optimize_tree here (no path reduction; depth ~ 3)
            // @@@ if no improvement perform custom KL, otherwise repeat hardcoded KL

            kernighan_optimize_tree(at, settings, &org_pars);
            AP_FLOAT ker_pars = get_root_node()->costs();
            if (ker_pars == prev_pars) break; // kern-lin did not improve tree -> done
            ap_assert(prev_pars>ker_pars);    // otherwise kernighan_optimize_tree worsened the tree
            prev_pars         = ker_pars;
        }
        else {
            ap_assert(prev_pars>nni_pars); // otherwise nn_interchange_rec worsened the tree
            prev_pars = nni_pars;
        }
        progress.subtitle(GBS_global_string("best=%.1f (gain=%.1f)", prev_pars, org_pars-prev_pars));
    }

    delete nniPerf;

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
                     makeWindowCallback(AWT_resize_cb, ntw),
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
                     "--unused", "--unused", // (this is necessary because ARB_PARS and ARB_NTREE use the same tree painting routines)
                     "--unused", "--unused",
                     "--unused", "--unused",

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
            if (event.type() == AW_Mouse_Press) {
                switch (event.button()) {
                    case AW_BUTTON_LEFT: {
                        if (clicked.node()) {
                            arb_progress  progress("NNI optimize subtree");
                            AP_tree_nlen *atn = DOWNCAST(AP_tree_nlen*, clicked.node());
                            atn->nn_interchange_rec(UNLIMITED, ANY_EDGE, AP_BL_NNI_ONLY);
                            exports.save = 1;
                            ASSERT_VALID_TREE(get_root_node());
                        }
                        break;
                    }
                    case AW_BUTTON_RIGHT: {
                        arb_progress progress("NNI optimize tree");
                        long         prevCombineCount = AP_sequence::combine_count();
                        
                        get_root_node()->nn_interchange_rec(UNLIMITED, ANY_EDGE, AP_BL_NNI_ONLY);
                        printf("Combines: %li\n", AP_sequence::combine_count()-prevCombineCount);

                        exports.save = 1;
                        ASSERT_VALID_TREE(get_root_node());
                        break;
                    }

                    default: break;
                }
            }
            break;
        case AWT_MODE_KERNINGHAN:
            if (event.type() == AW_Mouse_Press) {
                switch (event.button()) {
                    case AW_BUTTON_LEFT:
                        if (clicked.node()) {
                            arb_progress  progress("Kernighan-Lin optimize subtree");
                            parsimony.kernighan_optimize_tree(DOWNCAST(AP_tree_nlen*, clicked.node()), KL_Settings(aw_root), NULL);
                            this->exports.save = 1;
                            ASSERT_VALID_TREE(get_root_node());
                        }
                        break;
                    case AW_BUTTON_RIGHT: {
                        arb_progress progress("Kernighan-Lin optimize tree");
                        parsimony.kernighan_optimize_tree(get_root_node(), KL_Settings(aw_root), NULL);
                        this->exports.save = 1;
                        ASSERT_VALID_TREE(get_root_node());
                        break;
                    }
                    default: break;
                }
            }
            break;
        case AWT_MODE_OPTIMIZE:
            if (event.type()==AW_Mouse_Press) {
                switch (event.button()) {
                    case AW_BUTTON_LEFT:
                        if (clicked.node()) {
                            arb_progress  progress("Optimizing subtree");
                            parsimony.optimize_tree(DOWNCAST(AP_tree_nlen*, clicked.node()), KL_Settings(aw_root), progress);
                            this->exports.save = 1;
                            ASSERT_VALID_TREE(get_root_node());
                        }
                        break;
                    case AW_BUTTON_RIGHT: {
                        arb_progress progress("Optimizing tree");
                        
                        parsimony.optimize_tree(get_root_node(), KL_Settings(aw_root), progress);
                        this->exports.save = 1;
                        ASSERT_VALID_TREE(get_root_node());
                        break;
                    }
                    default: break;
                }
            }
            break;

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

    klSettings = new KL_Settings(GBT_get_alignment_type(gb_main, aliName));

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
        root->compute_tree();

        // first check initial state:
        {
            AP_tree_members& root_info = root->gr;

            TEST_EXPECT_EQUAL(root_info.grouped,             0);
            TEST_EXPECT_EQUAL(root_info.hidden,              0);
            TEST_EXPECT_EQUAL(root_info.has_marked_children, 1);
            TEST_EXPECT_EQUAL(root_info.leaf_sum,            15);

            TEST_EXPECT_SIMILAR(root_info.max_tree_depth, 1.624975, 0.000001);
            TEST_EXPECT_SIMILAR(root_info.min_tree_depth, 0.341681, 0.000001);

            GB_transaction ta(env.gbmain());
            GBT_mark_all(env.gbmain(), 0); // unmark all species
            root->compute_tree();
            TEST_EXPECT_EQUAL(root_info.has_marked_children, 0);
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

        TEST_EXPECT(root && rootEdge);

        root->reorder_tree(BIG_BRANCHES_TO_TOP); TEST_EXPECT_NEWICK(nREMARK, root, bs_origi_topo);

        TEST_EXPECT_EQUAL(env.combines_performed(), 0);

        root_edge->nni_rec(UNLIMITED, ANY_EDGE, AP_BL_MODE(AP_BL_BL_ONLY|AP_BL_BOOTSTRAP_LIMIT),    NULL);
        root->reorder_tree(BIG_BRANCHES_TO_TOP);
        TEST_EXPECT_NEWICK(nREMARK, root, bs_limit_topo);
        TEST_EXPECT_EQUAL(env.combines_performed(), 214);

        root_edge->nni_rec(UNLIMITED, ANY_EDGE, AP_BL_MODE(AP_BL_BL_ONLY|AP_BL_BOOTSTRAP_ESTIMATE), NULL);
        root->reorder_tree(BIG_BRANCHES_TO_TOP);
        TEST_EXPECT_NEWICK(nREMARK, root, bs_estim_topo);
        TEST_EXPECT_EQUAL(env.combines_performed(), 200);

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
