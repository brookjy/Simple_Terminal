/*
 * main.c
 *
 * A simple program to illustrate the use of the GNU Readline library
 */

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>

//Define variables and set the state constant:
/*------------------------------------------*/
#define Max_directory 256  /*Maxmum directory*/
#define Max_param 15   /*Maximum length of command*/
#define Max_background 5   /*Maximum numbers of background*/
/* Background Job states */
#define ER 0 /* ERROR(UNDEFIND) */
#define BG 1   /* running in background */
#define ST 2   /* stopped */

struct bg_jobs {              /* The job struct */
    pid_t pid;              /* job PID */
    int job_number;                /* job ID [1, 2, ...] */
    int state;              /* UNDEF, BG, FG, or ST */
    char direct[Max_directory];  /* command line */
};
struct bg_jobs jobs[Max_background]; /* The job list */
int counter=0;  /* Counting the number of background jobs */
int correct=0;
int killz;

/*      End of Global Variables             */
/*------------------------------------------*/

/* Clear the entries in a job struct */
void clearjob(struct bg_jobs *job) {
    job->pid = 0;
    job->job_number= 0;
    job->state = ER;
    job->direct[0] = '\0';
}

/* Swap the job with another job, swap job1 with job2*/
void job_swap(struct bg_jobs *job, int job1, int job2){
	job[job1].pid = job[job2].pid;
	job[job1].job_number = job[job2].job_number;
	job[job1].state = job[job2].state;
	strcpy(jobs[job1].direct,jobs[job2].direct);
}

/* Update the list of the job list, and when a job is removed, 
* this method remove it from the list.
*/
void update_list(struct bg_jobs *job, int job_number){
	for (int i = job_number; i < Max_background-1; i++) {
 		if (jobs[i+1].pid != 0){
 			job_swap(jobs,i,i+1);
 			clearjob(&jobs[i+1]);
 			printf("%d\n", job[i].pid);
 		}
	}
}

/* Get the pid of the job. (From the struct jobs)*/
pid_t getPid(struct bg_jobs *job, int job_number){
    for (int i = 0; i < Max_background; i++) {
        if (jobs[i].job_number == job_number){
            return jobs[i].pid;
        }
    }
    return 0;
    
}

/* Update the state and the state change if the stop or continuous signal
*  has send.
*/
void update_state(struct bg_jobs *job, int job_number, int state){
	for (int i = 0; i < Max_background; i++) {
        if (jobs[i].job_number == job_number){
            job[i].state = state;
        }
    }
}


/* Add a job to the job list */
int addjob(struct bg_jobs *jobs, pid_t pid, int state, char *direct,int job_number)
{
    int i;
    if (pid < 1)
        return 0;
    
    for (i = 0; i < Max_background; i++) {
        if (jobs[i].pid == 0) {
            jobs[i].pid = pid;
            jobs[i].state = state;
            jobs[i].job_number = job_number;
            strcpy(jobs[i].direct, direct);
            return 1;
        }
    }
    counter++;
    printf("Reach the maximum jobs.\n");
    return 0;
}

/* Delete a job from the job list by its pid number*/
int deletejob(struct bg_jobs *jobs, pid_t pid)
{
    int i;
    counter--;

    if (pid < 1)
        return 0;
    
    for (i = 0; i < Max_background; i++) {
        if (jobs[i].pid == pid) {
            clearjob(&jobs[i]);
            return 1;
        }
    }
    return 0;
}

/* Print the job list */
void listjobs(struct bg_jobs *jobs)
{
    int i;
    for (i = 0; i < Max_background; i++) {
        if (jobs[i].pid != 0) {
            //printf("[%d] (%d) ", jobs[i].job_number, jobs[i].pid);
            switch (jobs[i].state) {
                case BG:
                    printf("%d [R]: %s \n", i, jobs[i].direct);/*Display the running*/
                    break;
                case ST:
                    printf("%d [S]: %s \n", i, jobs[i].direct);/*Display the stopping*/
                    break;
                default:
                    printf("listjobs: Internal error: job[%d].state=%d ",
                           i, jobs[i].state);
            }
        } 
    }
    printf("Total Background jobs: %d\n", counter);
}

/*          End of JObs Methods             */
/*------------------------------------------*/


// Split cmd into array of parameters
void parseCmd(char * cmd, char ** params)
{
    for(int i = 0; i < Max_param; i++) {
        params[i] = strsep(&cmd, " ");
        if(params[i] == NULL) break;
    }
}


