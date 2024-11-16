#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_PARTS 20 
#define MAX_CHARACTERS 200

typedef struct{
    char node_name[MAX_CHARACTERS];
    char command[MAX_CHARACTERS];
    int error;
}Node;

typedef struct{
    char pipe_name[MAX_CHARACTERS];
    char from[MAX_CHARACTERS];
    char to[MAX_CHARACTERS];
}Pipe;

typedef struct{
    char concatenate_name[MAX_CHARACTERS];
    int parts;
    char part_name[MAX_PARTS][MAX_CHARACTERS];
}Concatenate;

typedef struct{
    char file_node_name[MAX_CHARACTERS];
    char file_name[MAX_CHARACTERS];
    int state;
}File;

typedef struct{
    char name[MAX_CHARACTERS];
    char from[MAX_CHARACTERS];
}Stderr;


Node nodes[MAX_PARTS];
Pipe pipes[MAX_PARTS];
Concatenate concatenations[MAX_PARTS];
File files[MAX_PARTS];
Stderr stderrr[MAX_PARTS];

int pid_count=0;
int num_nodes=0, num_pipes=0, num_concats=0, num_err=0, num_files=0;

//function definitions to find index of nodes
int find_node_index(char *, int);
int find_concatenation_index(char *, int);
int find_pipe_index(char *,int);
int find_error_index(char *,int);
int find_file_index(char *,int);

//function definitions to read file
void read_file(char *,int *,int *,int *,int *,int *);

//function definition to tokenize arguments and extract word if in quote
int tokenn(char*,char*[]);
char* extract_quoted_token(char *);

//function definition to run nodes, concatenations, pipes, errors and files handling
void run_node(char *, int ,int);
void run_pipe(char *,int , int);
void run_concatenation(char *, int, int);
void run_error(char *,int , int);
void run_file(char *,int , int);

//function definition to orchestrate the flow of given file
void run_flow(char * , int , int);

// Function to find node index by name
int find_node_index(char *name, int num_nodes) {
    for (int i = 0; i < num_nodes; i++) {
        if (strcmp(nodes[i].node_name, name) == 0) {
            return i;
        }
    }
    return -1;
}

// Function to find concatenation index by name
int find_concatenation_index(char *name, int concatenation_count) {
    for (int i = 0; i < concatenation_count; i++) {
        if (strcmp(concatenations[i].concatenate_name, name) == 0) {
            return i;
        }
    }
    return -1;
}

// Function to find pipe index by name
int find_pipe_index(char *name, int pipe_count) {
    for (int i = 0; i < pipe_count; i++) {
        if (strcmp(pipes[i].pipe_name, name) == 0) {
            return i;
        }
    }
    return -1;
}

// Function to find file index by name
int find_file_index(char *name, int file_count) {
    for (int i = 0; i < file_count; i++) {
        if (strcmp(files[i].file_node_name, name) == 0) {
            return i;
        }
    }
    return -1;
}

