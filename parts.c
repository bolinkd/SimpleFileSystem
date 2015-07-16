#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <fcntl.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>

struct BlockInfo{
	char FSI[16];
	char BShex[4];
	char BS[4];
	int BSint;
	char FSS[8];
	char FSShex[8];
	int FSSint;
	char FATStart[8];
	char FATBlocks[8];
	char ROOTStart[8];
	char ROOTBlocks[8];
};

struct BlockInfo blockInfo;
void part1(FILE *fp){
	char superblock[512];
	fread(superblock,sizeof(superblock),1, fp);
	
	memcpy(blockInfo.FSI, superblock, 8);
	printf("Super block information: %s\n",blockInfo.FSI);
	memcpy(blockInfo.BShex, &superblock[8], 2);
	memcpy(blockInfo.FSShex, &superblock[10],4);
	int i;
	int j;
	for (i = 0; i < 2; i++)  {
 		sprintf(&blockInfo.BS[i*2],"%02x", blockInfo.BShex[i]);	
	}
	blockInfo.BSint = (int)strtol(blockInfo.BS,NULL,16);
	printf("Block size: %d\n", blockInfo.BSint);
	for (j = 0; j < 4; j++)  {
 		sprintf(&blockInfo.FSS[j*2],"%02x", blockInfo.FSShex[j]);	
	}
	blockInfo.FSSint = (int)strtol(blockInfo.FSS,NULL,16);
	printf("Block count: %d\n", blockInfo.FSSint);
	printf("FAT starts: \n");
	printf("FAT blocks: \n");

}

int main(int argc, char* argv[]){

	FILE *fp = fopen(argv[1],"rb");
	if(fp == NULL){
		printf("NULL File descriptor");
		return -1;
	}

	#if defined(PART1)
		printf ("Part 1: diskinfo\n");
		part1(fp);
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