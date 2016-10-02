// =============================================================== //
//                                                                 //
//   File      : gb_memory.h                                       //
//   Purpose   :                                                   //
//                                                                 //
//   Institute of Microbiology (Technical University Munich)       //
//   http://www.arb-home.de/                                       //
//                                                                 //
// =============================================================== //

#ifndef GB_MEMORY_H
#define GB_MEMORY_H

#if defined(DEBUG)
#define MEMORY_TEST 1
// MEMORY_TEST == 1 uses malloc and normal pointers for internal ARBDB memory
// -> memory checkers like valgrand work
// -> loading DB is slower (file must be parsed completely)
#else
#define MEMORY_TEST 0
// MEMORY_TEST == 0 uses own allocation and relative pointers for internal ARBDB memory
// -> memory checkers like valgrand won't be very useful
// -> debugging is difficult, cause it's difficult to follow pointers
// -> DB loads w/o delay, cause it's mmap'ped into memory
#endif

#if defined(UNIT_TESTS) // UT_DIFF
#undef MEMORY_TEST
#define MEMORY_TEST 0 // test mmapped-DB version in unittests (recommended setting; same as in RELEASE)
// #define MEMORY_TEST 1 // test DEBUG DB version in unittests
#endif

struct gb_if_entries;
struct gb_index_files;
struct gb_header_list;
struct GBDATA;
struct GBCONTAINER;

#if (MEMORY_TEST==1)

typedef void            *GB_REL_ADD;
typedef char            *GB_REL_STRING;
typedef GBDATA          *GB_REL_GBDATA; // @@@ use GBENTRY?
typedef GBCONTAINER     *GB_REL_CONTAINER;
typedef gb_header_list  *GB_REL_HLS;
typedef gb_if_entries   *GB_REL_IFES;
typedef gb_index_files  *GB_REL_IFS;
typedef gb_if_entries  **GB_REL_PIFES;

#define UNUSED_IN_MEMTEST(param) param = param

#else

typedef long GB_REL_ADD;
typedef long GB_REL_STRING;
typedef long GB_REL_GBDATA;
typedef long GB_REL_CONTAINER;
typedef long GB_REL_HLS;
typedef long GB_REL_IFES;
typedef long GB_REL_IFS;
typedef long GB_REL_PIFES;

#endif

// -------------------------------
//      GB_RESOLVE / GB_SETREL
//
// set GB_REL_* "pointers"

#if (MEMORY_TEST==1)

#define GB_RESOLVE(typ, struct_add, member_name)    ((typ)((struct_add)->member_name))
#define GB_SETREL(struct_add, member_name, address) (struct_add)->member_name = (address)

#else

#define GB_RESOLVE(typ, struct_add, member_name)                       \
    ((typ)(((struct_add)->member_name)                                 \
           ? (typ) (((char*)(struct_add))+((struct_add)->member_name)) \
           : NULL))

#define GB_SETREL(struct_add, member_name, address)                     \
    do {                                                                \
        char *pntr = (char *)(address);                                 \
        if (pntr) {                                                     \
            (struct_add)->member_name = (char*)(pntr)-(char*)(struct_add); \
        }                                                               \
        else {                                                          \
            (struct_add)->member_name = 0;                              \
        }                                                               \
    } while (0)


#endif

// --------------------------------
//      ARBDB memory functions

enum ARB_MEMORY_INDEX {
    GBM_CB_INDEX     = -1, // Note: historical name. originally was used to allocate database-callback-memory (which is now done with new and delete)
    GBM_HASH_INDEX   = -2,
    GBM_HEADER_INDEX = -3,
    GBM_UNDO         = -4,
    GBM_DICT_INDEX   = -5,
    GBM_USER_INDEX   = -6
};

// gbm_get_mem returns a block filled with zero (like calloc() does)

#if defined(DEBUG)
#if defined(DEVEL_RALF)
#define FILL_MEM_ON_FREE 0xdb
#endif
#endif

#if (MEMORY_TEST==1)

inline void *gbm_get_mem(size_t size, long )              { return ARB_calloc<char>(size); }
#if defined(FILL_MEM_ON_FREE)
inline void gbm_free_mem(void *block, size_t size, long ) { memset(block, FILL_MEM_ON_FREE, size); free(block); }
#else // !defined(FILL_MEM_ON_FREE)
inline void gbm_free_mem(void *block, size_t , long )     { free(block); }
#endif

#else // MEMORY_TEST==0

void *gbmGetMemImpl(size_t size, long index);
void gbmFreeMemImpl(void *data, size_t size, long index);

inline void *gbm_get_mem(size_t size, long index)              { return gbmGetMemImpl(size, index); }
inline void gbm_free_mem(void *block, size_t size, long index) {
#if defined(FILL_MEM_ON_FREE)
    memset(block, FILL_MEM_ON_FREE, size);
#endif
    gbmFreeMemImpl(block, size, index);
}

#endif // MEMORY_TEST

#else
#error gb_memory.h included twice
#endif // GB_MEMORY_H