// Function to find error index by name
int find_error_index(char *name, int error_count) {
    for (int i = 0; i < error_count; i++) {
        if (strcmp(stderrr[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

char* extract_quoted_token(char *token) {
    //extract first character to check if it is ' or ""
    char quote = token[0];
    char buffer[500];       
    int index = 0;
    token++;                

    // Copy characters from the first token after the opening quote
    while (*token != '\0') {
        if (*token == quote) {
            // If closing quote is found, terminate and return the result
            buffer[index] = '\0';
            char *temp = (char*)malloc(index);
            strcpy(temp,buffer);
            return temp;
        }
        buffer[index++] = *token;
        token++;
    }

    // Continue to the next tokens if the closing quote wasn't found in the first token
    char *next_token = strtok(NULL, " ");
    while (next_token != NULL) {
        buffer[index++] = ' ';  // Add space between tokens
        while (*next_token != '\0') {
            if (*next_token == quote) {
                // Found the closing quote, end the extraction
                buffer[index] = '\0';
                char *temp = (char*)malloc(index);
                strcpy(temp,buffer);
                return temp;  // Return the final string
            }
            buffer[index++] = *next_token;
            next_token++;
        }
        // Get the next token if still inside quotes
        next_token = strtok(NULL, " ");
    }

    // If we reach here, it means there was a missing closing quote
    perror("Unclosed quote!");
    exit(1);
}


int tokenn(char *name, char *args[]) {
    char *token = strtok(name, " ");  
    int i = 0;

    while (token != NULL) {
        //if quotes are present
        if (token[0] == '\'' || token[0] == '"') {
            args[i] = extract_quoted_token(token);
        } else {
            //copy the token directly
            args[i] = (char *)malloc(strlen(token) + 1);
            if (args[i] == NULL) {
                // Handle allocation failure
                perror("Malloc Failed!");
                exit(1);
            }
            strcpy(args[i], token);  
        }
        i++;
        token = strtok(NULL, " ");
    }

    args[i] = NULL;
    return i; 
}


void run_node(char *key,int fd_in, int fd_out) {
    int node_index = find_node_index(key,num_nodes);
    if (fd_in != STDIN_FILENO) {
            dup2(fd_in, STDIN_FILENO);
    }
    if (fd_out != STDOUT_FILENO) {
        dup2(fd_out, STDOUT_FILENO);
    }
    if (nodes[node_index].error==1) {
        dup2(fd_out, STDERR_FILENO);
    }
    char *command[10];
    tokenn(nodes[node_index].command, command);
    execvp(command[0], command);
}

void run_concatenation(char *key, int fd_in, int fd_out) {
    if (fd_out == -1) {
        fd_out = STDOUT_FILENO;
    }

    int concat_index = find_concatenation_index(key, num_concats);
    int parts = concatenations[concat_index].parts;
    int fds[parts][2];
    pid_t pids[parts];  // Array to store process IDs of child processes

    for (int i = 0; i < parts; i++) {
        if (pipe(fds[i]) == -1) {   //Open only that pipe which is required
            perror("Pipe Failed!");
            exit(1);
        }

        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            close(fds[i][0]);  
            dup2(fds[i][1], STDOUT_FILENO);  
            close(fds[i][1]); 

            run_flow(concatenations[concat_index].part_name[i], fd_in, STDOUT_FILENO);
            exit(0);
        } else if (pid > 0) {
            // Parent process
            pids[i] = pid;
            close(fds[i][1]);
        } else {
            perror("Fork Unsuccessful!");
            exit(1);
        }
    }

    // Wait for all child processes to finish
    for (int i = 0; i < parts; i++) {
        waitpid(pids[i], NULL, 0);
    }

    // Read from pipes and write the combined output
    char buff[5000];
    for (int i = 0; i < parts; i++) {
        int read_bytes;
        while ((read_bytes = read(fds[i][0], buff, sizeof(buff) - 1)) > 0) {
            int write_bytes = 0;
            while (write_bytes < read_bytes) {
                int written = write(fd_out, buff + write_bytes, read_bytes - write_bytes);
                if (written == -1) {
                    perror("Write Failed!");
                    exit(1);
                }
                write_bytes += written;
            }
        }
        if (read_bytes == -1) {
            perror("Read Failed!");
            exit(1);
        }
        close(fds[i][0]);
    }
}

void run_pipe(char *key,int fd_in, int fd_out){
    // printf("Inside Pipe\n");
    int fds[2];
    if (pipe(fds) == -1) {
        perror("Fork Unsuccessful!");
        exit(1);
    }

    int index = find_pipe_index(key,num_pipes);
    pid_t pid1 = fork();
    if (pid1 == 0) {
        // pipe from command
        if (fd_in != STDIN_FILENO) {
            dup2(fd_in, STDIN_FILENO);
            close(fd_in);
        }
        dup2(fds[1], STDOUT_FILENO);
        close(fds[0]);
        close(fds[1]);

        run_flow(pipes[index].from, STDIN_FILENO, STDOUT_FILENO);
        exit(0);
    } else if (pid1 < 0) {
        perror("Fork Unsuccessful!");
        exit(1);
    }

    pid_t pid2 = fork();
    if (pid2 == 0) {
        // pipe to command
        dup2(fds[0], STDIN_FILENO);
        close(fds[1]);
        close(fds[0]);
        if (fd_out != STDOUT_FILENO) {
            dup2(fd_out, STDOUT_FILENO);
            close(fd_out);
        }
        if(find_file_index(pipes[index].to,num_files)!=-1){
            int file_index = find_file_index(pipes[index].to,num_files);
            files[file_index].state=1;
        }
        run_flow(pipes[index].to, STDIN_FILENO, STDOUT_FILENO);
        exit(0);
    } else if (pid2 < 0) {
        perror("Fork Unsuccessful!");
        exit(1);
    }

    // Parent process
    if (fd_in != STDIN_FILENO) {
        close(fd_in);
    }
    if (fd_out != STDOUT_FILENO) {
        close(fd_out);
    }
    close(fds[0]);
    close(fds[1]);

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

void run_file(char *key, int fd_in, int fd_out) {
    // printf("Inside File\n");
    // printf("File Key: %s\n",key);

    int file_index = find_file_index(key,num_files);
    if(files[file_index].state==0){
        execlp("cat","cat",files[file_index].file_name);
    }
    else{
        int bytes;
        char buff[5000];

        // Open the file for writing (create it if it doesn't exist)
        int fd = open(files[file_index].file_name, O_CREAT | O_WRONLY, 0644);
        if (fd == -1) {
            perror("Open Falied!");
            exit(1);
        }

        // Read from input_fd and write to the file
        while ((bytes = read(fd_in, buff, sizeof(buff))) > 0) {
            int writee = 0;
            while (writee < bytes) {
                int written = write(fd, buff + writee, bytes - writee);
                if (written == -1) {
                    perror("Write Failed!");
                    close(fd);  // Make sure to close the file descriptor
                    exit(1);
                }
                writee += written;
            }
        }

        if (bytes == -1) {
            perror("Read failed!");
        }

        close(fd);  // Close the file descriptor after writing is done
    }
}

void run_error(char *key,int fd_in, int fd_out){
    // printf("Inside error!\n");
    int err_index = find_error_index(key,num_err);
    char *node_name = stderrr[err_index].from;

    if(find_node_index(node_name,num_nodes)!=-1){
        int node_index = find_node_index(node_name,num_nodes);
        nodes[node_index].error=1;
        run_flow(node_name,fd_in,fd_out);
        nodes[node_index].error=0;
    }
    else{
        perror("Node not found!\n");
        exit(1);
    }
}

void run_flow(char *key , int fd_in, int fd_out){
    //check if it is a node
    if(find_node_index(key,num_nodes)!=-1){
        run_node(key,fd_in,fd_out);
    }
    //if it is a concatenation node
    else if(find_concatenation_index(key,num_concats)!=-1){
        run_concatenation(key, fd_in, fd_out);
    }
    //if it is a pipe
    else if(find_pipe_index(key,num_pipes)!=-1){
        run_pipe(key,fd_in,fd_out);
    }
    //EXTRA CREDITS - error handling
    else if(find_error_index(key,num_err)!=-1){
        // printf("Inside Error!\n");
        run_error(key, fd_in, fd_out);
    }
    else if(find_file_index(key,num_files)!=-1){
        // printf("Inside File1!\n");
        run_file(key,fd_in,fd_out);
    }
    else{
        fprintf(stderr, "unknown action : %s\n", key);
        exit(1);
    }
    close(fd_in);
    close(fd_out);

}

void read_file(char *filename,int *nc,int *pc,int *cc,int *fc, int *ec){
    //open the file
    FILE *file_content=fopen(filename,"r");
    if(!file_content){
        perror("Error opening file");
        exit(1);
    }
    int node_count = 0, pipe_count = 0, concat_count = 0, file_count=0, err_count=0;

    char line[500];
    //read each line
    while(fgets(line,sizeof(line),file_content)){
        //if node is present in line
        if(strncmp(line,"node=",5) == 0){
            sscanf(line, "node=%s", nodes[node_count].node_name);
            fgets(line, sizeof(line), file_content);
            sscanf(line, "command=%[^\n]", nodes[node_count].command);
            node_count++;
        }
        //if concatenation is present
        else if(strncmp(line,"concatenate=",12) == 0){
            sscanf(line,"concatenate=%s",concatenations[concat_count].concatenate_name);
            fgets(line,sizeof(line),file_content);
            sscanf(line,"parts=%d",&concatenations[concat_count].parts);
            for(int i=0;i<concatenations[concat_count].parts;i++){
                fgets(line,sizeof(line),file_content);
                sscanf(line, "part_%d=%s",&i,concatenations[concat_count].part_name[i]);
            }
            concat_count++;
        }
        //if pipe is present
        else if(strncmp(line,"pipe=",5) == 0){
            sscanf(line,"pipe=%s",pipes[pipe_count].pipe_name);
            fgets(line,sizeof(line),file_content);
            sscanf(line,"from=%s",pipes[pipe_count].from);
            fgets(line,sizeof(line),file_content);
            sscanf(line,"to=%s",pipes[pipe_count].to);
            pipe_count++;
        }
        //if error is present
        else if(strncmp(line,"stderr=",7) == 0){
            sscanf(line,"stderr=%s",stderrr[num_err].name);
            fgets(line,sizeof(line),file_content);
            sscanf(line,"from=%s",stderrr[num_err].from);
            err_count++;
        }
        //file is found
        else if(strncmp(line,"file=",5) == 0){
            sscanf(line,"file=%s",files[num_files].file_node_name);
            fgets(line,sizeof(line),file_content);
            sscanf(line,"name=%s",files[num_files].file_name);
            //by default read state
            files[num_files].state=0;
            file_count++;
        }
    }
    fclose(file_content);
    *nc=node_count;
    *pc=pipe_count;
    *cc=concat_count;
    *fc=file_count;
    *ec=err_count;
}

int main(int argc, char *argv[]){
    if(argc!=3){
        fprintf(stderr, "Usage: %s <flowfile> <action>\n", argv[0]);
        return 1;
    }

    read_file(argv[1],&num_nodes,&num_pipes,&num_concats,&num_files,&num_err);
    run_flow(argv[2],STDIN_FILENO,STDOUT_FILENO);
    //wait for process childs to stop
    int running=1;
    while(running){
        if(wait(NULL)<=0){
            running=0;
        }
    }
    return 0;

}