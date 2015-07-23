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
#include <arpa/inet.h>


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
	uint32_t *Blocks;
	uint32_t *BlocksHex;
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
	//printf("FileName: %s\n",fileInfo.fileName);
	//printf("FileStart: %d\n",fileInfo.startBlock);
	//printf("FileSize: %d\n",fileInfo.numBlocks);
	fflush(stdout);
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
	int FATStartByte = (imageInfo.BlockSize*imageInfo.FATStart);
    int FATSize = imageInfo.BlockSize*imageInfo.FATBlocks;
    int i;
    for(i=FATStartByte; i<FATStartByte+FATSize ;i=i+4){
    	int FATEntry = getImageInfo(p,i,4);
    	if(FATEntry == 0){
    		imageInfo.Free++;
    	}else if(FATEntry == 1){
    		imageInfo.Reserved++;
    	}else{
    		imageInfo.Allocated++;
    	}
    }
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
	//printf("%d %d\n", dirInfo.DirSize, dirInfo.DirStart);
	int DirectoriesPerBlock = imageInfo.BlockSize/64;
	char *directory_entry = (char *)malloc(sizeof(char) * 64);
	int offset = (dirInfo.DirStart)*imageInfo.BlockSize;
	for(i=0;i<dirInfo.DirSize;i++){
		for(j=0;j<DirectoriesPerBlock;j++){
			directory_entry = memcpy(directory_entry, (mmap+offset)+(i*imageInfo.BlockSize)+(64*j), 64);
			if((directory_entry[0] & 0x03) == 0x03 || (directory_entry[0] & 0x07) == 0x05){
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
	int *Blocks = malloc(sizeof(int)*NumBlocks);
	//printf("Blocks: ");
	Blocks[0] = BlockStart;
	for(i=0;i<NumBlocks;i++){
		//printf("%d ",Blocks[i]);
		j = imageInfo.FATStart*imageInfo.BlockSize+Blocks[i]*4;
		Blocks[i+1] = getImageInfo(mmap,j,4);
	}
	//printf("\n");
	int fileNameLength = findNameLength(fileLocation);
	//printf("FN:  %s\n", fileLocation);
	//printf("FNL: %d\n", fileNameLength);
	fflush(stdout);

	char *fileName = malloc(sizeof(char)*fileNameLength);
	fileName = strncpy(fileName,fileLocation,fileNameLength);

	int DirectoriesPerBlock = imageInfo.BlockSize/64;
	char *directory_entry = (char *)malloc(sizeof(char) * 64);
	char *directoryName = (char *)malloc(sizeof(char) * 31);

	for(i=0;i<NumBlocks;i++){
		int offset = Blocks[i]*imageInfo.BlockSize;
		for(j=0;j<DirectoriesPerBlock;j++){
			directory_entry = memcpy(directory_entry, mmap+offset+i*imageInfo.BlockSize+64*j, 64);
			if(fileNameLength == 0){
				//printf("WOWOWOWOWO\n");
				dirInfo.DirSize = NumBlocks;
				dirInfo.DirStart = BlockStart;
				return dirInfo;
			}else{
				//get new directory
				if((directory_entry[0] & 0x07) == 0x05){
					directoryName = memcpy(directoryName, directory_entry + 27, 31);
					//printf("Directory Name: %s\n", directoryName);
					fflush(stdout);
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
	//printf("%d %d\n", dirInfo.DirSize, dirInfo.DirStart);
	int DirectoriesPerBlock = imageInfo.BlockSize/64;
	char *directory_entry = (char *)malloc(sizeof(char) * 64);
	int offset = (dirInfo.DirStart)*imageInfo.BlockSize;
	for(i=0;i<dirInfo.DirSize;i++){
		for(j=0;j<DirectoriesPerBlock;j++){
			directory_entry = memcpy(directory_entry, (mmap+offset)+(i*imageInfo.BlockSize)+(64*j), 64);
			//printf("ENTRY: %02x\n",directory_entry[0]);
			if((directory_entry[0] & 0x03) == 0x03){
				fileInfo = fillFileInfo(directory_entry);
				//printf("%s == %s",fileName, fileInfo.fileName);
				fflush(stdout);
				if(strcmp(fileInfo.fileName,fileName) == 0){
					//printf("REturN");
					fflush(stdout);
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
	//printf("FLE OPEN\n");
	int i;
	int blockSize = imageInfo.BlockSize;
	int currBlock = fileInfo.startBlock;
	int bytesToCopy = fileInfo.fileSize;
	for(i=0;i<fileInfo.numBlocks;i++){
		int offset = imageInfo.FATStart*imageInfo.BlockSize + currBlock*4;
		//printf("offset %d\n",offset);
		fflush(stdout);
		if(bytesToCopy < blockSize){
			//printf("FLE OPEN\n");
			fflush(stdout);
			writeToFile(file, mmap, currBlock, bytesToCopy);
			break;
		}else{
			writeToFile(file, mmap, currBlock, blockSize);
		}
		bytesToCopy -= blockSize;
		//printf("bytesToCopy: %d\n",bytesToCopy);
		fflush(stdout);
		//printf("Before: %d\n",currBlock);
		fflush(stdout);
		currBlock = getImageInfo(mmap,offset,4);
		//printf("After: %d\n",currBlock);
		if(currBlock == -1){
			break;
		}
		fflush(stdout);
	}
	fclose(file);
}

uint32_t *findEmptyBlocks(char *mmap, int numReturn){
	uint32_t* rtnArray = malloc(sizeof(uint32_t)*numReturn);
	int i;
	int j=0;
	int k=0;
    int FATStartByte = (imageInfo.BlockSize*imageInfo.FATStart);
    int FATSize = imageInfo.BlockSize*imageInfo.FATBlocks;
    for(i=FATStartByte; i<FATStartByte+FATSize ;i=i+4){
    	if(k == numReturn){
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

void reserveBlocks(char *mmap, int numBlocks, uint32_t *Blocks, uint32_t *BlocksHex){
	int i;
	for(i=0;i<numBlocks;i++){
		int currBlock = Blocks[i];
		int offset = imageInfo.FATStart*imageInfo.BlockSize + currBlock*4;
		memcpy(mmap+offset, &BlocksHex[i+1], 4);
	}
}

void writeBlocksToDisk(char *output_mmap, char *input_mmap, int numBlocks, uint32_t *Blocks, int bytesToCopy){
	int i;
	int bytesWritten=0;
	for(i=0;i<numBlocks;i++){
		int offset = Blocks[i]*imageInfo.BlockSize;
		if(bytesToCopy < 512){
			memcpy(output_mmap+offset, input_mmap+bytesWritten, bytesToCopy);
			break;
		}else{
			memcpy(output_mmap+offset, input_mmap+bytesWritten, 512);
			bytesWritten += 512;
		}
		bytesToCopy-=512;
	}
}

int findAvailibleFileLocation(char *mmap,int Block){
	int offset = imageInfo.BlockSize * Block;
	int i;
	int j=0;
	char *directory_entry = malloc(sizeof(char)*65);
	for(i=0;i<imageInfo.BlockSize;i=i+64){
		memcpy(directory_entry, mmap+offset+i, 64);
		if(getImageInfo(directory_entry,58,6) == 65534){
			return j;
		}
		j++;
	}
	return -1;

}

struct DirInfo createDirectory(char *mmap, struct FileInfo dirInfo, int Block){
	struct DirInfo rtn;

	uint32_t *block = (uint32_t *)findEmptyBlocks(mmap, 1);
	uint32_t *blockHex = malloc(sizeof(uint32_t)*2);

	blockHex[0] = htonl(block[0]);
	blockHex[1] = htonl(0xFFFFFFFF);

	reserveBlocks(mmap, 1, block, blockHex);

	uint32_t numBlocks = htonl(1);
	uint8_t used = 0x00;
	uint8_t ForD = 0x5;
	uint32_t fileSize = htonl(dirInfo.fileSize);
	uint16_t year = htons(dirInfo.year);

	int offset = Block*imageInfo.BlockSize + findAvailibleFileLocation(mmap, Block)*64; 
	memcpy(mmap+offset, &ForD, 1);
	memcpy(mmap+offset+1, &blockHex[0], 4);
	memcpy(mmap+offset+5, &numBlocks, 4);
	memcpy(mmap+offset+9, &fileSize, 4);
	memcpy(mmap+offset+13, &year, 2);
	memcpy(mmap+offset+15, &dirInfo.month, 1);
	memcpy(mmap+offset+16, &dirInfo.day, 1);
	memcpy(mmap+offset+17, &dirInfo.hour, 1);
	memcpy(mmap+offset+18, &dirInfo.minute, 1);
	memcpy(mmap+offset+19, &dirInfo.second, 1);
	memcpy(mmap+offset+20, &year, 2);
	memcpy(mmap+offset+22, &dirInfo.month, 1);
	memcpy(mmap+offset+23, &dirInfo.day, 1);
	memcpy(mmap+offset+24, &dirInfo.hour, 1);
	memcpy(mmap+offset+25, &dirInfo.minute, 1);
	memcpy(mmap+offset+26, &dirInfo.second, 1);
	memcpy(mmap+offset+27, dirInfo.fileName, 31);
	memcpy(mmap+offset+58, &used, 1);

	rtn.DirStart = block[0];
	rtn.DirSize = 1;

	return rtn;
}

struct DirInfo findDirectorywCreation(char* mmap, char* fileLocation, int BlockStart, int NumBlocks){
	time_t rawtime;
    struct tm *timeinfo;
    time (&rawtime);
    timeinfo = localtime(&rawtime);
	struct DirInfo dirInfo;
	dirInfo.DirSize = 0;
	dirInfo.DirStart = 0;
	while(strncmp(fileLocation,"/",1) == 0){
		fileLocation++;
	}
	int i,j;
	int *Blocks = malloc(sizeof(int)*NumBlocks+1);
	Blocks[0] = BlockStart;
	for(i=0;i<NumBlocks;i++){
		j = imageInfo.FATStart*imageInfo.BlockSize+Blocks[0]*4;
		Blocks[i+1] = getImageInfo(mmap,j,4);
	}

	int fileNameLength = findNameLength(fileLocation);
	//printf("FN:  %s %d\n", fileLocation, NumBlocks);
	//printf("FNL: %d\n", fileNameLength);

	char *fileName = malloc(sizeof(char)*fileNameLength);

	fileName = strncpy(fileName,fileLocation,fileNameLength);
	//printf("filename: %s",fileName);

	int DirectoriesPerBlock = imageInfo.BlockSize/64;
	char *directory_entry = (char *)malloc(sizeof(char) * 64);
	char *directoryName = (char *)malloc(sizeof(char) * 31);

	for(i=0;i<NumBlocks;i++){
		int offset = Blocks[i]*imageInfo.BlockSize;
		for(j=0;j<DirectoriesPerBlock;j++){
			directory_entry = memcpy(directory_entry, mmap+offset+i*imageInfo.BlockSize+64*j, 64);
			if(fileNameLength == 0){
				dirInfo.DirSize = NumBlocks;
				dirInfo.DirStart = BlockStart;
			}else{
				//get new directory
				if((directory_entry[0] & 0x07) == 0x05){
					directoryName = memcpy(directoryName, directory_entry + 27, 31);
					if(strcmp(fileName, directoryName) == 0){
						BlockStart = getImageInfo(directory_entry,1,4);
						NumBlocks  = getImageInfo(directory_entry,5,4);
						for(i=0;i<fileNameLength;i++){
							fileLocation++;
						}
						dirInfo = findDirectorywCreation(mmap, fileLocation, BlockStart, NumBlocks);
					}
				}
			}
		}
	}
	if(dirInfo.DirSize == 0 && dirInfo.DirStart == 0){
		//Create Directory
		struct FileInfo f;
		f.ForD = 'D';
		f.fileSize = imageInfo.BlockSize;
		f.fileName = fileName;
		//printf("file Name: %s\n",f.fileName);
		f.year = timeinfo->tm_year+1900;
		f.month = timeinfo->tm_mon + 1;
		f.day = timeinfo->tm_mday;
		f.hour = timeinfo->tm_hour;
		f.minute = timeinfo->tm_min;
		f.second = timeinfo->tm_sec;
		dirInfo = createDirectory(mmap, f, BlockStart);
		//printf("NEW CREATE\n");
		for(i=0;i<fileNameLength;i++){
			fileLocation++;
		}
		//printf("%d %d\n", dirInfo.DirStart,dirInfo.DirSize);
		dirInfo = findDirectorywCreation(mmap, fileLocation, dirInfo.DirStart, dirInfo.DirSize);


	}
	return dirInfo;
}


void addFileToDirectory(char *mmap, struct FileInfo fileInfo, struct DirInfo dirInfo){

	uint32_t block = (uint32_t)dirInfo.DirStart;
	uint32_t blockHex = htonl(fileInfo.startBlock); 


	uint32_t numBlocks = htonl(fileInfo.numBlocks);
	uint8_t used = 0x00;
	uint8_t ForD = 0x3;
	uint32_t fileSize = htonl(fileInfo.fileSize);
	uint16_t year = htons(fileInfo.year);

	int offset = block*imageInfo.BlockSize; 
	memcpy(mmap+offset, &ForD, 1);
	memcpy(mmap+offset+1, &blockHex, 4);
	memcpy(mmap+offset+5, &numBlocks, 4);
	memcpy(mmap+offset+9, &fileSize, 4);
	memcpy(mmap+offset+13, &year, 2);
	memcpy(mmap+offset+15, &fileInfo.month, 1);
	memcpy(mmap+offset+16, &fileInfo.day, 1);
	memcpy(mmap+offset+17, &fileInfo.hour, 1);
	memcpy(mmap+offset+18, &fileInfo.minute, 1);
	memcpy(mmap+offset+19, &fileInfo.second, 1);
	memcpy(mmap+offset+20, &year, 2);
	memcpy(mmap+offset+22, &fileInfo.month, 1);
	memcpy(mmap+offset+23, &fileInfo.day, 1);
	memcpy(mmap+offset+24, &fileInfo.hour, 1);
	memcpy(mmap+offset+25, &fileInfo.minute, 1);
	memcpy(mmap+offset+26, &fileInfo.second, 1);
	memcpy(mmap+offset+27, fileInfo.fileName, 31);
	memcpy(mmap+offset+58, &used, 1);

	return;
}


int part4(char* output_mmap, char *fileName, char *fileLocation){
	int fd,i;
	char *input_mmap;
	struct stat sf;
	struct FileInfo fileInfo;	
	if ((fd=open(fileName, O_RDWR)))
	{
		fstat(fd, &sf);
		input_mmap = mmap(NULL,sf.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	}
	int fileNameLength = strlen(fileLocation);
	int fileNameStart = findFileNameStart(fileLocation);
	//printf("%d %d %d\n",fileNameLength, fileNameStart, fileNameLength-fileNameStart+1);

	char *diskFileName = malloc(sizeof(char)*(fileNameLength-fileNameStart));
	memcpy(diskFileName,fileLocation+fileNameStart,fileNameLength);
	fileLocation[fileNameStart] = '\0';

    time_t rawtime;
    struct tm *timeinfo;
    time (&rawtime);
    timeinfo = localtime(&rawtime);

	fileInfo.ForD = 'F';
	fileInfo.fileSize = sf.st_size;
	fileInfo.fileName = diskFileName;
	fileInfo.year = timeinfo->tm_year + 1900;
	fileInfo.month = timeinfo->tm_mon + 1;
	fileInfo.day = timeinfo->tm_mday;
	fileInfo.hour = timeinfo->tm_hour;
	fileInfo.minute = timeinfo->tm_min;
	fileInfo.second = timeinfo->tm_sec;
	fileInfo.numBlocks = (fileInfo.fileSize + imageInfo.BlockSize - 1) / imageInfo.BlockSize;

	if(fileInfo.numBlocks > imageInfo.Free){
		printf("Not Enough Space On Disk\n");
		return -1;
	}
	struct DirInfo dirInfo;
	//printf("%d %d\n", dirInfo.DirSize, dirInfo.DirStart);
	dirInfo = findDirectorywCreation(output_mmap, fileLocation, imageInfo.ROOTStart, imageInfo.ROOTBlocks);
	fileInfo.Blocks = findEmptyBlocks(output_mmap,fileInfo.numBlocks);
	fileInfo.BlocksHex = malloc(sizeof(uint32_t *)*fileInfo.numBlocks+1);
	for(i=0;i<fileInfo.numBlocks;i++){
		fileInfo.BlocksHex[i] = htonl(fileInfo.Blocks[i]);
	}
	fileInfo.startBlock = fileInfo.Blocks[0];
	//printf("THSSAFSGSGWG WE: %d", fileInfo.startBlock);
	fileInfo.BlocksHex[fileInfo.numBlocks] = 0xFFFFFFFF;
	reserveBlocks(output_mmap, fileInfo.numBlocks, fileInfo.Blocks, fileInfo.BlocksHex);
	writeBlocksToDisk(output_mmap, input_mmap, fileInfo.numBlocks, fileInfo.Blocks, fileInfo.fileSize);
	addFileToDirectory(output_mmap, fileInfo, dirInfo);


	return 1;

}

int part3(char* mmap, char* fileLocation, char* localFileName){
	int fileNameStart = findFileNameStart(fileLocation);
	char *fileName = malloc(sizeof(char) * 31);
	strcpy(fileName,fileLocation+fileNameStart);
	//printf("fileName: %s\n",fileName);
	fileLocation[fileNameStart] = '\0';
	//printf("fileLocation: %s\n",fileLocation);
	struct DirInfo dirInfo = findDirectory(mmap, fileLocation, imageInfo.ROOTStart, imageInfo.ROOTBlocks);
	if(dirInfo.DirSize == 0 && dirInfo.DirStart == 0){
		printf("Error Code: -1\n");
		return -1;
	}else{
		//printf("Success Code: ");
		fflush(stdout);
		struct FileInfo fileInfo = getFileInfo(mmap, dirInfo, fileName);
		//printf("%d %d", dirInfo.DirStart, dirInfo.DirSize);
		fflush(stdout);
		if(strcmp(fileInfo.fileName, "") == 0){
			return -1;
		}else{
			//printf("CopyFile\n");
			fflush(stdout);
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
		printDirectory(mmap, dirInfo);
		return;
	}
	

}

void part1(char* mmap){
	printf("Super block information:\n");
	printf("Block size: %d\n", imageInfo.BlockSize);
	printf("Block count: %d\n", imageInfo.BlockCount);
    printf("FAT starts: %d\n", imageInfo.FATStart);
    printf("FAT blocks: %d\n", imageInfo.FATBlocks);
    printf("Root directory start: %d\n", imageInfo.ROOTStart);
    printf("Root directory blocks: %d\n\n", imageInfo.ROOTBlocks);

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
		part3(p, argv[2],argv[3]);
	#elif defined(PART4)
		part4(p, argv[2],argv[3]);
	#else
	# 	error "PART[1234] must be defined"
	#endif
	return 0;
} 