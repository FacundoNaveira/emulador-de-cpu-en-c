# Explicaciones sobre la Máquina RTM32 y Arquitectura STX4

A continuación, se responden las preguntas fundamentales para entender qué están haciendo y cómo trabajar con el emulador.

## 1. ¿Qué es la máquina (RTM32)?
La **máquina RTM32** es un **emulador**. Es un programa de software que simula el comportamiento de una computadora física y su procesador (una CPU llamada **STX4**, inventada para este trabajo). 
Imagínalo como una "computadora virtual" dentro de tu computadora real. No existe físicamente, pero tiene su propia memoria RAM, sus propios "registros" (pequeños espacios para guardar números temporalmente y muy rápido) y sus propias reglas de cómo sumar, restar o leer datos.

## 2. ¿Cómo funciona?
Al igual que una computadora real, el emulador RTM32 hace un ciclo básico de tres pasos:
1. **Lee (Fetch):** Busca en su memoria virtual la próxima instrucción que debe ejecutar.
2. **Decodifica (Decode):** Interpreta los 32 bits de esa instrucción para entender si tiene que sumar, restar, saltar a otro lado, etc.
3. **Ejecuta (Execute):** Realiza la operación matemática o lógica usando sus registros, o lee/escribe en su memoria, y avanza a la siguiente instrucción.

El objetivo del proyecto es interactuar con esta máquina, dándole instrucciones en su propio "lenguaje de máquina" (Machine Code) y viendo cómo se comporta internamente.

## 3. ¿Cómo se corre?
Como este emulador no tiene un monitor gráfico, funciona enteramente por consola de texto y red. Los pasos para usarla son:

1. Primero, enciendes la máquina virtual en modo "debugger" (depurador) usando la terminal de Linux (WSL):
   ```bash
   ./rtm32 -d telnet
   ```
   *Esto arranca la máquina, la cual se queda pausada y abre el puerto 4444 esperando órdenes.*

2. Luego, abres **otra pestaña/ventana de terminal** y te conectas a la máquina usando telnet:
   ```bash
   telnet localhost 4444
   ```
   *Esto abrirá la consola del debugger (verás un prompt como `RTM32>`). A partir de aquí, estás "enchufado" directamente al cerebro de la máquina y puedes darle órdenes manuales (ver valores de la memoria, cambiar registros, decirle que avance un paso).*

## 4. ¿Cómo se usan las instrucciones?
Las instrucciones (ej. `ADD`, `SUB`, `JUMP`) son los comandos básicos que entiende el procesador STX4. 
Para usar una instrucción:
1. Traduces la instrucción de texto a su versión numérica (Binario o Hexadecimal). El manual explica cómo cada instrucción se forma de 32 bits (5 bits de operación, 5 bits de registro fuente, etc.).
2. A través del debugger de telnet, cargas ese número (Machine Code) en una dirección de la memoria virtual (ej. en la posición `0x0000`).
3. También con el debugger, "preparas el escenario" poniendo valores en los registros que vas a usar (ej. `setear el registro 1 en 5`).
4. Le dices al debugger que ejecute esa instrucción (normalmente con un comando de tipo `step`).

## 5. ¿Cómo nos damos cuenta que una instrucción NO funciona?
El profesor programa este emulador y puede haber cometido errores en su código (bugs). 
Para saber si una instrucción falla, se hace un **Testing manual de Caja Negra**:
1. **Planteas una hipótesis:** "Si yo sumo 5 (en registro 1) y 10 (en registro 2) usando la instrucción `ADD`, el resultado debe ser 15 y debe guardarse en el registro 3".
2. **Preparas la máquina:** Pones un 5 en el registro 1, un 10 en el registro 2.
3. **Pones la instrucción:** Cargas la instrucción correspondiente a `ADD $1, $2, $3` en memoria.
4. **Ejecutas:** Haces que la máquina ejecute el comando.
5. **Verificas el resultado (Postcondición):** Le preguntas al debugger qué valor quedó en el registro 3. 
   - **Si tiene 15:** La instrucción funciona (el profesor la programó bien).
   - **Si tiene otro número, si da error, o si la máquina se cuelga:** ¡Felicidades! Encontraste un fallo. Esto significa que el emulador decodificó mal la instrucción o hizo mal la cuenta internamente. En ese caso, vas al foro, reportas "La instrucción ADD está fallando bajo X circunstancias", el profesor revisa su código en C del emulador, lo corrige, y sube una nueva versión de `rtm32`.
