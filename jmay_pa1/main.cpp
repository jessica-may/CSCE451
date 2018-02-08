/*
 * Copyright (c) 2018, Yutaka Tsutano
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#include <iostream>
#include <string>
#include <vector>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include "command.hpp"
#include "parser.hpp"

//handles redirecting to files
int execProc(shell_command *command){
    int fd[2];
    static int prevpipe;
    if((*command).cout_mode == ostream_mode::pipe){
      pipe(fd); 
       
    }
    pid_t cpid = fork();
    int status;
                
    if (cpid < 0){ /*Rule 2: exit if fork failed */
        fprintf(stderr, "Fork Failed \n" );
        exit(EXIT_FAILURE);
    }
    else if (cpid == 0){ 
        int write = 1;
        if((*command).cin_mode == istream_mode::pipe){
            dup2(prevpipe, STDIN_FILENO);
            close(prevpipe);
        }
        if((*command).cout_mode == ostream_mode::pipe){
            dup2(fd[write], STDOUT_FILENO);
            close(fd[write]);    
        }
        if((*command).cin_mode == istream_mode::file){ //if its inputing from a file
            int in_file= dup2(fileno(fopen((*command).cin_file.c_str(), "r")), STDIN_FILENO);
            if(in_file < 0){
                exit(EXIT_FAILURE);
            }
            
        }
        else if((*command).cout_mode == ostream_mode::file){
            int out_file = dup2(fileno(fopen((*command).cout_file.c_str(), "w")), STDOUT_FILENO);
            if(out_file < 0){
                exit(EXIT_FAILURE);
            }
        }
        else if((*command).cout_mode == ostream_mode::append){
            int append_file = dup2(fileno(fopen((*command).cout_file.c_str(), "a")), STDOUT_FILENO);
            if(append_file < 0){
                exit(EXIT_FAILURE);
            }
        }
        char **args;
        int args_size = (*command).args.size() + 2;
        args = new char*[args_size];
        args[0] = (char *)((*command).cmd.c_str());
        for(int i = 0; i < (int)args_size - 2; i++){
            args[i+1] = (char *)(*command).args[i].c_str();
        }
        args[args_size - 1] = NULL;
        execvp((*command).cmd.c_str(), args);
        fprintf(stderr, "Exec Failed \n");
        exit(EXIT_FAILURE);
    }

    else {
        if((*command).cout_mode != ostream_mode::pipe){
            waitpid(cpid, &status, 0);
        }
        if((*command).cin_mode == istream_mode::pipe){
            close(prevpipe);
        }
        if((*command).cout_mode == ostream_mode::pipe){
            prevpipe = fd[0];
            close(fd[1]);
        }
        prevpipe = fd[0];
       // printf("Child caught \n");
    }

    // = cpid;
    return status;
}

int main(int argv_size, const char** argv)
{
    int status;
    std::string input_line;
    int print_osh = 1;
    for(int i = 0; i < argv_size; i++){
        if(strcmp(argv[i], "-t")){
            print_osh = 0;
        }
    }
    for (;;) {
        
        // Print the prompt.
        if(print_osh == 1){
            std::cout << "osh> " << std::flush;
        }
        
        
        // Read a single line.
        if (!std::getline(std::cin, input_line) || input_line == "exit") {
            break;
        }

        try {
            // Parse the input line.
            std::vector<shell_command> shell_commands
                = parse_command_string(input_line);
            //int prevpipe;

        for(uint i = 0; i < shell_commands.size(); i++){
                status = execProc(&(shell_commands[i]));
                //int pid;
                //break if the line contains an && and the first statement doesnt succeed
                if(shell_commands[i].next_mode == next_command_mode::success){
                    if(status != 0){
                        break;
                    }
                }
                //break if the line contains an || and the first statement succeeds (no need to check the second statement)
                if(shell_commands[i].next_mode == next_command_mode::fail){
                    if(status == 0){
                        break;
                    }
                }
                
            }
            
            //waitpid(pid, &status, 0);
            //Print the list of commands.
        /* std::cout << "-------------------------\n";
            for (const auto& cmd : shell_commands) {
                std::cout << cmd;
                std::cout << "-------------------------\n";
            } */
            
        }
        catch (const std::runtime_error& e) {
            std::cout << "osh: " << e.what() << "\n";
        } 
    }
    std::cout << std::endl;
}
