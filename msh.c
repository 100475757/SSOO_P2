//P2-SSOO-23/24

//  MSH main file
// Write your msh source code here

//#include <parser.h>
#include <stddef.h>         /* NULL */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_COMMANDS 8 

int history_size = 20;
struct command {
    int num_commands;
    int *args;
    char ***argvv;
    char filev[3][64];
    int in_background;
};
struct command * history;
int head = 0;
int tail = 0;
int n_elem = 0;

void free_command(struct command *cmd) {
    if((*cmd).argvv != NULL) {
        char **argv;
        for (int i = 0; (*cmd).argvv[i] != NULL; i++) {
            for (argv = (*cmd).argvv[i]; argv && *argv; argv++) {
                if(*argv) {
                    free(*argv);
                    *argv = NULL;
                }
            }
            free((*cmd).argvv[i]);
        }
        free((*cmd).argvv);
    }
    free((*cmd).args);
}

void store_command(char ***argvv, char filev[3][64], int in_background, struct command* cmd) {
    int num_commands = 0;
    while(argvv[num_commands] != NULL) {
        num_commands++;
    }

    for(int f = 0; f < 3; f++) {
        if(strcmp(filev[f], "0") != 0) {
            strcpy((*cmd).filev[f], filev[f]);
        } else {
            strcpy((*cmd).filev[f], "0");
        }
    }

    (*cmd).in_background = in_background;
    (*cmd).num_commands = num_commands;
    (*cmd).argvv = (char ***) calloc((num_commands + 1), sizeof(char **));
    (*cmd).args = (int*) calloc(num_commands, sizeof(int));

    for(int i = 0; i < num_commands; i++) {
        int args = 0;
        while(argvv[i][args] != NULL) {
            args++;
        }
        (*cmd).args[i] = args;
        (*cmd).argvv[i] = (char **) calloc((args + 1), sizeof(char *));
        for(int j = 0; j < args; j++) {
            (*cmd).argvv[i][j] = (char *) calloc(strlen(argvv[i][j]) + 1, sizeof(char));
            strcpy((*cmd).argvv[i][j], argvv[i][j]);
        }
        (*cmd).argvv[i][args] = NULL;
    }
    (*cmd).argvv[num_commands] = NULL;
}

void siginthandler(int param) {
    printf("****  Exiting MSH **** \n");
    for (int i = 0; i < n_elem; i++) {
        free_command(&history[(head + i) % history_size]);
    }
    free(history);
    exit(0);
}

// files in case of redirection
char filev[3][64];

//to store the execvp second parameter
char *argv_execvp[8];

int mycalc(char *argv[]);

void getCompleteCommand(char ***argvv, int num_command) {
    for(int j = 0; j < 8; j++) {
        argv_execvp[j] = NULL;
    }
    for(int i = 0; argvv[num_command][i] != NULL; i++) {
        argv_execvp[i] = argvv[num_command][i];
    }
}

void myhistory() {
    int current = head;
    for (int i = 0; i < n_elem; i++) {
        fprintf(stderr, "%d: ", i);
        for (int j = 0; j < history[current].num_commands; j++) {
            for (int k = 0; k < history[current].args[j]; k++) {
                fprintf(stderr, "%s ", history[current].argvv[j][k]);
            }
            if (j < history[current].num_commands - 1) {
                fprintf(stderr, "| ");
            }
        }
        if (strcmp(history[current].filev[0], "0") != 0) {
            fprintf(stderr, "< %s ", history[current].filev[0]);
        }
        if (strcmp(history[current].filev[1], "0") != 0) {
            fprintf(stderr, "> %s ", history[current].filev[1]);
        }
        if (strcmp(history[current].filev[2], "0") != 0) {
            fprintf(stderr, "2> %s ", history[current].filev[2]);
        }
        if (history[current].in_background) {
            fprintf(stderr, "&");
        }
        fprintf(stderr, "\n");
        current = (current + 1) % history_size;
    }
}

