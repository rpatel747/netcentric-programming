/*
 * proxy.c - A Simple Sequential Web proxy
 *
 * Course Name: 14:332:456-Network Centric Programming
 * Assignment 3
 * Student Name: Rushabh Patel
 *
 * IMPORTANT: This program handles requests from a client server and acts as a proxy to the website the client wants to go to.
 * There are two versions of the program, one of them performs this task using multiple processes and the other version uses multiple threads.
 * For multiprocess: ./proxy PORT_NUMBER 1
 * For multithread: ./proxy PORT_NUMBER 2
 */

#include "csapp.h"


/*
 * Function prototypes
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, int size);
void process_handle_request(int client_to_proxy_fd, struct sockaddr_in clientadr);
void* thread_handle_request(void* arg);


struct proxy_data {
    int client_to_proxy_fd;
    struct sockaddr_in clientadr;
};

/* For mulithreaded application we need to secure writing to proxy file*/
pthread_mutex_t write_mutex;




/*
 * main - Main routine for the proxy program
 */
int main(int argc, char **argv){

    /* Check arguments */
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <port number> <type:basic,multi-process,multi-threaded>\n", argv[0]);
        exit(0);
    }

    /* Get the Port number for proxy server to run on*/
    int port_number = atoi(argv[1]);

    /* Check if we are running multi-process, mult-threaded*/
    int proxy_type = atoi(argv[2]);

    /* Create a proxy server that runs at port_number*/
    /* Create the server, and set it to receive connection requests on port port_number */
    int proxy_server_fd, client_to_proxy_fd;
    socklen_t clientlen;

    pthread_mutex_init(&write_mutex,NULL);


    if ((proxy_server_fd = Open_listenfd(port_number)) < 0)
    {
        printf("Error: Proxy server could not listen at %d\n",port_number);
    }

    /* These will be used to store the client's information*/
    struct sockaddr_in clientadr;

    while (1)
    {
        clientlen = sizeof(clientadr);
        if ((client_to_proxy_fd = Accept(proxy_server_fd, (SA *)&clientadr, &clientlen)) < 0)
        {
            printf("File Descriptor for connection could not be opened\n");
        }



        // Multi Process Version
        // Once client connection is made, call fork and pass it to the child process
        // The child process is responsible for closing the client_to_proxy_fd
        // Wait for the child process to terminate, but don't block the execution
        if (proxy_type == 1){

            pid_t current_pid = fork();

            if (current_pid < 0 ) {
                perror("Error: Child process could not be created\n");
                exit(0);
            }

            if (current_pid == 0) {
                process_handle_request(client_to_proxy_fd,clientadr);
                Close(client_to_proxy_fd);
            }

            else {
                /* Wait for the child to finish but do not suspend the current execution*/
                waitpid(current_pid,NULL,WNOHANG);
            }

        }


        // Multi Threaded Version
        // Once the client connection is made, spawn a new thread and pass it the client fd
        // Child handles the request for the client fd, and then closes the connection
        // Detach the children, and they will finish on their own
        // Add mutexes for writing to the proxy log file
        else if (proxy_type == 2) {

            struct proxy_data* data = (struct proxy_data*) malloc(sizeof(struct proxy_data));
            data->client_to_proxy_fd = client_to_proxy_fd;
            data->clientadr = clientadr;

            /* Create the thread and pass it the fd to the client and the client address */
            pthread_t thread_id;
            if (pthread_create(&thread_id,NULL,&thread_handle_request,(void*)data) != 0) {
                perror("Error: Thread could not be created\n");
                exit(0);
            }

            /* Detach the thread, allow it to finish on its own do not suspend main thread */
            pthread_detach(thread_id);

        }




    }

    /* Destroy the write mutex that was created */
    pthread_mutex_destroy(&write_mutex);

    close(proxy_server_fd);
    printf("Exiting program\n");

    exit(0);
}

