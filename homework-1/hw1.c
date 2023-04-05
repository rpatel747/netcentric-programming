#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

int main(){

    /* The two different child processes need to communicate with the parent process*/
    /* To perform this IPC, this program uses pipes*/
    int pipes[2];
    pipe(pipes);


    int final_sum = 0;

    pid_t first_pid;
    int first_status;
    int second_status;

    // Error occurred
    if((first_pid=fork()) <0){
        perror("First child process creation failed\n");
        return -1;
    }

    /* Child Process #1 handles the summation from 1....50*/
    /* It calculates the result and writes it to the pipe */
    if(first_pid == 0){
        int sum = 0;
        for(int i= 1; i<=50;i++){
            sum += i;
        }

        write(pipes[1],&sum,sizeof(sum));
        return sum;
    }

    /* This branch is executed by the parent process*/
    /* Parent will read the sum of 1...50 from the pipe generated by the first child*/
    /* Parent will then create another child process, this child process will calculate the sum from 51....100 */
    else{
        /* Wait for the first child process to finish before creating the second child process*/
        wait(&first_status);

        if (first_status == -1){
            perror("Error occurred, could not wait for first child: \n");
            exit(1);
        }


        read(pipes[0],&final_sum,sizeof(final_sum));


        /* Create the new child process and have it sym from 51.....100*/
        pid_t second_pid;

        if ((second_pid = fork()) < 0){
            perror("Second Child Process creation failed \n");
            return -1;
        }


        /* Check if we are in the child process */
        if(second_pid == 0){
            int sum = 0;
            for(int i= 51; i<=100;i++){
                sum += i;
            }
            write(pipes[1],&sum,sizeof(sum));
            return sum;
        }

        else{
            /* We are in the parent process, wait for the child and then read from the pipes */
            wait(&second_status);

            if (second_status == -1){
                perror("Error occurred, could not wait for second child: \n");
                exit(1);
            }

            int temp_sum = 0;
            read(pipes[0],&temp_sum,sizeof(temp_sum));
            final_sum += temp_sum;

            printf("%d\n",final_sum);
        }
    }

    /* Close the pipes before exiting*/
    close(pipes[0]);
    close(pipes[1]);

    return 0;
}