#include <stdlib.h>

#include "ps_filebuffer.hxx"

int main( void ) {
    PS_FileBuffer *fb = new PS_FileBuffer( "testdata",true );
    char *data = (char *)malloc(1024);
    fb->get(data,4096);
    fb->get(data,100);

    free(data);
    delete fb;
}
