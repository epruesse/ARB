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

/* client.c */
int aisc_c_read P_((int socket, char *ptr, long size));
int aisc_c_write P_((int socket, char *ptr, int size));
void aisc_c_add_to_bytes_queue P_((char *data, int size));
int aisc_c_send_bytes_queue P_((aisc_com *link));
int aisc_add_message_queue P_((aisc_com *link, long size));
int aisc_check_error P_((aisc_com *link));
void *aisc_client_sigio P_((void));
char *aisc_client_get_hostname P_((void));
const char *aisc_client_get_m_id P_((char *path, char **m_name, int *id));
const char *aisc_client_open_socket P_((char *path, int delay, int do_connect, int *psocket, char **unix_name));
void *aisc_init_client P_((aisc_com *link));
aisc_com *aisc_open P_((char *path, long *mgr, long magic));
int aisc_close P_((aisc_com *link));
int aisc_get_message P_((aisc_com *link));
int aisc_get P_((aisc_com *link, int o_type, long objekt, ...));
long *aisc_debug_info P_((aisc_com *link, int o_type, long objekt, int attribute));
int aisc_put P_((aisc_com *link, int o_type, long objekt, ...));
int aisc_nput P_((aisc_com *link, int o_type, long objekt, ...));
int aisc_create P_((aisc_com *link, int father_type, long father, int attribute, int object_type, long *object, ...));
int aisc_copy P_((aisc_com *link, int s_type, long source, int father_type, long father, int attribute, int object_type, long *object, ...));
int aisc_delete P_((aisc_com *link, int objekt_type, long source));
int aisc_find P_((aisc_com *link, int father_type, long father, int attribute, int object_type, long *object, char *ident));

#ifdef __cplusplus
}
#endif

#undef P_
