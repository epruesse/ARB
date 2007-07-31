#include <stdio.h>
#include <names_client.h>

main(argc, argv)
	int argc;
	char **argv;
{
	int zw;
	int com;
	aisc_com *link;

	if(argc<3){
		printf("syntax: %s <machine name> address\n", argv[0]);
		exit(1);
	}
	link = (aisc_com *)aisc_open(argv[1], &com, AISC_MAGIC_NUMBER);
	if (!link) {
		printf("cannot contact server\n", argv[0]);
		exit(1);
	}
	zw = atoi(argv[2]);
	if(!zw) zw = (int)com;
	printf("%s\n",md2(zw));
	exit(0);

}
