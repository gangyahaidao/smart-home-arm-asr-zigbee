#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

int main(void)
{
	printf("Content-type: text/html\n\n");
	char readBuf[10] = {0};

	FILE *fp = NULL;
	fp = fopen("./test.txt", "r");
	fgets(readBuf, 10, fp);

	printf("{\"temp_value\":\"%s\"}", readBuf);
	fclose(fp);
    
	return 0;
}