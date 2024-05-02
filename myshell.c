/*
Implementation of a stripped-down command-line shell
Author: Kameren Jouhal
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/times.h>

/*
Executes a command
*/
void execute(char **args, int count) {
        char delim[] = "<>"; // Delimeter for the redirect command
        char special_chars[100] = ""; // String to hold the specific redirect
        int num = 0; // Count for special_chars
        char string[100]; // String to hold arguments in
        char *file; // File string
        FILE *opened_file = NULL; // File pointer
        int status; // Status int for wait
        int return_value; // Return value for functions

        // Loop through the arguments and test if any of them have redirect
        // commands in them
        for(int i = 0; i < count; ++i) {
                // Copy the argument into string
                strcpy(string, args[i]);

                // Loop through the characters in string
                for(int j = 0; j < strlen(string); ++j) {
                        // If the character is a redirect character, add it to
                        // special_chars and increment num
                        if(string[j] == '<' || string[j] == '>') {
                                special_chars[num] = string[j];
                                ++num;
                        }
                }

                // If the count for special_chars is greater than 0
                if(num > 0) {

                        // Set the last character to the null terminator
                        special_chars[num] = '\0';

                        // If the length of the string is greater than 2,
                        // that means the user did not put space between the
                        // redirect symbol and the file
                        if(strlen(string) > 2) {

                                // If it is all in one string
                                if(count == 1) {
                                        // Tokenize the string and put the
                                        // command in args[0]
                                        file = strtok(args[i], delim);
                                        args[0] = file;
                                        args[1] = NULL;

                                        // Tokenize again to get the file
                                        file = strtok(NULL, delim);
                                }
                                // Otherwise
                                else {
                                       // Tokenize the string as such
                                        file = strtok(args[i], delim);
                                        args[i] = NULL;
                                }
                        }
                        // Otherwise
                        else {

                                // The next argument is the file
                                args[i] = NULL;
                                file = args[++i];
                                args[i] = NULL;
                        }

                        // Break out of the loop
                        break;
                }
        }

        // If special_characters is not empty
        if(num > 0) {

                // If the command is an output write redirect, redirect the
                // output of stdout to the file with the mode 'w'
                if(strcmp(special_chars, ">") == 0) {
                        opened_file = freopen(file, "w", stdout);
                }

                // If the command is an output append redirect, redirect the
                // output of stdout to the file with the mode 'a'
                if(strcmp(special_chars, ">>") == 0) {
                        opened_file = freopen(file, "a", stdout);
                }

                // If the command is an input redirect, redirect the input
                // of stdin to the program from the file with the mode 'r'
                if(strcmp(special_chars, "<") == 0) {
                        opened_file = freopen(file, "r", stdin);

                        // If the file cannot be opened, display an error
                        // message and return
                        if(opened_file == NULL) {
                                puts("redirect error: file does not exist");
                                return;
                        }
                }

                // Otherwise, the command is invalid, display an error message
                // and return
                if(strcmp(special_chars, ">") != 0 && strcmp(special_chars, ">>") != 0 && strcmp(special_chars, "<") != 0) {
                        puts("error: invalid redirect operation");
                        return;
                }
        }

        // Create a fork. If the current part of the fork is the child
        if(fork() == 0) {
                // Execute the arguments
                return_value = execvp(args[0], args);

                // If the function returns -1
                if(return_value == -1) {
                        // Close the file if it is opened
                        if(opened_file != NULL) {
                                fclose(opened_file);
                        }
                        // Set stdout and stdin back to normal, display an
                        // error message, and exit the fork
                        freopen("/dev/tty", "r", stdin);
                        freopen("/dev/tty", "w", stdout);
                        puts("error: invalid command");
                        exit(1);
                }
        }
        // If it is the parent part of the fork
        else {
                // Wait for the child to finish executing
                wait(&status);

                // If the file is still open, close it
                if(opened_file != NULL) {
                        fclose(opened_file);
                }
        }
}

int main() {
        char space[] = " \t\n"; // Space delimiters
        char command[100]; // Command input
        char *args[101]; // Arguments
        int count; // Count for the arguments
        char *tokens; // Pointer for the tokens
        int length; // Length of the command string
        int return_value; // Return value for the functions
        double user_time; // User time for times function
        double system_time; // System time for the times function

        while(1) {
                // Set stdout and stdin back to normal
                freopen("/dev/tty", "r", stdin);
                freopen("/dev/tty", "w", stdout);

                // Input command
                fputs("% ", stdout);
                fgets(command, 100, stdin);

                // Get the length of the command
                length = strlen(command);

                // If nothing was inputted, continue
                if(length-1 == 0) {
                        continue;
                }

                // Set the \n to the null terminator
                command[length - 1] = '\0';

                // Tokenize the command string
                tokens = strtok(command, space);

                // Set the arguments count to 0
                count = 0;

                // Put all the arguments into an array
                while(tokens != NULL) {
                        args[count] = tokens;

                        ++count;

                        tokens = strtok(NULL, space);
                }

                args[count] = NULL;

                // If the commnad is exit, exit the shell
                if(strcmp(args[0], "exit") == 0) {
                        puts("exiting the shell");
                        exit(0);
                }

                // If the command is cd, change the directory
                if(strcmp(args[0], "cd") == 0) {

                        // If there aren't enough arguments, display
                        // an error message and continue
                        if(count < 2) {
                            puts("cd: missing argument");
                            continue;
                        }

                        return_value = chdir(args[1]);

                        // If the return value is -1, display an
                        // error message
                        if(return_value == -1) {
                                puts("cd: directory does not exist");
                        }

                        continue;
                }

                // If the command is time
                if(strcmp(args[0], "time") == 0) {

                        // If there aren't enough arguments, display
                        // an error message and continue
                        if(count < 2) {
                            puts("time: missing command");
                            continue;
                        }

                        // Declare start and end structs for the times function
                        struct tms start;
                        struct tms end;

                        // Call the times function to start
                        return_value = times(&start);

                        // If the return value is -1, times failed
                        if(return_value == -1) {
                                puts("times: failed");
                                continue;
                        }

                        // Execute the arguments minus the times part
                        execute(args + 1, count-1);

                        // When it is done executing, call times function to
                        // end
                        return_value = times(&end);

                        // If the return value is -1, times failed
                        if(return_value == -1) {
                                puts("times: failed");
                                continue;
                        }

                        // Calculate the user and system times
                        user_time = (double)(end.tms_cutime - start.tms_cutime) / sysconf(_SC_CLK_TCK);
                        system_time = (double)(end.tms_cstime - start.tms_cstime) / sysconf(_SC_CLK_TCK);

                        // Display the user and system times
                        printf("User time: %.5fs\nSystem time: %.5fs\n", user_time, system_time);

                        // Continue once this is done
                        continue;
                }

                // If there are no internal shell commands, execute the
                // arguments
                execute(args, count);
        }

        // Return once exit has been called
        return 0;
}
