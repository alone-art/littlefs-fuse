#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>


#define IMAGE "/dev/loop14"


int user_flash_read(uint32_t address, void *buffer, uint32_t size)
{
	FILE *fp = fopen(IMAGE, "rb");
	uint32_t offset = address;


//	printf("read: ");

	if(fp)
	{
		fseek(fp, offset, SEEK_SET);
		fread(buffer, 1, size, fp);
		fclose(fp);
//		printf("offset %d, size %d %x %x\n", offset, size, ((char*)buffer)[0], ((char*)buffer)[1]);
	}


	return 0;
}

int user_flash_prog(uint32_t address, const void *buffer, uint32_t size)
{
	FILE *fp = fopen(IMAGE, "rb+");
	uint32_t offset = address;
//	printf("write: ");

	if(fp)
	{
		fseek(fp, offset, SEEK_SET);
		fwrite(buffer, 1, size, fp);
		fclose(fp);
/*		printf("offset %d, size %d\n", offset, size);
		printf("check: \n");
		unsigned char *p = (unsigned char *)buffer;
		for(int i=0; i<20; i++)
		{
			printf("%02x ", p[i]);
		}
		printf("\n");
		char buf[20] = {0};
		
		FILE *fd = fopen(IMAGE, "rb");
		fseek(fd, offset, SEEK_SET);
		fread(buf, sizeof(buf), 1, fd);
		fclose(fd);
		for(int i=0; i<20; i++)
		{
			printf("%02hhx ", buf[i]);
		}
		printf("\n");*/
	}

	return 0;
}

int user_flash_erase(uint32_t address, uint32_t block)
{
	FILE *fp = fopen(IMAGE, "rb+");
	uint32_t offset = address;
	printf("erase:");

	if(fp)
	{
		printf("offset %d, block %d\n", offset, block);
		char buffer[512] = {0};
		memset(buffer, 0xff, sizeof(buffer));
		fseek(fp, offset, SEEK_SET);
		for(int i=0; i<block; i++)
		{
			fwrite(buffer, 1, sizeof(buffer), fp);
		}
		fclose(fp);
	}
	return 0;
}

int user_flash_sync(void)
{

	printf("sync:\n");
	return 0;
}




