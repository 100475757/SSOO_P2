//P2-SSOO-23/24

//  MSH main file
// Write your msh source code here

//#include <parser.h>
#include <stddef.h>			/* NULL */
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


// files in case of redirection
char filev[3][64];

//to store the execvp second parameter
char *argv_execvp[8];

// Creamos un array y un contador de procesos en background
int bg_processes_number = 0; 
int *bg_processes; 

// Creamos un array de pids para almacenar los pids de los procesos hijos
int *pids;

//Aqui incluimos los prototipos de las funciones
//Esto es una buena practica para que el compilador pueda comprobar que las funciones estan bien declaradas
int int_digits(long long int n);
int wait_bg_processes();
int myCalc(char *argv[]);


void siginthandler(int param)
{
	printf("****  Exiting MSH **** \n");
	//signal(SIGINT, siginthandler);
	exit(0);
}
struct command
{
  // Store the number of commands in argvv
  int num_commands;
  // Store the number of arguments of each command
  int *args;
  // Store the commands
  char ***argvv;
  // Store the I/O redirection
  char filev[3][64];
  // Store if the command is executed in background or foreground
  int in_background;
};

int history_size = 20;
struct command * history;
int head = 0;
int tail = 0;
int n_elem = 0; 

void free_command(struct command *cmd)
{
    if((*cmd).argvv != NULL)
    {
        char **argv;
        for (; (*cmd).argvv && *(*cmd).argvv; (*cmd).argvv++)
        {
            for (argv = *(*cmd).argvv; argv && *argv; argv++)
            {
                if(*argv){
                    free(*argv);
                    *argv = NULL;
                }
            }
        }
    }
    free((*cmd).args);
}

void store_command(char ***argvv, char filev[3][64], int in_background, struct command* cmd)
{
    int num_commands = 0;
    while(argvv[num_commands] != NULL){
        num_commands++;
    }

    for(int f=0;f < 3; f++)
    {
        if(strcmp(filev[f], "0") != 0)
        {
            strcpy((*cmd).filev[f], filev[f]);
        }
        else{
            strcpy((*cmd).filev[f], "0");
        }
    }

    (*cmd).in_background = in_background;
    (*cmd).num_commands = num_commands-1;
    (*cmd).argvv = (char ***) calloc((num_commands) ,sizeof(char **));
    (*cmd).args = (int*) calloc(num_commands , sizeof(int));

    for( int i = 0; i < num_commands; i++)
    {
        int args= 0;
        while( argvv[i][args] != NULL ){
            args++;
        }
        (*cmd).args[i] = args;
        (*cmd).argvv[i] = (char **) calloc((args+1) ,sizeof(char *));
        int j;
        for (j=0; j<args; j++)
        {
            (*cmd).argvv[i][j] = (char *)calloc(strlen(argvv[i][j]),sizeof(char));
            strcpy((*cmd).argvv[i][j], argvv[i][j] );
        }
    }
}

/**
 * Get the command with its parameters for execvp
 * Execute this instruction before run an execvp to obtain the complete command
 * @param argvv
 * @param num_command
 * @return
 */
void getCompleteCommand(char*** argvv, int num_command) {
	//reset first
	for(int j = 0; j < 8; j++)
		argv_execvp[j] = NULL;

	int i = 0;
	for ( i = 0; argvv[num_command][i] != NULL; i++)
		argv_execvp[i] = argvv[num_command][i];
}

/**
 * Main sheell  Loop  
 */
