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
#include <time.h>

#ifdef HAVE_ST_BIRTHTIME
#define birthtime(x) x.st_birthtime
#else
#define birthtime(x) x.st_ctime
#endif


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

struct FileInfo{
	char ForD;
	int fileSize;
	char *fileName;
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second;
	int numBlocks;
	int startBlock;
	int* Blocks;
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

struct FileInfo fillFileInfo(char *directory_entry){
	struct FileInfo fileInfo;
	fileInfo.fileName    = (char *)malloc(sizeof(char) * 31);
	fileInfo.startBlock  = getImageInfo(directory_entry, 1 , 4);
	fileInfo.numBlocks   = getImageInfo(directory_entry, 5 , 4);
	fileInfo.fileSize    = getImageInfo(directory_entry, 9 , 4);
	fileInfo.year        = getImageInfo(directory_entry, 20, 2);
	fileInfo.month       = getImageInfo(directory_entry, 22, 1);
	fileInfo.day         = getImageInfo(directory_entry, 23, 1);
	fileInfo.hour        = getImageInfo(directory_entry, 24, 1);
	fileInfo.minute      = getImageInfo(directory_entry, 25, 1);
	fileInfo.second      = getImageInfo(directory_entry, 26, 1);
	if((directory_entry[0] & 0x03) == 0x03){
		fileInfo.ForD = 'F';
	}else{
		fileInfo.ForD = 'D';
	}
	fileInfo.fileName = memcpy(fileInfo.fileName, directory_entry + 27, 31);
	return fileInfo;
}

void printFile(char *directory_entry){
	struct FileInfo fileInfo = fillFileInfo(directory_entry);
	printf("%c %10d %30s ",fileInfo.ForD, fileInfo.fileSize, fileInfo.fileName);
	printf("%4d/%02d/%02d %2d:%02d:%02d\n", fileInfo.year, fileInfo.month, fileInfo.day, fileInfo.hour, fileInfo.minute, fileInfo.second);
}

void setSuperImageInfo(char *p){
	imageInfo.BlockSize  = getImageInfo(p, 8 , 2);
	imageInfo.BlockCount = getImageInfo(p, 10, 4);
	imageInfo.FATStart   = getImageInfo(p, 14, 4);
	imageInfo.FATBlocks  = getImageInfo(p, 18, 4);
	imageInfo.ROOTStart  = getImageInfo(p, 22, 4);
	imageInfo.ROOTBlocks = getImageInfo(p, 26, 4);
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
	for(i=0;i<dirInfo.DirSize;i++){
		for(j=0;j<DirectoriesPerBlock;j++){
			directory_entry = memcpy(directory_entry, (mmap+offset)+(i*imageInfo.BlockSize)+(64*j), 64);
			if((directory_entry[0] & 0x03) == 0x03 || (directory_entry[0] & 0x05) == 0x07){
				printFile(directory_entry);
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

int findFileNameStart(char *fileLocation){
	int length = strlen(fileLocation);
	int i;
	for(i=length;i>=0;i--){
		if(fileLocation[i] == '/'){
			return i+1;
		}
	}
	return -1;

}


struct FileInfo getFileInfo(char *mmap, struct DirInfo dirInfo, char *fileName){
	int i,j;
	struct FileInfo fileInfo;
	fileInfo.fileName = "";
	printf("%d %d\n", dirInfo.DirSize, dirInfo.DirStart);
	int DirectoriesPerBlock = imageInfo.BlockSize/64;
	char *directory_entry = (char *)malloc(sizeof(char) * 64);
	int offset = (dirInfo.DirStart)*imageInfo.BlockSize;
	for(i=0;i<dirInfo.DirSize;i++){
		for(j=0;j<DirectoriesPerBlock;j++){
			directory_entry = memcpy(directory_entry, (mmap+offset)+(i*imageInfo.BlockSize)+(64*j), 64);
			if((directory_entry[0] & 0x03) == 0x03){
				fileInfo = fillFileInfo(directory_entry);
				if(strcmp(fileInfo.fileName,fileName) == 0){
					return fileInfo;
				}
			}else{
				// Is not a file
			}
		}
	}
	return fileInfo;
}

void writeToFile(FILE *file, char *mmap, int currBlock, int copyLength){
	int offset = currBlock*imageInfo.BlockSize;
	char *directory_entry = malloc(sizeof(char)*copyLength);
	memcpy(directory_entry,mmap+offset,copyLength);
	//printf("::::\n%s\n::::::\n",directory_entry);
	fwrite(mmap+offset, copyLength, 1, file);
}

void copyFile(char* mmap, struct FileInfo fileInfo, char* localFileName){
	FILE *file = fopen(localFileName, "wb");
	int i;
	int currBlock = fileInfo.startBlock;
	int bytesToCopy = fileInfo.fileSize;
	for(i=0;i<fileInfo.numBlocks;i++){
		int offset = imageInfo.FATStart*imageInfo.BlockSize + currBlock*4;
		if(bytesToCopy < 512){
			writeToFile(file, mmap, currBlock, bytesToCopy);
			break;
		}else{
			writeToFile(file, mmap, currBlock, 512);
		}
		bytesToCopy -= 512;
		currBlock = getImageInfo(mmap,offset,4);
	}
	fclose(file);
}

int* findEmptyBlocks(char *mmap, int numReturn){
	int* rtnArray = malloc(sizeof(int)*numReturn);
	int i;
	int j=0;
	int k=0;
    int FATStartByte = (imageInfo.BlockSize*imageInfo.FATStart);
    int FATSize = imageInfo.BlockSize*imageInfo.FATBlocks;
    for(i=FATStartByte; i<FATStartByte+FATSize ;i=i+4){
    	if(k == numReturn){
		    //for(i=0;i<numReturn;i++){
		    //	printf("%d ",rtnArray[i]);
		    //}
		    //printf("\n");
		    return rtnArray;   		
    	}
    	int FATEntry = getImageInfo(mmap,i,4);
    	if(FATEntry == 0){
    		rtnArray[k] = j;
    		k++;
    	}
    	j++;
    }
    return NULL;
}

int part4(char* mmap, char *fileName, char *fileLocation){
	int fd;
	struct stat sf;
	struct FileInfo fileInfo;	
	if ((fd=open(fileName, O_RDONLY)))
	{
		fstat(fd, &sf);
	}
    time_t rawtime;
    struct tm *timeinfo;
    time (&rawtime);
    timeinfo = localtime(&rawtime);
	fileInfo.ForD = 'F';
	fileInfo.fileSize = sf.st_size;
	fileInfo.fileName = fileName;
	fileInfo.year = timeinfo->tm_year + 1900;
	fileInfo.month = timeinfo->tm_mon + 1;
	fileInfo.day = timeinfo->tm_mday;
	fileInfo.hour = timeinfo->tm_hour;
	fileInfo.minute = timeinfo->tm_min;
	fileInfo.second = timeinfo->tm_sec;
	fileInfo.numBlocks = (fileInfo.fileSize + imageInfo.BlockSize - 1) / imageInfo.BlockSize;
	fileInfo.Blocks = findEmptyBlocks(mmap,fileInfo.numBlocks);
	if(fileInfo.Blocks == NULL){
		return -1;
	}

	//printf("%d\n%d\n%d\n",fileInfo.numBlocks, fileInfo.fileSize, fileInfo.startBlock);

	return 1;

}

int part3(char* mmap, char* fileLocation, char* localFileName){
	int fileNameStart = findFileNameStart(fileLocation);
	char *fileName = malloc(sizeof(char) * 31);
	strcpy(fileName,fileLocation+fileNameStart);
	fileLocation[fileNameStart] = '\0';
	struct DirInfo dirInfo = findDirectory(mmap, fileLocation, imageInfo.ROOTStart, imageInfo.ROOTBlocks);
	if(dirInfo.DirSize == 0 && dirInfo.DirStart == 0){
		printf("Error Code: -1\n");
		return -1;
	}else{
		printf("Success Code: ");
		struct FileInfo fileInfo = getFileInfo(mmap, dirInfo, fileName);
		if(strcmp(fileInfo.fileName, "") == 0){
			return -1;
		}else{
			copyFile(mmap, fileInfo, localFileName);
		}
		return -1;
	}	


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

void part1(char* mmap){
	int i;
	printf("Super block information:\n");
	printf("Block size: %d\n", imageInfo.BlockSize);
	printf("Block count: %d\n", imageInfo.BlockCount);
    printf("FAT starts: %d\n", imageInfo.FATStart);
    printf("FAT blocks: %d\n", imageInfo.FATBlocks);
    printf("Root directory start: %d\n", imageInfo.ROOTStart);
    printf("Root directory blocks: %d\n\n", imageInfo.ROOTBlocks);
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
    printf("FAT information: \n");
    printf("Free Blocks: %d\n", imageInfo.Free);
    printf("Reserved Blocks: %d\n", imageInfo.Reserved);
    printf("Allocated Blocks: %d\n", imageInfo.Allocated);

}

int main(int argc, char* argv[]){

	int fd;
	struct stat sf;
	char *p;
	
	if ((fd=open(argv[1], O_RDWR)))
	{
		fstat(fd, &sf);
		p = mmap(NULL,sf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
		setSuperImageInfo(p);
	}
	else
		printf("Fail to open the image file.\n");

	close(fd);

	#if defined(PART1)
		part1(p);
	#elif defined(PART2)
		part2(p, argv[2]);
	#elif defined(PART3)
		printf ("Part 3: diskget\n");
		part3(p, argv[2],argv[3]);
	#elif defined(PART4)
		printf ("Part 4: diskput\n");
		part4(p, argv[2],argv[3]);
	#else
	# 	error "PART[1234] must be defined"
	#endif
	return 0;
} 