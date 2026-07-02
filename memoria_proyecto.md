# Memoria del Proyecto RTM32 (STX4)

Este archivo actúa como una "base de conocimientos" para documentar todo el funcionamiento, problemas y peculiaridades descubiertas trabajando con el emulador RTM32 (Arquitectura STX4) a fin de tener un registro rápido para el futuro.

## 1. Arquitectura y Componentes
*   **Máquina/Emulador:** `rtm32`. Es un binario de Linux de 64 bits (ELF). Su propósito es simular un procesador inventado llamado STX4.
*   **Procesador (STX4):** Es un procesador RISC de 32 bits de arquitectura ortogonal, fuertemente inspirado en MIPS (comparte nombres de registros como `$zero`, `$at`, `$v0`, `$a0`, `$t0`, `$s0`, `$gp`, `$sp`, `$fp`, `$ra`).
*   **Memoria Mapeada (MMIO):** La consola interactiva mediante UART está mapeada en la dirección `0xFFFFFF00`. 
    *   Escribir (SB - Store Byte) en esa dirección imprime un caracter en consola.
    *   Leer de esa dirección permite leer un caracter del teclado desde un buffer de 16 bytes.
*   **Herramientas ausentes:** No existe (o no se ha provisto) un ensamblador (Assembler) ni un compilador de C (como GCC) para STX4. Todo el código debe inyectarse en memoria en formato crudo (Machine Code en Hexadecimal/Binario) calculando los opcodes a mano o escribiendo scripts que generen el archivo `.bin`.

## 2. Funcionamiento del Debugger (Depurador)
*   Se activa al ejecutar el emulador con la flag correspondiente: `./rtm32 -d telnet`.
*   Esto bloquea el hilo principal y levanta un servidor TCP interno en el **puerto 4444**.
*   Para interactuar se requiere conectarse con un cliente de red simple: `telnet localhost 4444`.

### Comandos conocidos del Debugger
*   **Menú principal (`help`):**
    *   `step (n)`: Ejecuta una instrucción (paso a paso).
    *   `continue (c)`: Reanuda la ejecución hasta encontrar un breakpoint.
    *   `examine (x)`: Lee la memoria.
    *   `registers (r)`: Muestra el estado de los registros de CPU.
    *   `set (s)`: Inyecta un valor en memoria o en un registro. (Ej: `set r1 0xFF00`, `set [0x40] 0xEA0000`).
    *   `load ( )`: Carga un archivo binario a memoria. Sintaxis: `load <filename> [fast|exact]`. Ej: `load firmware.bin exact`.

## 3. Errores Encontrados y Soluciones

*   **Error:** `Permission denied` al intentar ejecutar `./rtm32`.
    *   **Causa:** Al descargar o mover archivos nuevos en Linux, los permisos de ejecución (flag `x`) se pierden.
    *   **Solución:** Ejecutar `chmod +x rtm32`.
*   **Error:** `Command 'telnet' not found`.
    *   **Causa:** Las instalaciones modernas de Ubuntu en WSL ya no incluyen el cliente Telnet por defecto por temas de seguridad.
    *   **Solución:** Instalarlo manualmente con `sudo apt install telnet`.
*   **Error:** Falso reconocimiento de `rtm32.s`.
    *   **Causa:** Originalmente, se creyó que el archivo `.s` era código fuente en lenguaje ensamblador, pero una inspección con la herramienta `file` reveló que `rtm32.s` era, de hecho, un binario ejecutable ELF compilado estáticamente (una versión diferente del mismo emulador `rtm32` dinámico).
    *   **Solución:** Utilizar el archivo `rtm32` e ignorar el archivo `.s` para la programación del código.
*   **Error:** Fallo en script automatizado por puerto bloqueado (`Address already in use`).
    *   **Causa:** Intentar lanzar el debugger o conectarse programáticamente mientras la terminal 1 del usuario ya mantenía el socket del emulador abierto.
    *   **Solución:** Pedir la ejecución manual de comandos `help` al usuario en su terminal 2 para no chocar con los puertos TCP.

## 4. Notas de Programación
*   Para escribir a consola desde ensamblador (rutina probada en el "Hola mundo"):
    1. Usar `LUI` (Load Upper Immediate) para cargar la parte superior de la dirección de MMIO.
    2. Usar `ORI` (OR Immediate) para completar la parte inferior (`0xFFFFFF00`).
    3. Usar `ADDI` (Add Immediate) con el registro `$zero` (siempre vale 0) para cargar el valor ASCII del caracter en un registro temporal.
    4. Usar `SB` (Store Byte) para guardar el caracter en la dirección MMIO.

### Comando `examine` (x) avanzado
El comando `examine` fue actualizado y tiene una sintaxis avanzada no documentada en el help:
*   Sintaxis: `examine [/<format><size>] address count` (la barra `/` es opcional).
*   Tamaños soportados (`size`): `w` (word), `h` (half), `b` (byte)
*   Formatos soportados (`format`): `x` (hex), `t` (bin), `d` (dec), `o` (oct), `s` (str)
*   Ejemplos de uso válido: `x /tb 0 3`, `x bin word 4`.
*   Nota: Genera un error si se intenta examinar en un tamaño que no esté correctamente alineado en memoria.

*Este documento se mantendrá actualizado a medida que surjan nuevos desafíos técnicos.*
