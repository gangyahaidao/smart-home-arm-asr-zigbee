#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

int main(void)
{
	printf("Content-type: text/html\n\n");
	float curr_temp = 23.1;
	printf("{\"temp_value\":\"%.2f\"}", curr_temp);
    
	return 0;
}