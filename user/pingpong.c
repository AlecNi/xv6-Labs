#include "kernel/types.h"
#include "user/user.h"

int main(int agrc, char *agrv[])
{	      
	int pid;
	int parent[2];
	int child[2];
	char child_buf[20] = {0};
	char parent_buf[20] = {0};

	pipe(child);
	pipe(parent);

	//Child
	if((pid = fork()) == 0){
		close(parent[1]);
		close(child[0]);
		read(parent[0],child_buf,20);

		close(parent[0]);
		printf("%d: received %s\n",getpid(),child_buf);

		write(child[1],"pong",20);
		exit(0);
	}
	//Parent
	else{	
		close(child[1]);
		close(parent[0]);
		write(parent[1],"ping",4);
	
		read(child[0],parent_buf,20);
		printf("%d: recieved %s\n",getpid(),parent_buf);
		exit(0);
	}

	printf("unexcepted error!");
	exit(-1);
}

