#include "kernel/types.h"
#include "user/user.h"

int main(int agrc, char *agrv[])
{
        int pid;
        int parent[2];
        int child[2];
        char buf[20] = {0};

        pipe(child);
        pipe(parent);

        //Child
        if((pid = fork()) == 0){
                close(parent[1]);
                read(parent[0],buf,4);
                printf("%d: received %s\n",getpid(),buf);

                close(child[0]);
                write(child[1],"pong",sizeof(buf));
                exit(0);
        }
        //Parent
        else{
                close(parent[0]);
                write(parent[1],"ping",4);

                close(child[1]);
                read(child[0],buf,sizeof(buf));
                printf("%d: recieved %s\n",getpid(),buf);
                exit(0);
        }

        printf("unexcepted error!");
        exit(-1);
}
