#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* #include <malloc.h> */
#include <ad_varargs.h>
#include <aisc_com.h>
#include "client.h"
#include "aisc_global.h"

#ifdef __cplusplus
extern "C" {
const char *aisc_debug_local(aisc_com	*link,int key,long object,char *str,...);
}
#endif

const char *aisc_debug_local(aisc_com *link, int key, long object, char *str, ...) {
    static char buf[4096];
    static char *keystr;
    static long anz;
    long code,*er,type;
    char	*bptr,*sc,*sp,*sm;
    va_list parg;
    static char *gstring;
    static double gdouble;
    static long gint,father;
    static bytestring gbs;
    bptr = &buf[0];
    va_start(parg,str);
    sm = sc = strdup(str);
    if (aisc_get(link,AISC_COMMON,object,
		 COMMON_KEYSTRING, &keystr,
		 COMMON_CNT, &anz,
		 COMMON_PARENT, &father,
		 NULL)) return "unknown object";
    sprintf(bptr,"\naisc_get  (link,%14s,%20li, /* (Count %li ) */\n",
	    keystr,object,anz);
    bptr += strlen(bptr);
    if(father){
	if (!aisc_get(link,AISC_COMMON,father,
		      COMMON_KEYSTRING, &keystr,
		      COMMON_CNT, &anz,
		      NULL)){
	    sprintf(bptr,"%14s/* PARENT: %s %li (Count: %li)*/\n",
		    " ",keystr,father,anz);
	    bptr += strlen(bptr);
	}
    }
    while ( (code=va_arg(parg,long)) ) {
	for (sp=sc; ((*sp)!=0) && ((*sp)!=','); sp++);
	*sp = 0;
	sprintf(bptr,"%30s,",sc);
	bptr += strlen(bptr);
	sc += strlen(sc)+1;
	type = code & 0xff000000;
	er = (long *)aisc_debug_info(link,key,object,(int)code);
	if (!er)return"connection problems";
	if (!er[1]) {
	    switch (type) {
		case AISC_ATTR_INT:
		case AISC_ATTR_COMMON:
		    if (aisc_get(link, key, object, code, &gint, NULL))
			return "connection problems";
		    sprintf(bptr, "%20li, /* ", gint);
		    break;
		case AISC_ATTR_DOUBLE:
		    if (aisc_get(link, key, object, code, &gdouble, NULL))
			return "connection problems";
		    sprintf(bptr, "%20f, /* ", gdouble);
		    break;
		case AISC_ATTR_BYTES:
		    if (aisc_get(link, key, object, code, &gbs, NULL))
			return "connection problems";
		    sprintf(bptr, "%5d:%14s, /* ", gbs.size,gbs.data);
		    break;
		case AISC_ATTR_STRING:
		    if (aisc_get(link, key, object, code, &gstring, NULL))
			return "connection problems";
		    {
			char *buf2 = (char *)calloc(sizeof(char),strlen(gstring)+3);
			sprintf(buf2,"\"%s\"",gstring);
			sprintf(bptr, "%20s, /* ", buf);
			free(buf2);
			break;
		    }
	    }
	}else{
	    type = 0;
	    sprintf(bptr, "%20s, /* ", "Get not implemented");
	}
	bptr += strlen(bptr);
	er = (long *)aisc_debug_info(link,key,object,(int)code);
	/*if (!er[0]) *(bptr++) = 'd'; else *(bptr++) = '-';*/
	if (!er[1]) *(bptr++) = 'g'; else *(bptr++) = '-';
	if (!er[2]) *(bptr++) = 'p'; else *(bptr++) = '-';
	if (!er[3]) *(bptr++) = 'f'; else *(bptr++) = '-';
	if (!er[4]) *(bptr++) = 'c'; else *(bptr++) = '-';
	if (!er[5]) *(bptr++) = 'c'; else *(bptr++) = '-';
	if ((type == AISC_ATTR_COMMON) && (gint) ) {
	    if (aisc_get(link,AISC_COMMON,gint,
			 COMMON_KEYSTRING, &keystr,
			 COMMON_CNT, &anz,
			 NULL)) return "connection problems";
	    sprintf(bptr,"  %s (%li) */\n",keystr,anz);
	    bptr += strlen(bptr);
	}else{
	    sprintf(bptr, "*/\n");
	    bptr += strlen(bptr);
	}
    }
    sprintf(bptr, "\t)\n");
    bptr += strlen(bptr);
    free(sm);
    fwrite(buf,strlen(buf),1,stdout);
    /*printf("%s",buf);*/
    return "";
}

