#ifndef P_
# if defined(__STDC__) || defined(__cplusplus)
#  define P_(s) s
# else
#  define P_(s) ()
# endif
#else
# error P_ already defined elsewhere
#endif

#ifdef __cplusplus
extern "C" {
#endif    
    
    struct aisc_hash_node;    
    
/* struct_man.c */
struct aisc_hash_node **aisc_init_hash P_((int size));
int aisc_hash P_((char *key, int size));
void aisc_free_key P_((struct aisc_hash_node **table, char *key));
void aisc_free_hash P_((struct aisc_hash_node **table));
void aisc_insert_hash P_((struct aisc_hash_node **table, char *key, long data));
long aisc_read_hash P_((struct aisc_hash_node **table, char *key));
const char *aisc_link P_((dllpublic_ext *parent, dllheader_ext *mh));
const char *aisc_unlink P_((dllheader_ext *mh));
long aisc_find_lib P_((dllpublic_ext *parent, char *ident));
int trf_hash P_((long p));
long trf_create P_((long old, long new_item));
void trf_link P_((long old, long *dest));
int trf_begin P_((void));
int trf_commit P_((int errors));
int aisc_server_dllint_2_bytestring P_((dllpublic_ext *pb, bytestring *bs, int offset));
int aisc_server_dllstring_2_bytestring P_((dllpublic_ext *pb, bytestring *bs, int offset));

#ifdef __cplusplus
}
#endif

#undef P_
