#include <stdio.h>
#include <PT_com.h>
#include <client.h>
extern "C" {
	char *md2(void * object);
}
main(int argc,char ** argv)
{
	int zw;
	T_PT_MAIN com;
	aisc_com *link;
	if(argc<3){
		printf("syntax: %s <machine name> address\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	link = (aisc_com *)aisc_open(argv[1], &com, AISC_MAGIC_NUMBER);
	if (!link) {
		printf("cannot contact server\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	zw = atoi(argv[2]);
	if(!zw) zw = (int)com;
	printf("%s\n",md2((void *)zw));
}
