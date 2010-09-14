// =============================================================== //
//                                                                 //
//   File      : secedit_extern.hxx                                //
//   Purpose   : external interface                                //
//                                                                 //
//   Coded by Ralf Westram (coder@reallysoft.de) in July 2007      //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef SECEDIT_EXTERN_HXX
#define SECEDIT_EXTERN_HXX

struct AW_event;

struct SEC_host {
    // defines interface which has to be implemented by host (e.g. by EDIT4)

    virtual ~SEC_host() {}

    virtual bool SAIs_visualized() const                                = 0; // == true -> SECEDIT visualizes SAIs
    virtual const char *get_SAI_background(int start, int end) const    = 0; // get SAI background color
    virtual const char *get_search_background(int start, int end) const = 0; // get background color of search hits

    virtual int get_base_position(int seq_position) const = 0; // transform seq-pos(gaps) to base-pos(nogaps)

    virtual void forward_event(AW_event *event) const = 0; // all events not handled by SECEDIT are forwarded here

    virtual void announce_current_species(const char *species_name) = 0;
};

AW_window *SEC_create_main_window(SEC_host& host, AW_root *awr, GBDATA *gb_main);


#else
#error secedit_extern.hxx included twice
#endif // SECEDIT_EXTERN_HXX
