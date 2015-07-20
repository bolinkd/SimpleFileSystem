#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h> 
#include <string.h>
#include <stdint.h>

struct BlockInfo{
	char FSI[16];
	int BlockSize;
	int BlockCount;
	int FATStart;
	int FATBlocks;
	int ROOTStart;
	int ROOTBlocks;
	int Free;
	int Allocated;
	int Reserved;
};

struct BlockInfo blockInfo;

int getImageInfo(char * mmap, int offset, int length)
{
        int i = 0 , retVal = 0;
        char *tmp = (char *)malloc(sizeof(char) * length);
        memcpy(tmp, mmap+offset, length);  
        for(i=0; i < length; i++){
                retVal += (((int)tmp[i]&0xFF)<<(8*(length - i - 1)));
        }
        free(tmp);
        return retVal;
};

void setSuperBlockInfo(char *p){
	blockInfo.BlockSize  = getImageInfo(p, 8 , 2);
	blockInfo.BlockCount = getImageInfo(p, 10, 4);
	blockInfo.FATStart   = getImageInfo(p, 14, 4);
	blockInfo.FATBlocks  = getImageInfo(p, 18, 4);
	blockInfo.ROOTStart  = getImageInfo(p, 22, 4);
	blockInfo.ROOTBlocks = getImageInfo(p, 26, 4);
}

void part1(char* mmap){
	int i;
	printf("Block size: %d\n", blockInfo.BlockSize);
	printf("Block count: %d\n", blockInfo.BlockCount);
    printf("Block where FAT start: %d\n", blockInfo.FATStart);
    printf("Block in FAT: %d\n", blockInfo.FATBlocks);
    printf("Block where ROOT start: %d\n", blockInfo.ROOTStart);
    printf("Block in ROOT: %d\n", blockInfo.ROOTBlocks);
    int FATStartByte = (blockInfo.BlockSize*blockInfo.FATStart);
    int FATSize = blockInfo.BlockSize*blockInfo.FATBlocks;
    for(i=FATStartByte; i<FATStartByte+FATSize ;i=i+4){
    	int FATEntry = getImageInfo(mmap,i,4);
    	if(FATEntry == 0){
    		blockInfo.Free++;
    	}else if(FATEntry == 1){
    		blockInfo.Reserved++;
    	}else{
    		blockInfo.Allocated++;
    	}
    }
    printf("Free: %d\n", blockInfo.Free);
    printf("Allocated: %d\n", blockInfo.Allocated);
    printf("Reserved: %d\n", blockInfo.Reserved);

}

int findNameLength(char *FileLocation){
	int retVal = 0;
	while(FileLocation[retVal] != '/' && FileLocation[retVal] != '\0'){
		//printf("%c\n",FileLocation[retVal]);
		retVal++;
	}
	return retVal++;
}

int findDirectory(char* mmap, char* fileLocation, int BlockStart, int NumBlocks){
	int found = 0;
	if(strncmp(fileLocation,"/",1) == 0){
		found = 1;
		fileLocation++;
	}
	int i,j;
	//printf("Start: %s\n",fileLocation);
	int fileNameLength = findNameLength(fileLocation);
	//printf("fileNameLength: %d\n", fileNameLength);
	char *fileName = malloc(sizeof(char)*fileNameLength);
	fileName = strncpy(fileName,fileLocation,fileNameLength);

	int DirectoriesPerBlock = blockInfo.BlockSize/64;
	char *directory_entry = (char *)malloc(sizeof(char) * 64);
	int offset = BlockStart*blockInfo.BlockSize;
	char *directory_name_bytes = (char *)malloc(sizeof(char) * 31);
	//unsigned char *file_size_bytes = (unsigned char *)malloc(sizeof(unsigned char) * 4);  

	char *directoryName = fileName;

	if(fileNameLength == 0){
		for(i=0;i<NumBlocks;i++){
			for(j=0;j<DirectoriesPerBlock;j++){
				directory_entry = memcpy(directory_entry, mmap+offset+i*blockInfo.BlockSize+64*j, 64);
				if((directory_entry[0] & 0x03) == 0x03 || (directory_entry[0] & 0x05) == 0x07){
					char ForD;
					int file_size  = getImageInfo(directory_entry, 9 , 4);
					uint16_t year  = getImageInfo(directory_entry, 20, 2);
					uint8_t month  = getImageInfo(directory_entry, 22, 1);
					uint8_t day    = getImageInfo(directory_entry, 23, 1);
					uint8_t hour   = getImageInfo(directory_entry, 24, 1);
					uint8_t minute = getImageInfo(directory_entry, 25, 1);
					uint8_t second = getImageInfo(directory_entry, 26, 1);

					if((directory_entry[0] & 0x03) == 0x03){
						ForD = 'F';
					}else{
						ForD = 'D';
					}

					directory_name_bytes = memcpy(directory_name_bytes, directory_entry + 27, 31);

					printf("%c %10d %30s ",ForD, file_size, directory_name_bytes);
					printf("%4d/%02d/%02d %2d:%02d:%02d\n", year, month, day, hour, minute, second);
				}
			}
		}
		return 1;
	}else{
		for(i=0;i<NumBlocks;i++){
			for(j=0;j<DirectoriesPerBlock;j++){
				directory_entry = memcpy(directory_entry, mmap+offset+i*blockInfo.BlockSize+64*j, 64);
				if ((directory_entry[0] & 0x05) == 0x07)   {
					directory_name_bytes = memcpy(directory_name_bytes, directory_entry + 27, 31);
					//printf("Directory Name: %s\n", directory_name_bytes);
					//printf("Directory Name: %s\n", directoryName);
					//set rest of info to check other information
					if(strcmp(directoryName, directory_name_bytes) == 0){
						found = 1;
					}
					//getImageInfo()

				}
			}
		}
	}


	if(found == 0){
		return -1;
	}

	free(fileName);
	for(i=0;i<fileNameLength;i++){
		fileLocation++;
	}

	int DirectoryBlock = findDirectory(mmap, fileLocation, BlockStart, NumBlocks);
	if(DirectoryBlock == -1){
		return -1;
	}else{
		return DirectoryBlock;
	}

}

void part2(char* mmap, char* fileLocation){

	int directoryLocation = findDirectory(mmap, fileLocation, blockInfo.ROOTStart, blockInfo.ROOTBlocks);
	if(directoryLocation == -1){
		printf("ERROR\n");
		return;
	}else{
		printf("WOOO\n");
		return;
	}
	

}

int main(int argc, char* argv[]){

	int fd;
	struct stat sf;
	char *p;
	
	if ((fd=open(argv[1], O_RDONLY)))
	{
		fstat(fd, &sf);
		p = mmap(NULL,sf.st_size, PROT_READ, MAP_SHARED, fd, 0);
		setSuperBlockInfo(p);
	}
	else
		printf("Fail to open the image file.\n");

	close(fd);

	#if defined(PART1)
		printf ("Part 1: diskinfo\n");
		part1(p);
	#elif defined(PART2)
		printf ("Part 2: disklist\n");
		part2(p, argv[2]);
	#elif defined(PART3)
		printf ("Part 3: diskget\n");
	#elif defined(PART4)
		printf ("Part 4: diskput\n");
	#else
	# 	error "PART[1234] must be defined"
	#endif
	return 0;
} 