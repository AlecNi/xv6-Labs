#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

//define _TEST
//define _TEST_PATH

char buf[1024];

int match(char*,char*);
int matchhere(char*,char*);
int matchstar(int,char*,char*);
char* fmtname(char*);
void find(char*,char*);

int match(char *re, char *text)
{
	if(re[0] == '^')
		return matchhere(re + 1, text);
	do{
		if(matchhere(re, text))
			return 1;
	}while(*text++ != '\0');
	return 0;
}

int matchhere(char *re, char *text)
{
	if(re[0] == '\0')
		return 1;
	if(re[1] == '*')
		return matchstar(re[0],re + 2,text);
	if(re[0] == '$' && re[1] == '\0')
		return * text == '\0';
	if(*text != '\0' && (re[0] == '.' || re[0] == *text))
		return matchhere(re + 1, text + 1);
	return 0;
}

int matchstar(int c, char *re, char *text)
{
	do{
		if(matchhere(re,text))
			return 1;
	}while(*re != '\0' && (*text++ == c || c == '.'));
	return 0;
}

char* fmtname(char *path)
{
	static char buf[DIRSIZ+1];
	char *p;

	for(p=path+strlen(path); p >= path && *p != '/'; p--)
		;
	p++;

	if(strlen(p) >= DIRSIZ)
		return p;
	memset(buf, 0, sizeof(buf));
	memmove(buf, p, strlen(p));
	return buf;
}

void find(char *path, char *re)
{
#ifdef _TEST_PATH
	printf("in find, path: %s\n",path);
#endif
	char buf[512], *p;
	int fd;
	struct dirent de;
	struct stat st;

	if((fd = open(path, 0)) < 0){
		fprintf(2, "find: cannot open %s\n", path);
		return;
	}

	if(fstat(fd, &st) < 0){
		fprintf(2, "find: cannot stat %s\n", path);
		close(fd);
		return;
	}

	switch(st.type){
		case T_FILE:
#ifdef _TEST
			printf("is file path\n", path);
#endif
		
			if(match(re,fmtname(path)))
				printf("%s\n",path);
			break;

		case T_DIR:
#ifdef _TEST_CMP
			printf("is dir path\n", path);
#endif
			if(strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)){
				printf("find: path too long\n");				
				break;							
			}
			           
		strcpy(buf, path);
		p = buf+strlen(buf);
		*p++ = '/';
		while(read(fd, &de, sizeof(de)) == sizeof(de)){
			if(de.inum == 0)
				continue;
			memmove(p, de.name, DIRSIZ);
			p[DIRSIZ] = 0;
			if(stat(buf, &st) < 0){
				printf("find: cannot stat %s\n", buf);		
				continue;
			}
					
			char *cmp = fmtname(buf);
#ifdef _TEST
			printf("cmp: %s\n",cmp);
#endif
			if(strcmp(".",cmp) == 0 || strcmp("..",cmp) == 0)
					continue;
			else
				find(buf,re);
		}
		break;
	
		default:
#ifdef _TEST
		printf("neither file or dir\n");
#endif
		break;
	}
	close(fd);
}

int main(int argc, char *argv[])
{
	if(argc < 2)
		printf("Parameters are not enough\n");
	else
		find(argv[1], argv[2]);

	exit(0);
}