/*
 * format_log_entry - Create a formatted log entry in logstring.
 *
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), and the size in bytes
 * of the response from the server (size).
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr,
                      char *uri, int size)
{
    time_t now;
    char time_str[MAXLINE];
    unsigned long host;
    unsigned char a, b, c, d;

    /* Get a formatted time string */
    now = time(NULL);
    strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

    /*
     * Convert the IP address in network byte order to dotted decimal
     * form. Note that we could have used inet_ntoa, but chose not to
     * because inet_ntoa is a Class 3 thread unsafe function that
     * returns a pointer to a static variable (Ch 13, CS:APP).
     */
    host = ntohl(sockaddr->sin_addr.s_addr);
    a = host >> 24;
    b = (host >> 16) & 0xff;
    c = (host >> 8) & 0xff;
    d = host & 0xff;

    /* Return the formatted log entry string */
    sprintf(logstring, "%s: %d.%d.%d.%d %s", time_str, a, b, c, d, uri);
}



void process_handle_request(int client_to_proxy_fd,struct sockaddr_in clientadr){


    /* Buffer holds the content of the Client's request*/
    /* Method holds the type of request the Client is trying to make*/
    /* Uri holds the uri of the site the Client wants to access */
    // char bufferb[MAXLINE], method[MAXLINE], uri[MAXLINE], temp_buffer[MAXLINE],ffinal_buffer[MAXLINE], empty[MAXLINE];
    int bytes = 0;

    /* Using CSAPP Robust Input Ouput to perform reads and writes from the socket connection */
    rio_t client_to_proxy_rio;
    rio_t proxy_to_server_rio;

    /* Default port connection is 80, unless specified in the url */
    int host_port_number = 80;
    char *tempurl;
    char *file_path;

    char* hosturl;


    /* All buffers need to allocated dynamically on heap because an error was occuring when they were allocated on the stack there was data corruption*/
   
    /* read_buffer holds the data read from the client socket connection*/
    char* read_buffer = malloc(sizeof(char) * MAXLINE);
    /* Strings are null terminated*/
    read_buffer[MAXLINE-1] = '\0';

    /* final_buffer contains the request headers in the format that will be send to the hosturl socket */
    char* final_buffer = malloc(sizeof(char) * MAXLINE);
    /* Strings are null terminated*/
    final_buffer[MAXLINE-1] = '\0';

    /* temp_buffer holds the data read from the hosturl socket */
    char* temp_buffer = malloc(sizeof(char) * MAXLINE);
    /* Strings are null terminated*/
    temp_buffer[MAXLINE-1] = '\0';

    /* uri that gets parsed from the first request header that is read */
    char* uri = malloc(sizeof(char) * MAXLINE);
    /* Strings are null terminated*/
    uri[MAXLINE-1] = '\0'; 

    /* method that gets parsed from the first request header that is read */
    char* method = malloc(sizeof(char) * MAXLINE);
    /* Strings are null terminated*/
    method[MAXLINE-1] = '\0'; 



    /* Init the  Robust IO, and read the first line from socket connection*/
    Rio_readinitb(&client_to_proxy_rio,client_to_proxy_fd);
    Rio_readlineb(&client_to_proxy_rio,read_buffer,MAXLINE);

    /* From the first line extract the request method type and the request uri */
    sscanf(read_buffer,"%s %s",method,uri);
    

    /* Our proxy only responds to GET requests from the browser*/
    if (strcmp(method, "GET") == 0){

        char* changeConnection = "Connection: close\r\n";

        /* Read all of the headers from the Request, if "Connection: keep-alive" is found then replace it with "Connection: close"*/
        /* This is because "Connection: keep-alive" stalls out the proxy if the request errors*/
        while(strcmp(read_buffer,"\r\n")){

            if(strstr(read_buffer,"Connection: keep-alive")) {
                strncat(final_buffer,changeConnection,strlen(changeConnection));
                Rio_readlineb(&client_to_proxy_rio,read_buffer,MAXLINE);
                continue;
            }
            
            /* Copy the request header from the read buffer to the final buffer */
            strncat(final_buffer,read_buffer,strlen(read_buffer));
            Rio_readlineb(&client_to_proxy_rio,read_buffer,MAXLINE);
        }

        /* Free the buffer that was used for reading the original request */
        free(read_buffer);

        /* There needs to be a \r\n from the request headers section to the rest of the request parts*/
        strncat(final_buffer,"\r\n",strlen("\r\n"));

        
        /* Need the hosturl by itself (i.e "google.com") */
        /* Remove the leading "http://", and the trailing "/..." */
        tempurl = strpbrk(uri, "/") + 2;
        if (tempurl != NULL){
            file_path = strpbrk(tempurl, "/");
        }
        
        /* Dynamically allocate the hosturl, allocating using char[] causes data corruption*/
        hosturl = malloc(sizeof(char) * strlen(tempurl));
        /* Strings are null terminated*/
        hosturl[strlen(tempurl)-1] = '\0';

        /* Isolate the hosturl from the uri */
        for(int i =0; i < (strlen(tempurl)-strlen(file_path));i++){
            hosturl[i] = tempurl[i];
        }

        /* Search the hosturl for a port number */
        char *nonp;
        char port_number_url[4];
        char *hosturl_port_start = (strpbrk(hosturl, ":"));

        /* If hosturl_port_start is NULL then the request did not specify a port number, port 80 is used*/
        if (hosturl_port_start != NULL)
        {
            strncpy(port_number_url, hosturl_port_start + 1, 4);
            if ((host_port_number = (strtol(port_number_url, &nonp, 10))) == 0)
            {
                host_port_number = 80;
            }
            hosturl[strlen(hosturl)- strlen(hosturl_port_start)] = '\0';
        }


        /* Open a socket connection to the server that the client wanted to visit*/
        int proxy_to_server_fd;
        if ((proxy_to_server_fd = open_clientfd(hosturl, host_port_number)) < 0)
        {
            perror("Socket connection to hosturl could not be made\n");
        }

        /* Write the original request message from the client to this server*/
        write(proxy_to_server_fd, final_buffer, strlen(final_buffer));

        /* Start reading the response */
        Rio_readinitb(&proxy_to_server_rio, proxy_to_server_fd);
        bytes+= Rio_readlineb(&proxy_to_server_rio, temp_buffer, MAXLINE);

        int len = 0;
        
        /* Continue reading the response headers*/
        while (strcmp(temp_buffer, "\r\n")){
            // printf("%s",temp_buffer);
            write(client_to_proxy_fd, temp_buffer, strlen(temp_buffer));
            len =  Rio_readlineb(&proxy_to_server_rio, temp_buffer, MAXLINE);
            bytes += len;
        }

        /* Response headers finished, \r\n was encountered, write that to the client*/
        write(client_to_proxy_fd, "\r\n", strlen("\r\n"));

        while ((len = rio_readnb(&proxy_to_server_rio, temp_buffer, MAXLINE)) > 0){
            bytes +=len;
            write(client_to_proxy_fd, temp_buffer, strlen(temp_buffer));
        }

        // Open file named proxy.log, if it does not exist then create it
        // FILE *file_pointer = fopen("proxy.log","a");
        int file_fd = open("proxy.log",O_WRONLY|O_APPEND);
        
        /* Create a lock on the file so other processes can't write at the same time */
        int file_lock = flock(file_fd,LOCK_EX);

        // Pass information to format_log_entry to get a formatted string
        char formattedString[MAXLINE];
        formattedString[MAXLINE-1] = '\0';
        format_log_entry(formattedString,&clientadr,uri,bytes);


        /* Allocate the write_string, this will contain the final string that will be written to the file */
        char* write_string = malloc(sizeof(char) * MAXLINE);
        write_string[MAXLINE-1] = '\0';

        /* Perform final formatting */
        sprintf(write_string,"%s %d\n",formattedString,bytes);

        /* Write the string to the file */
        write(file_fd,write_string,strlen(write_string));

        /* Free the allocated string */
        free(write_string);

        /* Unlock the lock on the so that other proccess can write */
        int file_unlock = flock(file_fd,LOCK_UN);

        /* Close the log file */
        close(file_fd);

        /* Free all of the memory that was allocated */
        free(uri);
        free(method);
        free(temp_buffer);
        free(final_buffer);
        free(hosturl);

        /* Close the connection to the end server*/
        Close(proxy_to_server_fd);
    }


}