int main(int argc, char* argv[]) {
    int end = 0;
    int executed_cmd_lines = -1;
    char *cmd_line = NULL;
    char *cmd_lines[10];

    if (!isatty(STDIN_FILENO)) {
        cmd_line = (char*)malloc(100);
        while (scanf(" %[^\n]", cmd_line) != EOF) {
            if(strlen(cmd_line) <= 0) return 0;
            cmd_lines[end] = (char*)malloc(strlen(cmd_line) + 1);
            strcpy(cmd_lines[end], cmd_line);
            end++;
            fflush(stdin);
            fflush(stdout);
        }
    }

    char ***argvv = NULL;
    history = (struct command*) malloc(history_size * sizeof(struct command));
    int run_history = 0;

    while (1) {
        int status = 0;
        int command_counter = 0;
        int in_background = 0;
        signal(SIGINT, siginthandler);

        if (run_history) {
            run_history = 0;
        } else {
            write(STDERR_FILENO, "MSH>>", strlen("MSH>>"));
            executed_cmd_lines++;
            if (end != 0 && executed_cmd_lines < end) {
                command_counter = read_command_correction(&argvv, filev, &in_background, cmd_lines[executed_cmd_lines]);
            } else if (end != 0 && executed_cmd_lines == end) {
                return 0;
            } else {
                command_counter = read_command(&argvv, filev, &in_background);
            }
        }

        if (command_counter > MAX_COMMANDS) {
            printf("Error: Max number of commands exceeded\n");
            exit(EXIT_FAILURE);
        }
        if (command_counter == 0) {
            continue;
        }

        if (strcmp(argvv[0][0], "mycalc") == 0) {
            if (!in_background) {
                store_command(argvv, filev, in_background, &history[tail]);
                tail = (tail + 1) % history_size;
                if (n_elem < history_size) {
                    n_elem++;
                } else {
                    head = (head + 1) % history_size;
                }
                mycalc(argvv[0]);
            } else {
                printf("Error: mycalc no se puede ejecutar en background \n");
                continue;
            }
        } else if (strcmp(argvv[0][0], "myhistory") == 0) {
            if (!in_background) {
                myhistory();
            } else {
                printf("Error: myhistory no se puede ejecutar en background \n");
                continue;
            }
        } else {
            store_command(argvv, filev, in_background, &history[tail]);
            tail = (tail + 1) % history_size;
            if (n_elem < history_size) {
                n_elem++;
            } else {
                head = (head + 1) % history_size;
            }

            int i;
            int pipefd[2 * (command_counter - 1)];
            for (i = 0; i < command_counter - 1; i++) {
                if (pipe(pipefd + i * 2) < 0) {
                    perror("pipe");
                    exit(EXIT_FAILURE);
                }
            }

            pid_t pid;
            for (i = 0; i < command_counter; i++) {
                pid = fork();
                if (pid == 0) {
                    if (in_background && i == command_counter - 1) {
                        printf("[%d]\n", getpid());
                    }
                    if (i > 0) {
                        dup2(pipefd[(i - 1) * 2], STDIN_FILENO);
                    }
                    if (i < command_counter - 1) {
                        dup2(pipefd[i * 2 + 1], STDOUT_FILENO);
                    }
                    for (int j = 0; j < 2 * (command_counter - 1); j++) {
                        close(pipefd[j]);
                    }
                    if (i == command_counter - 1 && strcmp(filev[1], "0") != 0) {
                        int fd_out = open(filev[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (fd_out == -1) {
                            perror("Error opening output file");
                            exit(EXIT_FAILURE);
                        }
                        dup2(fd_out, STDOUT_FILENO);
                        close(fd_out);
                    }
                    if (i == 0 && strcmp(filev[0], "0") != 0) {
                        int fd_in = open(filev[0], O_RDONLY);
                        if (fd_in == -1) {
                            perror("Error opening input file");
                            exit(EXIT_FAILURE);
                        }
                        dup2(fd_in, STDIN_FILENO);
                        close(fd_in);
                    }
                    if (i == 0 && strcmp(filev[2], "0") != 0) {
                        int fd_err = open(filev[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                        if (fd_err == -1) {
                            perror("Error opening error file");
                            exit(EXIT_FAILURE);
                        }
                        dup2(fd_err, STDERR_FILENO);
                        close(fd_err);
                    }
                    execvp(argvv[i][0], argvv[i]);
                    perror("execvp");
                    exit(EXIT_FAILURE);
                } else if (pid < 0) {
                    perror("fork");
                    exit(EXIT_FAILURE);
                }
            }

            if (!in_background) {
                for (i = 0; i < 2 * (command_counter - 1); i++) {
                    close(pipefd[i]);
                }
                while(wait(&status) != pid);
            }
        }
    }

    free_command(history);
    return 0;
}

int mycalc(char *argv[]) {
    if (argv[1] == NULL || argv[2] == NULL || argv[3] == NULL) {
        printf("[ERROR] La estructura del comando es mycalc <operando 1> <add/mul/div> <operando 2>\n");
        return -1;
    }

    if((strcmp(argv[0], "mycalc") != 0) || 
        (atoi(argv[1]) == 0 && strcmp(argv[1], "0") != 0) || 
        ((strcmp(argv[2], "add") != 0) && (strcmp(argv[2], "mul") != 0) && (strcmp(argv[2], "div") != 0)) || 
        (atoi(argv[3]) == 0 && strcmp(argv[3], "0") != 0)) {

        printf("[ERROR] La estructura del comando es mycalc <operando_1> <add/mul/div> <operando_2>\n");
        return -1;
    }

    int op1 = atoi(argv[1]);
    int op2 = atoi(argv[3]);
    int resultado;
    char *operacion = argv[2];

    char *acc_str = getenv("Acc");
    int acc = (acc_str != NULL) ? atoi(acc_str) : 0;

    if (strcmp(operacion, "add") == 0) {
        resultado = op1 + op2;
        acc += resultado;
        char acc_buffer[20];
        snprintf(acc_buffer, 20, "%d", acc);
        setenv("Acc", acc_buffer, 1);
        fprintf(stderr, "[OK] %d + %d = %d; Acc %d\n", op1, op2, resultado, acc);
    } else if (strcmp(operacion, "mul") == 0) {
        resultado = op1 * op2;
        fprintf(stderr, "[OK] %d * %d = %d\n", op1, op2, resultado);
    } else if (strcmp(operacion, "div") == 0) {
        if (op2 == 0) {
            printf("[ERROR] División por cero.\n");
            return -1;
        }
        int cociente = op1 / op2;
        int resto = op1 % op2;
        fprintf(stderr, "[OK] %d / %d = %d; Resto %d\n", op1, op2, cociente, resto);
    }

    return 0;
}
