// =============================================================== //
//                                                                 //
//   File      : aw_root_pimpl.hxx                                 //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef AW_ROOT_PIMPL_HXX
#define AW_ROOT_PIMPL_HXX

#ifdef DARWIN
# include <map>
# define ANYMAP std::map
#else
# ifdef Cxx11
#  include <unordered_map>
#  define ANYMAP std::unordered_map
# else
#  include <tr1/unordered_map>
#  define ANYMAP std::tr1::unordered_map
# endif
#endif

typedef ANYMAP<std::string, AW_action*> action_hash_t;
typedef ANYMAP<std::string, AW_awar*> awar_hash_t;

#undef ANYMAP

struct AW_root::pimpl : virtual Noncopyable {
    GdkCursor* cursors[3]; /** < The available cursors. Use AW_root::AW_Cursor as index when accessing */
    AW_active  active_mask;

    action_hash_t action_hash;
    awar_hash_t   awar_hash;

    pimpl() : active_mask(AWM_ALL) {}

#if defined(DEBUG)
    size_t callallcallbacks(int mode);
#endif
};

#else
#error aw_root_pimpl.hxx included twice
#endif // AW_ROOT_PIMPL_HXX
