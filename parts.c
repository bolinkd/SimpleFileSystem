#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <time.h>

int main(int argc, char* argv[]){
	#if defined(PART1)
		printf ("Part 1: diskinfo\n");
	#elif defined(PART2)
		printf ("Part 2: disklist\n");
	#elif defined(PART3)
		printf ("Part 3: diskget\n");
	#elif defined(PART4)
		printf ("Part 4: diskput\n");
	#else
	# 	error "PART[1234] must be defined"
	#endif
	return 0;
} 