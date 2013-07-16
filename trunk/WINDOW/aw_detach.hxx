// ================================================================= //
//                                                                   //
//   File      : aw_detach.hxx                                       //
//   Purpose   :                                                     //
//                                                                   //
//   Coded by Ralf Westram (coder@reallysoft.de) in September 2010   //
//   Institute of Microbiology (Technical University Munich)         //
//   http://www.arb-home.de/                                         //
//                                                                   //
// ================================================================= //

#ifndef AW_DETACH_HXX
#define AW_DETACH_HXX

#ifndef AW_AWAR_HXX
#include "aw_awar.hxx"
#endif
#ifndef AW_ROOT_HXX
#include "aw_root.hxx"
#endif

#include <sstream>

class Awar_Callback_Info : virtual Noncopyable {
    // this structure is used to store all information on an awar callback
    // and can be used to remove or remap awar callback w/o knowing anything else

    AW_root *awr;
    Awar_CB  callback;
    AW_CL    cd1, cd2;
    char    *awar_name;
    char    *org_awar_name;

    void init (AW_root *awr_, const char *awar_name_, Awar_CB2 callback_, AW_CL cd1_, AW_CL cd2_);

public:
    Awar_Callback_Info(AW_root *awr_, const char *awar_name_, Awar_CB2 callback_, AW_CL cd1_, AW_CL cd2_) { init(awr_, awar_name_, callback_, cd1_, cd2_); }
    Awar_Callback_Info(AW_root *awr_, const char *awar_name_, Awar_CB1 callback_, AW_CL cd1_)             { init(awr_, awar_name_, (Awar_CB2)callback_, cd1_, 0); }
    Awar_Callback_Info(AW_root *awr_, const char *awar_name_, Awar_CB0 callback_)                         { init(awr_, awar_name_, (Awar_CB2)callback_, 0, 0); }

    ~Awar_Callback_Info() {
        delete awar_name;
        delete org_awar_name;
    }

    void add_callback()    { awr->awar(awar_name)->add_callback(callback, cd1, cd2); }
    void remove_callback() { awr->awar(awar_name)->remove_callback(callback, cd1, cd2); }

    void remap(const char *new_awar);

    const char *get_awar_name() const     { return awar_name; }
    const char *get_org_awar_name() const { return org_awar_name; }

    AW_root *get_root() { return awr; }
};


/**
 * Holds information needed to detach a window.
 */
class AW_detach_information {
    Awar_Callback_Info *cb_info;
    AW_awar* label_awar;
public:
    
    /**
     * @param cb_info_ The callback that will be remaped upon detach
     * @param awar_base_name A unique awar identifier is created from this name. It is used for the label
     */
    AW_detach_information(Awar_Callback_Info *cb_info_, const char* awar_base_name)
        : cb_info(cb_info_), label_awar(NULL) {
        static unsigned counter = 0;
        std::stringstream ss;
        ss << awar_base_name << counter;
        ++counter;
        aw_assert(AW_root::SINGLETON);
        label_awar = AW_root::SINGLETON->awar_string(ss.str().c_str());
        aw_assert(label_awar);
        }

    Awar_Callback_Info *get_cb_info() const { return cb_info; }
    
    AW_awar *get_label_awar() const {
        return label_awar;
    }
};


#else
#error aw_detach.hxx included twice
#endif // AW_DETACH_HXX
