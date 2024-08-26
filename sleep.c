#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[])
{
        if (argc == 1){
                printf("Please enter the parameters!\n");
        }
        else{
                int waiting_time = atoi(argv[1]);

                sleep(waiting_time);
        }

        exit(0);
}
