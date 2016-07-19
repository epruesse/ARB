// ============================================================== //
//                                                                //
//   File      : aw_varupdate.hxx                                 //
//   Purpose   :                                                  //
//                                                                //
//   Institute of Microbiology (Technical University Munich)      //
//   http://www.arb-home.de/                                      //
//                                                                //
// ============================================================== //

#ifndef AW_VARUPDATE_HXX
#define AW_VARUPDATE_HXX

#if defined(DEBUG)
// #define DUMP_BUTTON_CREATION
#endif // DEBUG

class VarUpdateInfo : virtual Noncopyable { // used to refresh single items on change
    AW_window      *aw_parent;
    Widget          widget;
    AW_widget_type  widget_type;
    AW_awar        *awar;
    AW_scalar       value;
    AW_cb          *cbs;

    union TS { // AW_widget_type-specific
        AW_selection_list *sellist;    // only used for AW_WIDGET_SELECTION_LIST
        AW_ScalerType      scalerType; // only used for AW_WIDGET_SCALER
        TS() : sellist(NULL) {}
    } ts;

    void check_unwanted_callback() {
#if defined(DEVEL_RALF)
        if (cbs) {
            fprintf(stderr, "Warning: Unwanted user-callback bound to VarUpdateInfo. Should be rewritten using an awar-callback\n");
        }
        // aw_assert(!cbs); // @@@ generally unwanted (see #559)
        // @@@ if this assertion stops failing -> remove cbs from VarUpdateInfo!
#endif
#if defined(ASSERTION_USED)
        if (cbs) {
            aw_assert(widget_type != AW_WIDGET_SELECTION_LIST); // binding callbacks to selection lists is obsolete (see #559)
        }
#endif
    }

public:
    VarUpdateInfo(AW_window *aw, Widget w, AW_widget_type wtype, AW_awar *a, AW_cb *cbs_)
        : aw_parent(aw),
          widget(w),
          widget_type(wtype),
          awar(a),
          value(a),
          cbs(cbs_)
    {
        check_unwanted_callback();
    }
    template<typename T>
    VarUpdateInfo(AW_window *aw, Widget w, AW_widget_type wtype, AW_awar *a, T t, AW_cb *cbs_)
        : aw_parent(aw),
          widget(w),
          widget_type(wtype),
          awar(a),
          value(t),
          cbs(cbs_)
    {
        check_unwanted_callback();
    }

    void change_from_widget(XtPointer call_data);

    void set_widget(Widget w) { widget = w; }

    void set_sellist(AW_selection_list *asl) {
        aw_assert(widget_type == AW_WIDGET_SELECTION_LIST);
        ts.sellist = asl;
    }
    void set_scalerType(AW_ScalerType scalerType_) {
        aw_assert(widget_type == AW_WIDGET_SCALER);
        ts.scalerType = scalerType_;
    }

    AW_awar *get_awar() { return awar; }
};

#define SPACE_BEHIND_BUTTON 3

void AW_variable_update_callback(Widget /*wgt*/, XtPointer variable_update_struct, XtPointer call_data);
void aw_attach_widget(Widget w, AW_at *_at, int default_width = -1);

#else
#error aw_varupdate.hxx included twice
#endif // AW_VARUPDATE_HXX
