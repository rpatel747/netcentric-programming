#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>


// Purpose: Given the original string, generate a substring (sub) that ranges from
//          original[start] to original[end]
// int start --> starting index of substring
// int stop --> stopping index of substring
// char* original --> String that the substring will be derived from
//  char* sub --> final substring
int substring(int start, int stop, char* original, char* sub){
    int j = 0;
    for(int i = start;i <= start + stop; i++) {
        sub[j] = original[i];
        j++;
    }
    sub[j] = '\0';
    return 0;
}



int main(int argc, char **argv) {


    if (argc <= 1) {
        perror("Error: More input arguments are required, please check your input\n");
        return -1;
    }

    // Level 2 Mode: Atleast three arguments
    // Arg 1: "--systemcalls"
    // Arg 2: File Name
    // Arg 3...n: Substrings to search for

    // If the "--systemcalls" flag is provided then open,read and close must be used
    if (strcmp(argv[1],"--systemcalls") == 0) {
        
        if (argc <= 3) {
            perror("Error: More input arguments are required, please check your input again\n");
            return -1;
        }

        int num_of_substrings = argc - 2 + 2;
        int* results = (int*) malloc(sizeof(int)*num_of_substrings);
        char* file_name = argv[2];

        // Get the size of the file using the filename
        // Create a buffer with the size of the file
        struct stat st;
        stat(file_name,&st);
        char* file_content = (char*) malloc(sizeof(char) * st.st_size);


        // Open the file
        int fd = open(file_name,O_RDONLY);

        // Check if file was correctly opened, if not then return error and exit
        if(fd < 0) {
            perro("No such file or directory");
            return -1;
        }

        // Read the content of the file into the buffer
        ssize_t bytes_read = read(fd,file_content,st.st_size);

        int result_index = 0;

        for(int i = 3; i < num_of_substrings; i++) {
            
            // Get the current size of the current substring
            size_t sub_string_size = strlen(argv[i]);
            
            // Create a new substring array to hold the current substring of the file
            char* current_file_substring = (char*) malloc((sub_string_size + 1) * sizeof(char));
            
            // Number of times that the given substring is found in the file
            int substring_matches = 0;

            for(int j = 0; j < (int)st.st_size;j++){
                if((j + (int)sub_string_size) > st.st_size){
                    break;
                }
                substring(j,(int)sub_string_size - 1,file_content,current_file_substring);
                
                if(strcmp(argv[i],current_file_substring)==0) {
                    substring_matches++;
                }

            }

            results[result_index] = substring_matches;
            result_index++;

            free(current_file_substring);
        }


        // Close the file
        close(fd);

        // Print the results of search
        for(int i = 0; i < result_index;i++){
            printf("%d\n",results[i]);
        }
        
        free(results);
        free(file_content);
        
    }


    // Level 1 Mode: Atleast two arguments
    // Arg 1: File name
    // Arg 2...n: Substrings to search for

    // "--systemcalls" flag was not provided, therefore fopen,fread and fclose will be used.
    else {

        int num_of_substrings = argc - 2 + 2;
        int* results = (int*) malloc(sizeof(int)*num_of_substrings);
        char* file_name = argv[1];

        // Get the size of the file using the filename
        // Create a buffer with the size of the file
        struct stat st;
        stat(file_name,&st);
        char* file_content = (char*) malloc(sizeof(char) * st.st_size);


        // Open file
        FILE* current_file = fopen(file_name,"r");
        
        if(current_file == NULL) {
            perror("No such file or directory");
            return -1;
        }

        // Read the content of the file into the buffer
        size_t bytes_read = fread(file_content,sizeof(file_content),st.st_size,current_file);


        // Perform substring search
        int result_index = 0;


        // Perform substring search
            // For each substring
                // Iterate over the content of file
                    // Move over by 1 byte and check if the string exists
        for(int i = 2; i < num_of_substrings; i++) {
            
            // Get the current size of the current substring
            size_t sub_string_size = strlen(argv[i]);
            
            // Create a new substring array to hold the current substring of the file
            char* current_file_substring = (char*) malloc((sub_string_size + 1) * sizeof(char));
            
            // Number of times that the given substring is found in the file
            int substring_matches = 0;

            for(int j = 0; j < (int)st.st_size;j++){
                if((j + (int)sub_string_size) > st.st_size){
                    break;
                }
                substring(j,(int)sub_string_size - 1,file_content,current_file_substring);
                
                if(strcmp(argv[i],current_file_substring)==0) {
                    substring_matches++;
                }
            }

            results[result_index] = substring_matches;
            result_index++;

            free(current_file_substring);

        }

        // Close the file
        fclose(current_file);

        // Print the results
        for(int i = 0; i < result_index;i++){
            printf("%d\n",results[i]);
        }

        free(results);
        free(file_content);
    }






    return 0;
}