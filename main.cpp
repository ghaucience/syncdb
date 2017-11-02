#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#include "sync.h"


static const char *ip		= "127.0.0.1";
static int port					= 8000;
static const char *path = "./test.db";



int main(int argc, char *argv[]) {

	int ret = 0;

	while (1) {
		printf("sync [%s]\n", path);
		ret = db_sync(ip, port, path);
		printf("sync ret is %d\n", ret);
		sleep(5);
	}

	return 0;
}
