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
#include <math.h>

struct BlockInfo{
	char FSI[16];
	int BS;
	int FSS;
	int FATStart;
	int FATBlocks;
	int ROOTStart;
	int ROOTBlocks;
};

struct FATInfo{
	int Free;
	int Reserved;
	int Allocated;
};

struct FATInfo fatInfo;
struct BlockInfo blockInfo;


void part1(FILE *fp){
	char superblock[512];
	fread(superblock,sizeof(superblock),1, fp);

	
	memcpy(blockInfo.FSI, superblock, 8);
	printf("Super block information:\n");

	blockInfo.BS = superblock[8]*pow(16,2) + superblock[9];
	printf("Block size: %d\n", blockInfo.BS);
	
	blockInfo.FSS = superblock[10]*pow(16,6) + superblock[11]*pow(16,4) + superblock[12]*pow(16,2) + superblock[13];
	printf("Block count: %d\n", blockInfo.FSS);

	blockInfo.FATStart = superblock[14]*pow(16,6) + superblock[15]*pow(16,4) + superblock[16]*pow(16,2) + superblock[17];
	printf("FAT starts: %d\n", blockInfo.FATStart);

	blockInfo.FATBlocks = superblock[18]*pow(16,6) + superblock[19]*pow(16,4) + superblock[20]*pow(16,2) + superblock[21];
	printf("FAT blocks: %d\n", blockInfo.FATBlocks);

	blockInfo.ROOTStart = superblock[22]*pow(16,6) + superblock[23]*pow(16,4) + superblock[24]*pow(16,2) + superblock[25];
	printf("Root directory start: %d\n", blockInfo.FATBlocks);

	blockInfo.FATBlocks = superblock[26]*pow(16,6) + superblock[27]*pow(16,4) + superblock[28]*pow(16,2) + superblock[29];
	printf("Root directory blocks: %d\n\n", blockInfo.FATBlocks);

	printf("FAT information:\n");

	char FATBlock[blockInfo.BS];
	int i;
	int j;
	for(i=1;i<blockInfo.FATStart;i++){
		fread(FATBlock,blockInfo.BS,1, fp);
	}

	for(i=1;i<blockInfo.FATBlocks;i++){
		fread(FATBlock,blockInfo.BS,1, fp);
		for(j=0;j<blockInfo.BS;j=j+4){
			int value;
			if((value = FATBlock[j] + FATBlock[j+1] + FATBlock[j+2] + FATBlock[j+3]) == 0){
				fatInfo.Free++;
			}else if(value = (FATBlock[j] + FATBlock[j+1] + FATBlock[j+2] + FATBlock[j+3]) == 1){
				fatInfo.Reserved++;
			}else{
				value = (FATBlock[j] + FATBlock[j+1] + FATBlock[j+2] + FATBlock[j+3]);
				fatInfo.Allocated++;
			}
			//printf("I SEE VALUE: %d", value);
		}
	}
	printf("FREE: %d\n",fatInfo.Free);
	printf("Allocated: %d\n",fatInfo.Allocated);
	printf("Reserved: %d\n",fatInfo.Reserved);
	printf("TOTAL: %d\n",fatInfo.Free + fatInfo.Allocated + fatInfo.Reserved);


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