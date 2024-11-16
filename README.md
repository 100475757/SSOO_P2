# Minishell en C

## Objetivo del Proyecto

Este proyecto tiene como objetivo desarrollar un intérprete de comandos, o shell, en el lenguaje C sobre el sistema operativo UNIX/Linux. 
La práctica está diseñada para que los estudiantes se familiaricen con los servicios de gestión de procesos que proporciona POSIX y comprendan 
el funcionamiento interno de una shell en UNIX/Linux. El proyecto busca implementar un minishell que permita ejecutar comandos, gestionar procesos 
y trabajar con redirección de entradas y salidas.

## Funcionalidad

El proyecto proporciona las siguientes funcionalidades:

- **Ejecución de Mandatos Simples:** Permite ejecutar comandos como `ls -l`, `who`, etc., utilizando la entrada estándar.
- **Ejecución en Background:** Los comandos pueden ejecutarse de manera asíncrona añadiendo `&` al final de la línea, permitiendo que el minishell no quede bloqueado esperando su finalización.
- **Redirección de Entrada y Salida:** Soporta redirección de entrada (`<`), salida (`>`), y salida de error (`!>`), permitiendo una mejor gestión de los resultados de los comandos.
- **Pipes entre Comandos:** Permite conectar comandos mediante `|`, creando secuencias donde la salida de un comando es la entrada del siguiente.
- **Mandatos Internos:** Implementa comandos internos como `mycalc` (una calculadora simple que permite suma, multiplicación y división) y `mytime` (que muestra el tiempo de ejecución del minishell).

## Compilación

Para compilar este proyecto, sigue los siguientes pasos:

1. Asegúrate de tener instalado **GCC**.
2. Navega al directorio principal del proyecto:
   ```bash
   cd /ruta/a/tu/proyecto
   ```
3. Compila el código usando el siguiente comando:
   ```bash
   make
   ```
   Si la biblioteca `libparser.so` no se encuentra, asegúrate de indicar su ruta al compilador:
   ```bash
   export LD_LIBRARY_PATH=/home/username/path:$LD_LIBRARY_PATH
   ```

Asegúrate de que no haya errores durante la compilación para poder continuar con la ejecución.

## Uso

Una vez compilado, puedes ejecutar el proyecto siguiendo estos pasos:

1. Ejecuta el archivo compilado:
   ```bash
   ./msh
   ```
2. Sigue las instrucciones que aparecen en la consola para interactuar con el programa.

### Ejemplo de Uso

```bash
$ ./msh
MSH>> ls -l
(total de archivos listados)
MSH>> mycalc 3 add 5
[OK] 3 + 5 = 8; Acc 8
MSH>> myhistory
1: ls -l
2: mycalc 3 add 5
```

Este ejemplo muestra un caso básico de cómo interactuar con el programa.

## Requisitos

- **Sistema operativo:** Linux
- **Dependencias:** Biblioteca `libparser.so` proporcionada
- **Compilador/Requisitos de software:** GCC
