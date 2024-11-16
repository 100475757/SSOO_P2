//P2-SSOO-22/23

// MSH main file
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

// Definición de variables globales y estructuras
char filev[3][64]; // Ficheros para redirección
char *argv_execvp[8]; // Array para almacenar los parámetros de execvp
int *background_processes; // Array para procesos en background
int background_processes_counter = 0; // Contador de procesos en background
int *pids; // Array para almacenar los pids de los procesos hijos

// Prototipos de funciones
int myCalc(char *argv[]);
int myHistory(char *argv[]);
int get_digits(int n);
int wait_background_processes();

#define HISTORY_SIZE 100
char *history[HISTORY_SIZE]; // Historial de comandos
int history_count = 0; // Contador de comandos en el historial

// Manejador de señal SIGINT
void siginthandler(int param)
{
    printf("****  Saliendo del MSH **** \n");
    exit(0);
}

// Función para obtener el comando completo para execvp
void getCompleteCommand(char*** argvv, int num_command) {
    //reset first
    for(int j = 0; j < 8; j++)
        argv_execvp[j] = NULL;

    int i = 0;
    for (i = 0; argvv[num_command][i] != NULL; i++)
        argv_execvp[i] = argvv[num_command][i];
}

/**
 * Main shell Loop  
 */

int main(int argc, char* argv[])
{
    /**** Do not delete this code.****/
    int end = 0; 
    int executed_cmd_lines = -1;
    char *cmd_line = NULL;
    char *cmd_lines[10];
    background_processes = (int*)malloc(10 * sizeof(int));

    if (!isatty(STDIN_FILENO)) {
        cmd_line = (char*)malloc(100);
        while (scanf(" %[^\n]", cmd_line) != EOF){
            if(strlen(cmd_line) <= 0) return 0;
            cmd_lines[end] = (char*)malloc(strlen(cmd_line) + 1);
            strcpy(cmd_lines[end], cmd_line);
            end++;
            fflush(stdin);
            fflush(stdout);
        }
    }

    /*********************************/

    char ***argvv = NULL;

    while (1) 
    {
        int status = 0;
        int command_counter = 0;
        int in_background = 0;
        signal(SIGINT, siginthandler);

        // Prompt 
        write(STDERR_FILENO, "MSH>>", strlen("MSH>>"));

        // Obtener el comando
        executed_cmd_lines++;
        if (end != 0 && executed_cmd_lines < end) {
            command_counter = read_command_correction(&argvv, filev, &in_background, cmd_lines[executed_cmd_lines]);
        }
        else if (end != 0 && executed_cmd_lines == end){
            return 0;
        }
        else{
            command_counter = read_command(&argvv, filev, &in_background); // Modo normal
        }

        if (command_counter > 0) {
            // Almacenar el comando en el historial
            if (history_count < HISTORY_SIZE) {
                history[history_count] = strdup(argvv[0][0]);
                for (int i = 1; argvv[0][i] != NULL; i++) {
                    history[history_count] = realloc(history[history_count], strlen(history[history_count]) + strlen(argvv[0][i]) + 2);
                    strcat(history[history_count], " ");
                    strcat(history[history_count], argvv[0][i]);
                }
                history_count++;
            }

            if (command_counter > MAX_COMMANDS){
                printf("Error: Numero máximo de comandos es %d \n", MAX_COMMANDS);
            }
            else {
                if (strcmp(argvv[0][0], "mycalc") == 0) {
                    // Ejecución del comando interno "mycalc"
                    myCalc(argvv[0]);
                }
                else if (strcmp(argvv[0][0], "myhistory") == 0){
                    // Ejecución del comando interno "myhistory"
                    myHistory(argvv[0]);
                }
                else{ // comando externo
                    pids = (int*)malloc(command_counter * sizeof(int));
                    int pipefd[command_counter - 1][2];
                    
                    for (int i = 0; i < command_counter; i++){
                        if (i < command_counter - 1 && pipe(pipefd[i]) < 0) {
                            perror("[ERROR] Error al crear el pipe");    
                        }
                        int pid = fork();

                        if (pid == 0){ // Proceso hijo
                            // Redirección de entrada y salida estándar
                            if (i > 0){
                                dup2(pipefd[i-1][0], STDIN_FILENO); // Leer del pipe anterior
                                close(pipefd[i-1][1]); // Cerrar escritura del pipe anterior
                                waitpid(pids[i-1], &status, 0);
                            }
                            else if (strcmp(filev[0], "0") != 0) { // Redirección de entrada desde fichero
                                int fd = open(filev[0], O_RDONLY);
                                if (fd < 0) {
                                    perror("[ERROR] Error al abrir el fichero de entrada");
                                    exit(1);
                                }
                                dup2(fd, STDIN_FILENO);
                                close(fd);
                            }
                            if (i < command_counter - 1){
                                close(pipefd[i][0]);
                                dup2(pipefd[i][1], STDOUT_FILENO);
                            }
                            else {
                                close(pipefd[i][0]);
                                close(pipefd[i][1]);
                                if (strcmp(filev[1], "0") != 0) {
                                    int fd = open(filev[1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
                                    if (fd < 0) {
                                        perror("[ERROR] Error al abrir el fichero de salida");
                                        exit(1);
                                    }
                                    dup2(fd, STDOUT_FILENO);
                                    close(fd);
                                }
                                if (strcmp(filev[2], "0") != 0) {
                                    int fd = open(filev[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
                                    if (fd < 0) {
                                        perror("[ERROR] Error al abrir el fichero de salida de error");
                                        exit(1);
                                    }
                                    dup2(fd, STDERR_FILENO);
                                    close(fd);
                                }
                            }
                            execvp(argvv[i][0], argvv[i]);
                        } 
                        else if (pid < 0){
                            perror("[ERROR] fork");
                        }
                        else { // Proceso padre
                            pids[i] = pid;
                            if (i > 0){
                                close(pipefd[i-1][0]);
                                close(pipefd[i-1][1]);
                            }

                            if (i == command_counter - 1){
                                close(pipefd[i][0]);
                                close(pipefd[i][1]);

                                if (in_background != 1) { // Foreground
                                    for (int j = command_counter - 1; j >= 0; j--){
                                        waitpid(pids[j], &status, 0);
                                    }
                                    wait_background_processes();
                                } else { // Background
                                    fprintf(stderr, "[%d]\n", pid);
                                    background_processes = (int*)realloc(background_processes, (background_processes_counter + command_counter) * sizeof(int));
                                    background_processes_counter += command_counter;
                                    for (int j = 0; j < command_counter; j++){
                                        background_processes[background_processes_counter - j] = pids[j];
                                    }
                                }
                            }
                        }
                    }
                }    
            }
        }
    }
    return 0;
}

// Función para esperar a que terminen los procesos en background
int wait_background_processes(){ 
    int status; 
    while (background_processes_counter != 0){
        waitpid(background_processes[background_processes_counter - 1], &status, 0); 
        background_processes_counter--;
    }
    return 0;
}

// Implementación del comando interno "mycalc"
int myCalc(char *argv[]){
    long long int acc;
    char *acc_str = (char*) malloc(sizeof(char));
    long long int resultado = 0; 
    int resto = 0; 
    
    if (argv[1] == NULL || argv[2] == NULL || argv[3] == NULL || argv[4] != NULL){
        fprintf(stdout, "[ERROR] La estructura del comando es mycalc <operando_1> <add/mul/div> <operando_2>\n");
        return -1;
    } 
    if ((atoi(argv[1]) == 0 && strcmp(argv[1],"0") != 0) || (atoi(argv[3]) == 0 && strcmp(argv[3],"0") != 0)){
        fprintf(stdout, "[ERROR] La estructura del comando es mycalc <operando_1> <add/mul/div> <operando_2>\n");
        return -1;
    }
    if (strcmp(argv[2], "add") != 0 && strcmp(argv[2], "mul") != 0 && strcmp(argv[2], "div") != 0){
        fprintf(stdout, "[ERROR] La estructura del comando es mycalc <operando_1> <add/mul/div> <operando_2>\n");
        return -1;
    }
    
    switch (argv[2][0]){
        case 'a': 
            if (getenv("Acc") == NULL){
                setenv("Acc", "0", 1);
            }
            resultado = atoi(argv[1]) + atoi(argv[3]);
            
            acc = atoll(getenv("Acc"));
            acc += resultado;
            acc_str = malloc(sizeof(char) * get_digits(acc));
            sprintf(acc_str, "%lld", acc);
            setenv("Acc", acc_str, 1);
            
            fprintf(stderr, "[OK] %s + %s = %lld; Acc %lld\n", argv[1], argv[3], resultado, acc);
            break;
        
        case 'm': 
            resultado = atoi(argv[1]) * atoi(argv[3]);
            fprintf(stderr, "[OK] %s * %s = %lld\n", argv[1], argv[3], resultado);
            break;
        
        case 'd': 
            if (atoi(argv[3]) == 0){ 
                fprintf(stdout, "[ERROR] Division por 0\n"); 
                return -1;
            }
            resultado = atoi(argv[1]) / atoi(argv[3]);
            resto = atoi(argv[1]) % atoi(argv[3]);
            fprintf(stderr, "[OK] %s / %s = %lld; Resto %d\n", argv[1], argv[3], resultado, resto);
            break;
    }
    return 0;
}

// Implementación del comando interno "myhistory"
int myHistory(char *argv[]){
    for (int i = 0; i < history_count; i++) {
        printf("%d: %s\n", i + 1, history[i]);
    }
    return 0;
}

// Función para obtener el número de dígitos de un número
int get_digits(int num){
    int digits = 0;
    while (num != 0){
        digits++;
        num /= 10;
    }
    return digits;
}