int changeDirectory(char** params){
    // If we write no path (only 'cd'), then go to the home directory
    if (params[1] == NULL) {
        chdir(getenv("HOME"));
        return 1;
    }
    // Else we change the directory to the one specified by the
    // argument, if possible
    else{
        chdir(params[1]);
        //if (chdir(params[1]) == -1) {
        //    perror("Directory Error: ");
        //    return -1;
        return -1;
        
    }
    return 0;
}

int bg_jobs(char** params){
    

    pid_t fpid = fork();
    int status;
    *params= *params+3;
    char curr_dir[Max_directory];
    int n;

    if (fpid == -1) {
        printf("error in fork!\n");// fork error
    }
    else if (fpid == 0) //in child process
    {
        execvp(params[1],params );
        exit(0);
        
    }else{ // in the parent process
    	waitpid(fpid, &status, WNOHANG);
    	printf("%d\n",WIFEXITED(status));
        getcwd(curr_dir, Max_directory);
        strcat(curr_dir, "/");
        for(n=0;n<Max_param;n++){
        	if (params[n+1]==NULL){
        		break;
        	}
        }
        strcat(curr_dir, params[n]);
        
        addjob(jobs, fpid, BG, curr_dir, counter); /*Add jobs to the job lists */
        counter++;
        //waitpid(fpid, &status, WNOHANG);
    }
    
    return 0;
   
}

void executeCmd(char** params)
{
    
    // Start a Fork process
    pid_t pid = fork();
    
    // Error
    if (pid == -1) {
        char* error = strerror(errno);
        printf("fork: %s\n", error);
        
        
    }
    
    // Child process
    else if (pid == 0) {
        // Execute command
        execvp(*params, params);
        
        

        
    // Parent process
    }else {
        // Wait for child process to finish
        int childStatus;
        waitpid(pid, &childStatus, 0);
        
    }
}

/*              Into Main                   */
/*------------------------------------------*/

int main ( void )
{
    char* params[Max_param];
    char directory[Max_directory];
    pid_t kpid;
    
    while(1) {
        getcwd(directory,Max_directory);
        // Read command from standard input, exit on Ctrl+D
        printf("%s",directory);
        char *cmd = readline (">");
        int nums = 0;
        
        // Remove trailing newline character, if any
        if(cmd[strlen(cmd)-1] == '\n') {
            cmd[strlen(cmd)-1] = '\0';
        }
       
        // Split cmd into array of parameters
        parseCmd(cmd, params);
        
        // Exit?
        if(strcmp(params[0], "exit") == 0) {
            break;
        }
        
        /*Change directory if any: $cd .. */
        if (strcmp(params[0],"cd")==0){
            changeDirectory(params);
        }

        /*Run back ground jobs if any: $bg xxxxxx*/
        else if (strcmp(params[0],"bg")==0){
            if (bg_jobs(params) == -1){
            	printf("Wrong command\n");
            	*params=NULL;
            }
        }

        /*Print the list of bg jobs if any: $bglist */
        else if (strcmp(params[0],"bglist")==0){
            printf("List of jobs:\n");
            listjobs(jobs);
        }

        /*Terminate a process if any: $bgkill job_number*/
        else if (strcmp(params[0],"bgkill")==0){
            //kpid = getPid(jobs, (int)params[1]);
            nums = atoi(params[1]);
            printf("%d\n",nums);
            kpid = getPid(jobs,nums);
            printf("%d\n", kpid );
            if (kpid!=0){
            	kill(kpid, SIGKILL);
            	printf("The %d process is terminated.\n", kpid);
            	deletejob(jobs,kpid);
            	update_list(jobs,nums);
            }
 
         /*Stop a process if any: $stop job_number*/            
        }else if (strcmp(params[0],"stop")==0){
            nums = atoi(params[1]);
            printf("%d\n",nums);
            kpid = getPid(jobs,nums);
            printf("%d\n", kpid );
            if (kpid!=0){
            	kill(kpid, SIGSTOP);
            	printf("The %d process is stoped.\n", kpid);
 				update_state(jobs,nums,ST);
            }

        /*Start a process if any: $start job_number*/
        }else if (strcmp(params[0],"start")==0){
            nums = atoi(params[1]);
            printf("%d\n",nums);
            kpid = getPid(jobs,nums);
            printf("%d\n", kpid );
            if (kpid!=0){
            	kill(kpid, SIGCONT);
            	printf("The %d process is started.\n", kpid);
 				update_state(jobs,nums,BG);
            }
        
        }else{
        
        
        //Execute command;
        executeCmd(params);
                    
        free(cmd);
        }  
    }
}