/* Multi-threaded version of the handle_request() function */
/* Client_to_proxy_fd and clientadr are passed as arguments in struct proxy_data */
/* Uses global mutex to perform protected writes to the proxy log file */
void* thread_handle_request(void* arg) {

    /* Get the input variables from arg*/
    int client_to_proxy_fd = ((struct proxy_data*)arg)->client_to_proxy_fd;
    struct sockaddr_in clientadr = ((struct proxy_data*)arg)->clientadr;


    /* Perform same funcationality as handle_request*/
   int bytes = 0;

    rio_t client_to_proxy_rio;
    rio_t proxy_to_server_rio;

    int host_port_number = 80;
    char *tempurl;
    char *file_path;

    char* hosturl;

    /* All buffers need to allocated dynamically on heap because an error was occuring when they were allocated on the stack there was data corruption*/
    /* read_buffer holds the data read from the client socket connection*/
    char* read_buffer = malloc(sizeof(char) * MAXLINE);
    /* Strings are null terminated*/
    read_buffer[MAXLINE-1] = '\0';

    /* final_buffer contains the request headers in the format that will be send to the hosturl socket */
    char* final_buffer = malloc(sizeof(char) * MAXLINE);
    /* Strings are null terminated*/
    final_buffer[MAXLINE-1] = '\0';

    /* temp_buffer holds the data read from the hosturl socket */
    char* temp_buffer = malloc(sizeof(char) * MAXLINE);
    /* Strings are null terminated*/
    temp_buffer[MAXLINE-1] = '\0';

    /* uri that gets parsed from the first request header that is read */
    char* uri = malloc(sizeof(char) * MAXLINE);
    /* Strings are null terminated*/
    uri[MAXLINE-1] = '\0'; 

    /* method that gets parsed from the first request header that is read */
    char* method = malloc(sizeof(char) * MAXLINE);
    /* Strings are null terminated*/
    method[MAXLINE-1] = '\0'; 

    Rio_readinitb(&client_to_proxy_rio,client_to_proxy_fd);
    Rio_readlineb(&client_to_proxy_rio,read_buffer,MAXLINE);

    sscanf(read_buffer,"%s %s",method,uri);
    

    /* Our proxy only responds to GET requests from the browser*/
    if (strcmp(method, "GET") == 0){

        char* changeConnection = "Connection: close\r\n";

            while(strcmp(read_buffer,"\r\n")){

                /* Read all of the headers from the Request, if "Connection: keep-alive" is found then replace it with "Connection: close"*/
                /* This is because "Connection: keep-alive" stalls out the proxy if the request errors*/
                if(strstr(read_buffer,"Connection: keep-alive")) {
                    strncat(final_buffer,changeConnection,strlen(changeConnection));
                    Rio_readlineb(&client_to_proxy_rio,read_buffer,MAXLINE);
                    continue;
                }

                /* Copy the request header from the read buffer to the final buffer */
                strncat(final_buffer,read_buffer,strlen(read_buffer));

                /* Read the next request header from the socket*/
                Rio_readlineb(&client_to_proxy_rio,read_buffer,MAXLINE);
            }

        /* Free the buffer that was used for reading the original request */
        free(read_buffer);

        /* There needs to be a \r\n from the request headers section to the rest of the request parts*/
        strncat(final_buffer,"\r\n",strlen("\r\n"));


        /* Need the hosturl by itself (i.e "google.com") */
        /* Remove the leading "http://", and the trailing "/..." */
        tempurl = strpbrk(uri, "/") + 2;
        if (tempurl != NULL){
            file_path = strpbrk(tempurl, "/");
        }

        hosturl = malloc(sizeof(char) * strlen(tempurl));
        hosturl[strlen(tempurl)-1] = '\0';

        
        for(int i =0; i < (strlen(tempurl)-strlen(file_path));i++){
            hosturl[i] = tempurl[i];
        }


        /* Search the hosturl for a port number */
        char *nonp;
        char port_number_url[4];
        char *hosturl_port_start = (strpbrk(hosturl, ":"));

        /* If hosturl_port_start is NULL then the request did not specify a port number, port 80 is used*/
        if (hosturl_port_start != NULL)
        {
            strncpy(port_number_url, hosturl_port_start + 1, 4);
            if ((host_port_number = (strtol(port_number_url, &nonp, 10))) == 0)
            {
                host_port_number = 80;
            }
            hosturl[strlen(hosturl)- strlen(hosturl_port_start)] = '\0';
        }


        /* Open a socket connection to the server that the client wanted to visit*/
        int proxy_to_server_fd;
        if ((proxy_to_server_fd = open_clientfd(hosturl, host_port_number)) < 0)
        {
            printf("Socket connection could not be opened to the requested hosturl\n");
            exit(0);
        }

        /* Write the original request message from the client to this server*/
        write(proxy_to_server_fd, final_buffer, strlen(final_buffer));

        //printf("Opened connection to end server, writing request to server");

        /* Start reading the response */
        Rio_readinitb(&proxy_to_server_rio, proxy_to_server_fd);
        bytes+= Rio_readlineb(&proxy_to_server_rio, temp_buffer, MAXLINE);

       // printf("Response from server:\n");

        int len = 0;
        
        /* Continue reading the response headers*/
        while (strcmp(temp_buffer, "\r\n")){
            //printf("%s",temp_buffer);
            write(client_to_proxy_fd, temp_buffer, strlen(temp_buffer));
            len =  Rio_readlineb(&proxy_to_server_rio, temp_buffer, MAXLINE);
            bytes += len;
        }

        /* Response headers finished, \r\n was encountered, write that to the client*/
        write(client_to_proxy_fd, "\r\n", strlen("\r\n"));


        while ((len = rio_readnb(&proxy_to_server_rio, temp_buffer, MAXLINE)) > 0){
            bytes +=len;
            write(client_to_proxy_fd, temp_buffer, strlen(temp_buffer));
        }

        /* Get the lock to write to the proxy log file*/
        pthread_mutex_lock(&write_mutex);

        // Open file named proxy.log, if it does not exist then create it
        FILE *file_pointer = fopen("proxy.log","a");

        // Format the string
        char formattedString[MAXLINE];
        formattedString[MAXLINE-1] = '\0';
        format_log_entry(formattedString,&clientadr,uri,bytes);

        // Write the formatted string to the file
        fprintf(file_pointer,"%s %d\n",formattedString,bytes);

        // Close log file
        fclose(file_pointer);

        /* Open lock */
        pthread_mutex_unlock(&write_mutex);

        /* Free all of the memory that was allocated */
        free(uri);
        free(method);
        free(temp_buffer);
        free(final_buffer);
        free(hosturl);

        /* Close the connection to the end server*/
        Close(proxy_to_server_fd);

        /* Close client_to_proxy_fd and free data */
        free(arg);

        /* Thread is responsible for closing the connection to the client */
        Close(client_to_proxy_fd);

        /* Exit the thread */
        pthread_exit(NULL);
}
}