int main(int argc, char* argv[])
{
	/**** Do not delete this code.****/
	int end = 0; 
	int executed_cmd_lines = -1;
	char *cmd_line = NULL;
	char *cmd_lines[10];
    bg_processes = (int *)malloc(10 * sizeof(int));

	if (!isatty(STDIN_FILENO)) {
		cmd_line = (char*)malloc(100);
		while (scanf(" %[^\n]", cmd_line) != EOF){
			if(strlen(cmd_line) <= 0) return 0;
			cmd_lines[end] = (char*)malloc(strlen(cmd_line)+1);
			strcpy(cmd_lines[end], cmd_line);
			end++;
			fflush (stdin);
			fflush(stdout);
		}
	}

	/*********************************/

	char ***argvv = NULL;
	int num_commands;

	history = (struct command*) malloc(history_size *sizeof(struct command));
	int run_history = 0;

	while (1) 
	{
		int status = 0;
		int command_counter = 0;
		int in_background = 0;
		signal(SIGINT, siginthandler);

		if (run_history) {
        run_history=0;
        }

        // Prompt 
        write(STDERR_FILENO, "MSH>>", strlen("MSH>>"));
        
        // Get command
        //********** DO NOT MODIFY THIS PART. IT DISTINGUISH BETWEEN NORMAL/CORRECTION MODE***************
        executed_cmd_lines++;
        if( end != 0 && executed_cmd_lines < end) {
            command_counter = read_command_correction(&argvv, filev, &in_background, cmd_lines[executed_cmd_lines]);
        }
        else if( end != 0 && executed_cmd_lines == end) {
            return 0;
        }
        else {
            command_counter = read_command(&argvv, filev, &in_background); //NORMAL MODE
        }
		//************************************************************************************************


		/************************ STUDENTS CODE ********************************/
        int fd_in, fd_out, fd_err; //descriptores de fichero para las redirecciones
        pid_t pid; //pid de los procesos
        int fd[num_commands-1][2]; //array de pipes

        if (command_counter > 0) {
            if (command_counter > MAX_COMMANDS){
                printf("Error: Máximo número de comandos en %d \n", MAX_COMMANDS);
            }
            else {
                if (strcmp(argvv[0][0], "mycalc") == 0) {
                    myCalc(argvv[0]);
                }
                else if (strcmp(argvv[0][0], "myhistory") == 0) {
                    /* Mandato myhistory*/
                    // Comprobamos que la estructura del comando sea correcta
                   if (argvv[0][1] != NULL && (argvv[0][2] != NULL || isdigit(*argvv[0][1]) == 0)) {
                    printf("[ERROR] La estructura del comando es myhistory o myhistory <nº comando> \n");
                    continue;
                   }
                   if (argvv[0][1] == NULL) {
                        // Mostrar los 20 últimos comandos
                        for (int i = 0; i < n_elem; i++) {
                            fprintf(stderr, "%d ", i);
                            for (int j = 0; j < history[i].num_commands; j++) {
                                for (int k = 0; k < history[i].args[j]; k++) {
                                    fprintf(stderr, "%s ", history[i].argvv[j][k]);
                                }
                            }
                            fprintf(stderr, "\n");
                        }
                    }
                   else {
                        int n = atoi(argvv[0][1]);
                        if (n >= 0 && n < n_elem) {
                            fprintf(stderr, "Ejecutando el comando %d\n", n);
                            run_history = 1;
                            store_command(history[n].argvv, history[n].filev, history[n].in_background, &history[0]);
                        }
                        else {
                            fprintf(stdout, "ERROR: Comando no encontrado\n");
                        }
                   }
                      
                }
                else  {//Ahora comenzamos con el desarrollo de la minishell en si.
                    //como vamos a hacer una implementacion no limitada a ningun numero de procesos ni de pipes, es necesario hacer un bucle 
                    //desde i=0 que representa el primer proceso hasta el numero metido en la variable
                    //command_counter que representa el numero total de procesos
                    pids = (int *)malloc(command_counter * sizeof(int)); //reservamos memoria para los pids
                    for (int i = 0; i < command_counter; i++) {
                        //creamos los pipes
                        if (pipe(fd[i]) == -1) {
                            perror("[ERROR] Hubo un error al crear el pipe");
                        }
                        //creamos el proceso hijo
                        pid = fork();
                        if (pid == -1) {
                            perror("[ERROR] Hubo un error en el fork");
                        }
                        else if (pid == 0) { //proceso hijo
                            //Primero hacemos las comprobaciones de redirecciones de salida estandar de error.
                            if (strcmp(filev[2], "0")!= 0) {
                                fd_err = open(filev[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
                                if (fd_err == -1) {
                                    perror("[ERROR] Hubo un error al abrir el fichero de error");
                                    exit(-1);
                                }
                                if (dup2(fd_err, STDERR_FILENO) == -1) {
                                    perror("[ERROR] Hubo un error al redirigir el la salida estandar de error");
                                    exit(-1);
                                }
                                close(fd_err);
                            }
                            //si es el primer proceso
                            if (i == 0) {
                                //si hay redireccion de entrada
                                if (strcmp(filev[0], "0") != 0) {
                                    fd_in = open(filev[0], O_RDONLY);
                                    if (fd_in == -1) {
                                        perror("[ERROR] Hubo un error al abrir el fichero de entrada");
                                        exit(-1);
                                    }
                                    if (dup2(fd_in, STDIN_FILENO) == -1) {
                                        perror("[ERROR] Hubo un error al redirigir la entrada estandar");
                                        exit(-1);
                                    }
                                    close(fd_in);
                                }
                            }
                            //si es un proceso intermedio o final
                            //redireccionamos la entrada al pipe anterior
                            else {
                                if (dup2(fd[i-1][0], STDIN_FILENO) == -1) {
                                    perror("[ERROR] Hubo un error al redirigir la entrada al pipe anterior");
                                    exit(-1);
                                }
                                close(fd[i-1][1]); //cerramos la escritura del pipe anterior
                                waitpid(pids[i-1], &status, 0); 
                            }
                            //Cerramos la lectura del pipe actual, ya que
                            //no lo vamos a usar
                            close(fd[i][0]);
                            //si es el ultimo proceso
                            if (i == command_counter - 1) {
                                //el ultimo comando no usa el pipe i asi que lo cerramos
                                close(fd[i][1]);
                                close(fd[i][0]);
                                //si hay redireccion de salida
                                if (strcmp(filev[1], "0") != 0) {
                                    fd_out = open(filev[1], O_WRONLY | O_CREAT | O_TRUNC, 0666);
                                    if (fd_out == -1) {
                                        perror("[ERROR] Hubo un error al abrir el fichero de salida");
                                        exit(-1);
                                    }
                                    if (dup2(fd_out, STDOUT_FILENO) == -1) {
                                        perror("[ERROR] Hubo un error al redirigir la salida estandar");
                                        exit(-1);
                                    }
                                    close(fd_out);
                                }
                            }
                            else {
                                //si no es el ultimo proceso
                                //redireccionamos la salida al pipe actual
                                if (dup2(fd[i][1], STDOUT_FILENO) == -1) {
                                    perror("[ERROR] Hubo un error al redirigir la salida al pipe actual");
                                    exit(-1);
                                }
                            }
                            //ya hemos redireccionado tanto los pipes como los ficheros
                            //ahora ejecutamos el comando
                            execvp(argvv[i][0], argvv[i]); 
                            perror("[ERROR] Hubo un error al ejecutar el comando");
                            exit(-1);
                        }
                        //si es el padre
                        else {
                            //guardamos el pid del proceso hijo
                            //cerramos los pipes que no vamos a usar que son los del proceso anterior
                            //backgroud o no background
                            pids[i] = pid;
                            if(i>0) {
                                close(fd[i-1][0]);
                                close(fd[i-1][1]);
                            }
                            if (i == command_counter - 1) {
                                //el pipe del ultimo proceso no se usa
                                close(fd[i][0]);
                                close(fd[i][1]); 
                                
                                if (in_background == 0) { // ejecucion en foreground
                                    while(pid!= wait(&status));
                                    // esperamos a que terminen los procesos en background de secuencias de comandos anteriores
                                    wait_bg_processes();
                                }
                                else { // ejecucion en background
                                    fprintf(stderr, "Proceso en background con pid [%d]\n", pid);
                                    //vamos a relocalizar la memoria para el array de procesos en background
                                    bg_processes = (int *)realloc(bg_processes, (bg_processes_number + command_counter) * sizeof(int));
                                    bg_processes_number += command_counter;
                                    // Añadir los pids de esta secuencia a la lista de procesos en segundo plano (de adelante hacia atrás)
                                    for (int j = 0; j < command_counter; j++) {
                                        bg_processes[bg_processes_number - command_counter + j] = pids[j];
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

        int wait_bg_processes() {
            //esperamos a que terminen los procesos en background
            //de secuencias de comandos anteriores
            //evitamos porcesos zombies
            int status;
            for (int i = 0; i < bg_processes_number; i++) {
                //se espera empezando por el primer proceso en background
                waitpid(bg_processes[i], &status, 0);
            }
            return 0;
        }
        int myCalc(char *argv[]){
            // Esta función implementa una calculadora básica en la terminal.
            // Primero, verifica la validez de los argumentos proporcionados.
            // Luego, realiza la operación solicitada y muestra el resultado.

            long long int acc; // Esta variable local almacena el valor de la variable de entorno 'Acc'.
            char *acc_str=(char *) malloc(sizeof(char)); // Esta variable local almacena el valor de 'Acc' en formato de cadena.
            long long int resultado = 0; // Esta variable almacena el resultado de la operación.
            int resto = 0; // Esta variable almacena el resto de la operación de división.

            // Verifica que el comando tenga exactamente tres argumentos: operando_1, operador, operando_2.
            if (argv[1] == NULL || argv[2] == NULL || argv[3] == NULL || argv[4] != NULL){
                fprintf(stdout, "[ERROR] La estructura del comando es mycalc <operando_1> <add/mul/div> <operando_2>\n");
                return -1;
            } 

            // Verifica que los operandos sean números válidos.
            if ((atoi(argv[1]) == 0 && strcmp(argv[1],"0")!=0) || (atoi(argv[3]) == 0 && strcmp(argv[3],"0")!=0)){
                fprintf(stdout, "[ERROR] La estructura del comando es mycalc <operando_1> <add/mul/div> <operando_2>\n");
                return -1;
            }

            // Verifica que el operador sea una de las operaciones permitidas: add, mul, div.
            if (strcmp(argv[2], "add") != 0 && strcmp(argv[2], "mul") != 0 && strcmp(argv[2], "div") != 0){
                fprintf(stdout, "[ERROR] La estructura del comando es mycalc <operando_1> <add/mul/div> <operando_2>\n");
                return -1;
            }
            
            // Dependiendo del primer carácter del operador, selecciona la operación a realizar.
            switch (argv[2][0]){
                case 'a': // SUMA
                    // Si la variable de entorno 'Acc' no existe, se crea con valor 0.
                    if (getenv("Acc") == NULL){
                        setenv("Acc", "0", 1);
                    }
                    // Calcula el resultado de la suma.
                    resultado = atoi(argv[1]) + atoi(argv[3]);
                    
                    // Modifica 'Acc' sumándole el resultado.
                    acc = atoll(getenv("Acc"));
                    acc += resultado;
                    // Convierte 'acc' a cadena.
                    acc_str = malloc(sizeof(char)*int_digits(acc));
                    sprintf(acc_str, "%lld", acc);
                    // Guarda 'acc_str' en la variable de entorno 'Acc'.
                    setenv("Acc", acc_str, 1);
                    
                    // Muestra el resultado por pantalla.
                    fprintf(stderr, "[OK] %s + %s = %lld; Acc %lld\n", argv[1], argv[3], resultado, acc);
                    break;
                
                case 'm': // MULTIPLICACION
                    // Calcula el resultado de la multiplicación.
                    resultado = atoi(argv[1]) * atoi(argv[3]);
                    
                    // Muestra el resultado por pantalla.
                    fprintf(stderr, "[OK] %s * %s = %lld\n", argv[1], argv[3], resultado);
                    break;
                
                case 'd': // DIVISION
                    // Verifica que el divisor no sea 0.
                    if (atoi(argv[3])==0){ 
                        fprintf(stdout, "[ERROR] Division por 0\n"); 
                        return -1;
                    }
                    // Calcula el resultado y el resto de la división.
                    resultado = atoi(argv[1]) / atoi(argv[3]);
                    resto = atoi(argv[1]) % atoi(argv[3]);
                    
                    // Muestra el resultado y el resto por pantalla.
                    fprintf(stderr, "[OK] %s / %s = %lld; Resto %d\n", argv[1], argv[3], resultado, resto);
                    break;
            }
            return 0; // Retorna 0 si la operación se realizó con éxito.
        }

        int int_digits(long long int n){
            // Esta función calcula el número de dígitos de un número entero.
            int digits = 0;
            while (n != 0){
                n /= 10;
                digits++;
            }
            return digits;
        }
        


       