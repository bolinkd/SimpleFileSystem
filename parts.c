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

struct ImageInfo{
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

struct DirInfo{
	int DirStart;
	int DirSize;
};

struct ImageInfo imageInfo;

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

void setSuperImageInfo(char *p){
	imageInfo.BlockSize  = getImageInfo(p, 8 , 2);
	imageInfo.BlockCount = getImageInfo(p, 10, 4);
	imageInfo.FATStart   = getImageInfo(p, 14, 4);
	imageInfo.FATBlocks  = getImageInfo(p, 18, 4);
	imageInfo.ROOTStart  = getImageInfo(p, 22, 4);
	imageInfo.ROOTBlocks = getImageInfo(p, 26, 4);
}

void part1(char* mmap){
	int i;
	printf("Block size: %d\n", imageInfo.BlockSize);
	printf("Block count: %d\n", imageInfo.BlockCount);
    printf("Block where FAT start: %d\n", imageInfo.FATStart);
    printf("Block in FAT: %d\n", imageInfo.FATBlocks);
    printf("Block where ROOT start: %d\n", imageInfo.ROOTStart);
    printf("Block in ROOT: %d\n", imageInfo.ROOTBlocks);
    int FATStartByte = (imageInfo.BlockSize*imageInfo.FATStart);
    int FATSize = imageInfo.BlockSize*imageInfo.FATBlocks;
    for(i=FATStartByte; i<FATStartByte+FATSize ;i=i+4){
    	int FATEntry = getImageInfo(mmap,i,4);
    	if(FATEntry == 0){
    		imageInfo.Free++;
    	}else if(FATEntry == 1){
    		imageInfo.Reserved++;
    	}else{
    		imageInfo.Allocated++;
    	}
    }
    printf("Free: %d\n", imageInfo.Free);
    printf("Allocated: %d\n", imageInfo.Allocated);
    printf("Reserved: %d\n", imageInfo.Reserved);

}

int findNameLength(char *FileLocation){
	int retVal = 0;
	while(FileLocation[retVal] != '/' && FileLocation[retVal] != '\0'){
		retVal++;
	}
	return retVal++;
}

void printDirectory(char *mmap, struct DirInfo dirInfo){
	int i,j;
	printf("%d %d\n", dirInfo.DirSize, dirInfo.DirStart);
	int DirectoriesPerBlock = imageInfo.BlockSize/64;
	char *directory_entry = (char *)malloc(sizeof(char) * 64);
	int offset = (dirInfo.DirStart)*imageInfo.BlockSize;
	char *directoryName = (char *)malloc(sizeof(char) * 31);
	for(i=0;i<dirInfo.DirSize;i++){
		for(j=0;j<DirectoriesPerBlock;j++){
			directory_entry = memcpy(directory_entry, (mmap+offset)+(i*imageInfo.BlockSize)+(64*j), 64);
			//printf("Directory entry: \n %s\n",directory_entry);
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

					directoryName = memcpy(directoryName, directory_entry + 27, 31);

					printf("%c %10d %30s ",ForD, file_size, directoryName);
					printf("%4d/%02d/%02d %2d:%02d:%02d\n", year, month, day, hour, minute, second);
			}else{
				// Is not a file or directory
			}
		}
	}
}

struct DirInfo findDirectory(char* mmap, char* fileLocation, int BlockStart, int NumBlocks){
	struct DirInfo dirInfo;
	dirInfo.DirSize = 0;
	dirInfo.DirStart = 0;
	while(strncmp(fileLocation,"/",1) == 0){
		fileLocation++;
	}
	int i,j;
	int fileNameLength = findNameLength(fileLocation);
	char *fileName = malloc(sizeof(char)*fileNameLength);
	fileName = strncpy(fileName,fileLocation,fileNameLength);

	int DirectoriesPerBlock = imageInfo.BlockSize/64;
	char *directory_entry = (char *)malloc(sizeof(char) * 64);
	int offset = BlockStart*imageInfo.BlockSize;
	char *directoryName = (char *)malloc(sizeof(char) * 31);

	for(i=0;i<NumBlocks;i++){
		for(j=0;j<DirectoriesPerBlock;j++){
			directory_entry = memcpy(directory_entry, mmap+offset+i*imageInfo.BlockSize+64*j, 64);
			if(fileNameLength == 0){
				dirInfo.DirSize = NumBlocks;
				dirInfo.DirStart = BlockStart;
			}else{
				//get new directory
				if((directory_entry[0] & 0x05) == 0x07){
					directoryName = memcpy(directoryName, directory_entry + 27, 31);
					if(strcmp(fileName, directoryName) == 0){
						BlockStart = getImageInfo(directory_entry,1,4);
						NumBlocks  = getImageInfo(directory_entry,5,4);
						for(i=0;i<fileNameLength;i++){
							fileLocation++;
						}
						dirInfo = findDirectory(mmap, fileLocation, BlockStart, NumBlocks);
					}
				}
			}
		}
	}

	return dirInfo;

}

void part2(char* mmap, char* fileLocation){

	struct DirInfo dirInfo = findDirectory(mmap, fileLocation, imageInfo.ROOTStart, imageInfo.ROOTBlocks);
	if(dirInfo.DirSize == 0 && dirInfo.DirStart == 0){
		printf("Error Code: -1\n");
		return;
	}else{
		printf("Success Code: ");
		printDirectory(mmap, dirInfo);
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
		setSuperImageInfo(p);
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