#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "kernel/fs.h"
#include "kernel/param.h"

//#define _TEST

char *cut(char *buf);
void substring(char s[], char sub[], int pos, int len);

void substring(char s[], char sub[], int pos, int len)
{
    	int i = 0;
	while (i < len){
		*(sub + i) = s[pos+i];
		++i;
	}
	*(sub + i) = '\0';
}

char* cut(char *buf)
{
	if(strlen(buf) > 1 && buf[strlen(buf) - 1] == '\n'){
		char *subbuff = (char*)malloc(sizeof(char) * (strlen(buf) - 1));
		substring(buf, subbuff, 0, strlen(buf) - 1);
		return subbuff;
	}
	else{
		char *subbuff = (char*)malloc(sizeof(char) * strlen(buf));
		strcpy(subbuff, buf);
		return subbuff;
	}
}

int main(int argc, char *argv[])
{
	int pid;
	char buf[MAXPATH];
	char *args[MAXARG];
	char *cmd;

	if(argc == 1)
		cmd = "echo";
	else
		cmd = argv[1];

	int args_num = 0;
	while (1){
		memset(buf, 0, sizeof(buf));
		gets(buf, MAXPATH);
		char *arg = cut(buf);
		if(strlen(arg) == 0 || args_num >= MAXARG)
			break;
		args[args_num++]= arg;
	}

	int status;
	int argstartpos = 0;
	char *argv2exec[MAXARG];

	argv2exec[0] = cmd;

#ifdef _TEST
	for(int i = 0;i < argc;++i)
		printf("%s\n",argv[i]);
#endif
        while(argstartpos + 1 < argc){
		argv2exec[argstartpos] = argv[argstartpos + 1];	
		++argstartpos;
	}

	for (int i = 0; i < args_num; ++i)
		argv2exec[i + argstartpos] = args[i];

	argv2exec[args_num + argstartpos] = 0;

	if((pid = fork()) == 0)
		exec(cmd, argv2exec);
	else
		wait(&status);
	exit(0);
}
