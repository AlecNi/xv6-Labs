#include "kernel/types.h"
#include "user.h"
//#define _TEST

void init(int nums[]);
void send(int pd[], int infos[], int ptr);
void process(int pd[]);

int main(int argc, char *argv[])
{
	int nums[34];
	int pd[2];

	pipe(pd);
	init(nums);
	send(pd,nums,34);

#ifdef _TEST
	printf("init done, in process\n");
#endif
	process(pd);

	printf("unexcepted error!\n");
	exit(-1);
}

void process(int pd[])
{
	int prime;
	int num;
	int len;
	int pid;
	int child_pd[2];
	int infos[34];
	int ptr = 0;

#ifdef _TEST
	printf("in process, init parameters\n");
#endif
	pipe(child_pd);
#ifdef _TEST
	printf("pipe created\n");
#endif
	    
    	close(pd[1]);
	len = read(pd[0],&prime,sizeof(prime));
	printf("prime %d\n",prime);

#ifdef _TEST
	printf("in while\n");
#endif
	
	while(len){
		len = read(pd[0],&num,sizeof(num));
		if(num % prime){
			infos[ptr] = num;
			++ptr;
		}
	}
#ifdef _TEST
	printf("out while\n");
#endif
	close(pd[0]);
		     
     	if(ptr == 0)
		exit(0);

	pid = fork();
	if(pid)
		process(child_pd);
	else
		send(child_pd,infos,ptr);

	exit(0);
}

void init(int nums[])
{
	for(int i = 0; i < 34; ++i)
		nums[i] = i + 2;

	return;
}

void send(int pd[], int infos[], int len)
{
	int info;

	for(int i = 0; i < len; ++i){
				info = infos[i];
				write(pd[1],&info,sizeof(info));
	}

	return;
}
