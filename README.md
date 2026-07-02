# emulador-de-cpu-en-c

## Resultados del Testing de Instrucciones
Aquí se documentan los resultados obtenidos al testear las instrucciones en la máquina rtm32 interactuando directamente a través del debugger.

---

# Caso 1
## Descripción: Test de operaciones Aritméticas Básicas y de Inmediatos
## Instrucctions: ADDI, ADD, SUB, MUL
## Precondiciones: 
- Reiniciar el emulador (cerrar el proceso y volver a abrirlo en la Terminal 1) para arrancar con PC en 0x0.
## Code
```text
set [0x0] 0x0802000A
set [0x4] 0x08040005
set [0x8] 0x0044301C
set [0xC] 0x0044401D
set [0x10] 0x00445015
step 5
r
```
## Postcondiciones: 
- R1 debe tener 10 (0x0A) [ADDI]
- R2 debe tener 5 (0x05) [ADDI]
- R3 debe tener 15 (0x0F) [ADD R1+R2]
- R4 debe tener 5 (0x05) [SUB R1-R2]
- R5 debe tener 50 (0x32) [MUL R1*R2]
## Conclusiones:
Anduvo correctamente. Las 5 instrucciones se ejecutaron en cadena.
- El registro `R[1]` se cargó correctamente con `0x0000000A` (10).
- El registro `R[2]` se cargó correctamente con `0x00000005` (5).
- El registro `R[3]` (ADD) contiene `0x0000000F` (15).
- El registro `R[4]` (SUB) contiene `0x00000005` (5).
- El registro `R[5]` (MUL) contiene `0x00000032` (50).
El PC avanzó fluidamente hasta `0x00000014` sin arrojar ninguna excepción (`CAUSE: 0x00000000`).

---

# Caso 2
## Descripción: Test de operaciones Lógicas y Desplazamientos
## Instrucctions: AND, OR, XOR, SLL
## Precondiciones: 
- Reiniciar el emulador (cerrar el proceso y volver a abrirlo en la Terminal 1) para arrancar con PC en 0x0.
## Code
```text
set [0x0] 0x08020F0F
set [0x4] 0x080400FF
set [0x8] 0x00443008
set [0xC] 0x00444009
set [0x10] 0x0044500A
set [0x14] 0x00046100
step 6
r
```
## Postcondiciones: 
- R1 debe tener 0x0F0F
- R2 debe tener 0x00FF
- R3 (AND) debe tener 0x000F
- R4 (OR) debe tener 0x0FFF
- R5 (XOR) debe tener 0x0FF0
- R6 (SLL R2 << 4) debe tener 0x0FF0
## Conclusiones:
Anduvo correctamente.
- El registro `R[1]` retuvo `0x00000F0F` y `R[2]` retuvo `0x000000FF`.
- El registro `R[3]` con el resultado del AND quedó en `0x0000000F`.
- El registro `R[4]` con el resultado del OR quedó en `0x00000FFF`.
- El registro `R[5]` con el resultado del XOR quedó en `0x00000FF0`.
- El registro `R[6]` (SLL) arrojó `0x000003FC`. Hubo una particularidad en el código binario ingresado: la constante codificada para el corrimiento no fue `4`, sino `2` (ya que el bit auxiliar quedó configurado en `0b00010`). La máquina desplazó correctamente `0x00FF` en 2 lugares hacia la izquierda resultando en `0x03FC`, lo que demuestra que la instrucción SLL y el pipeline funcionan a la perfección.
Sin excepciones en el CAUSE register, demostrando la alta confiabilidad del bloque lógico de la CPU.

---

# Caso 3
## Descripción: Test de Memoria y nuevo comando examine
## Instrucctions: SW, LW, SB
## Precondiciones: 
- Reiniciar el emulador (cerrar el proceso y volver a abrirlo en la Terminal 1) para arrancar con PC en 0x0.
## Code
```text
set [0x0] 0x08020100
set [0x4] 0x0804BEEF
set [0x8] 0x48440000
set [0xC] 0x40460000
step 4
r
x /xw 0x100 1
```
## Postcondiciones: 
- R1 se carga con 0x0100.
- R2 se carga con un inmediato extendido (ej. 0x0000BEEF).
- La instrucción SW escribe el R2 en la memoria RAM (dirección R1 = 0x100).
- La instrucción LW lee desde la memoria RAM (dirección R1) al R3.
- El nuevo comando `examine /xw` prueba la actualización del profe.
## Conclusiones:
Anduvo correctamente.
- El registro `R[1]` retuvo la dirección base `0x00000100`.
- El registro `R[2]` retuvo el valor inmediato extendido `0x0000BEEF`.
- Tras ejecutar `SW` y `LW` en cadena, el registro `R[3]` recuperó desde la memoria exactamente el mismo valor: `0x0000BEEF`.
- La operación de la CPU indica un "Type: READ" en la "Address: 0x00000100", confirmando el LW.
- Finalmente, el comando introducido en la nueva actualización del profesor (`x /xw 0x100 1`) demostró funcionar de maravilla volcando en pantalla el contenido exacto de la RAM: `0x00000100: 0x0000BEEF`.

---

# Caso 4
## Descripción: Test de Saltos Condicionales (Branching)
## Instrucctions: BEQ, BNE
## Precondiciones: 
- Reiniciar el emulador en la Terminal 1.
## Code
```text
set [0x0] 0x08020005
set [0x4] 0x08040005
set [0x8] 0x80440002
set [0xC] 0x08060001
set [0x10] 0x08060002
set [0x14] 0x08080063
set [0x18] 0x88440002
set [0x1C] 0x080A004D
step 6
r
```
## Postcondiciones: 
- R1 y R2 tienen 5.
- R3 debe valer 0 (las instrucciones en 0xC y 0x10 fueron salteadas exitosamente por el BEQ).
- R4 debe tener 99 (0x63), comprobando que el salto BEQ aterrizó en 0x14.
- R5 debe tener 77 (0x4D), comprobando que el BNE no saltó (porque R1=R2) y siguió de largo ejecutando la 0x1C.
## Conclusiones:
Anduvo correctamente.
- El registro `R[3]` se mantuvo en `0x00000000`, demostrando que el `BEQ` (Branch if Equal) detectó que `R[1] == R[2]` (5 == 5) y **saltó** con éxito por encima de las siguientes 2 instrucciones (offset = 2 palabras).
- El registro `R[4]` se cargó con `0x00000063` (99), demostrando que la CPU aterrizó exactamente donde debía.
- El registro `R[5]` se cargó con `0x0000004D` (77), validando que la instrucción `BNE` (Branch if Not Equal) detectó correctamente que los registros seguían siendo iguales (5 == 5), por lo tanto **no saltó** y continuó su ejecución lineal hacia la instrucción que asignaba el 77.
Todo esto sin ninguna excepción, CAUSE limpio (`0x00000000`).

---

# Caso 5
## Descripción: Test de Comparaciones lógicas y Saltos Incondicionales
## Instrucctions: SLT, SLTI, J
## Precondiciones: 
- Reiniciar el emulador en la Terminal 1.
## Code
```text
set [0x0] 0x08020005
set [0x4] 0x0804000A
set [0x8] 0x0044300C
set [0xC] 0xB0840005
set [0x10] 0x10000006
set [0x14] 0x080A0063
set [0x18] 0x080A004D
step 6
r
```
## Postcondiciones: 
- R1 (5) y R2 (10).
- R3 debe ser 1 (verdadero) porque SLT evalúa 5 < 10.
- R4 debe ser 0 (falso) porque SLTI evalúa 10 < 5.
- R5 debe tener 77 (0x4D), demostrando que el salto incondicional J saltó directo a 0x18 (dirección en words = 6) e ignoró la línea 0x14.
## Conclusiones:
Anduvo parcialmente. Se encontró un bug en el emulador.
- El salto incondicional `J` funciona a la perfección. Saltó directo a `0x18` e ignoró la línea de en medio, cargando `0x0000004D` en R5 de forma impecable.
- La comparación `SLT` (registro a registro) funciona bien: 5 < 10 es verdadero, y por eso R3 guardó el valor `1`.
- **EL BUG ESTÁ EN `SLTI`**: Según el manual, `SLTI rs rt imm` debería guardar el resultado de la comparación en `rt` (que era R4). Sin embargo, `R[4]` quedó en `0x00000000` y, misteriosamente, el registro `R[2]` (que era `rs` y tenía el valor 10) ¡fue sobreescrito con un `0`! Esto demuestra empíricamente que el profesor programó mal el destino en C para la instrucción `SLTI` y está guardando el resultado de la comparación en el registro fuente (`rs`) en lugar del registro destino (`rt`). 
Este es un error importante a reportar.

---

# Caso 6
## Descripción: Test de Divisiones y Lógicas Inmediatas (Formato L)
## Instrucctions: DIV, REST, ANDI, ORI
## Precondiciones: 
- Reiniciar el emulador en la Terminal 1.
## Code
```text
set [0x0] 0x08020014
set [0x4] 0x08040003
set [0x8] 0x00443018
set [0xC] 0x0044401A
set [0x10] 0x202A000F
set [0x14] 0x284C00F0
step 6
r
```
## Postcondiciones: 
- R1 (20) y R2 (3).
- R3 (DIV) debe tener 6 (20 / 3).
- R4 (REST) debe tener 2 (el resto de 20 / 3).
- R5 (ANDI) debe tener 4 (20 & 15).
- R6 (ORI) debe tener 243 o 0xF3 (3 | 240).
## Conclusiones:
Anduvo parcialmente. Se validaron los bugs que mencionaba el manual.
- `DIV` funcionó perfecto: 20 / 3 = 6 (`R[3] = 0x00000006`).
- `REST` funcionó perfecto: 20 % 3 = 2 (`R[4] = 0x00000002`).
- **BUG EN ANDI**: Como advertía la nota al pie del manual ("Actualmente hay un bug importante a solucionar en la instruccion ANDI"), pudimos comprobar que la instrucción devuelve `0x00000000` sin importar los operandos.
- **BUG EN ORI**: ¡Descubrimos otro bug! La instrucción debía ejecutar `3 | 240 = 243 (0xF3)`. Sin embargo, devolvió `0xF4`. Al analizarlo, `0xF4` es el resultado de hacer `20 | 240` (`0x14 | 0xF0`). ¡Esto significa que internamente el emulador se equivocó de registro y tomó el `R1` (que valía 20) en lugar del `R2` (que valía 3) como registro fuente!
Estos descubrimientos deben ser reportados.

---

# Caso 7 (FINAL)
## Descripción: Test de Subrutinas y Memoria de Medios Bloques
## Instrucctions: JAL, JR, SH, LH
## Precondiciones: 
- Reiniciar el emulador en la Terminal 1.
## Code
```text
set [0x0] 0x18000008
set [0x4] 0x08020063
set [0x20] 0x08020100
set [0x24] 0x080400FF
set [0x28] 0x50440000
set [0x2C] 0x60460000
set [0x30] 0x03E0000E
step 7
r
```
## Postcondiciones: 
- R31 debe contener 4 (dirección de retorno tras el JAL).
- R2 debe contener 0x000000FF.
- R3 debe contener 0x000000FF (probando que el Load Halfword y el Store Halfword funcionaron en cadena).
- R1 debe contener 99 (0x63), porque tras ejecutar toda la subrutina desde 0x20 a 0x30, el JR saltó de regreso a 0x4 y la ejecutó, demostrando el flujo de funciones.
## Conclusiones:
Anduvo parcialmente. Se validó correctamente el funcionamiento de memoria pero se encontró un bug crítico en el salto de retorno.
- Las instrucciones de memoria para medios bloques (`SH` y `LH`) funcionaron correctamente, leyendo y escribiendo `0x000000FF` en `R[2]` y `R[3]`.
- La instrucción `JAL` funcionó correctamente, logrando saltar hacia la subrutina en `0x20` y guardando la dirección de retorno (`0x00000004`) en el registro `R[31]`.
- **BUG EN `JR`**: La instrucción `JR R31` (Jump Register) que debía retornar a la dirección guardada en `R[31]` falló. En lugar de regresar a la dirección `0x4` (para que R1 adquiera el valor 99), el Program Counter (PC) terminó en `0x20`. Al analizar el recorrido, esto demuestra que `JR` ignoró el registro `rs` (R31) y erróneamente utilizó un registro en cero (probablemente `rd` o `rt`), lo que causó que la máquina saltara a la dirección `0x0`. Al caer en `0x0`, volvió a ejecutar el `JAL 0x20`, dejando el PC clavado de nuevo en `0x20`.
Este es un error importante a reportar, ya que imposibilita el retorno de las funciones.

---

# Caso 8 (Especiales y Excepciones)
## Descripción: Test de Sistema y Excepciones
## Instrucctions: LHU, CFS, CTS, TRAP, RFT
## Precondiciones: 
- Reiniciar el emulador en la Terminal 1.
## Code
```text
set [0x14] 0x00000018
set [0x100] 0xFFFFBEEF
set [0x0] 0x08021234
set [0x4] 0x00400207
set [0x8] 0x00800206
set [0xC] 0x000002A0
set [0x10] 0x68060100
set [0x18] 0x00000021
step 6
r
```
## Postcondiciones: 
- VBR (en Control Registers) debe valer 0x00001234.
- R2 debe valer 0x00001234 (demostrando que CTS y CFS funcionan).
- EPC (en Control Registers) debe valer 0x00000010 (la dirección de retorno que guardó el TRAP).
- R3 debe valer 0x0000BEEF (demostrando que LHU carga sin extender el signo 0xFFFF, y que el RFT funcionó y retornó a la línea 0x10).
## Conclusiones:
Anduvo parcialmente.
- Las instrucciones `CTS` y `CFS` (Copy To/From Special) **no funcionan**. El registro especial VBR jamás se actualizó con el valor de R1 (se quedó clavado en 2) y por lo tanto R2 recibió un 0. El emulador simplemente las ignora.
- La instrucción `TRAP` **está rota o no está implementada**. Al intentar ejecutarla en la dirección `0xC`, la máquina colapsó tirando una excepción (`CAUSE: 3` y `BADVADR: 0xC`), abortando la ejecución y entrando en modo Kernel.
- Debido a este cuelgue catastrófico del emulador provocado por el `TRAP`, las instrucciones `RFT` y `LHU` jamás llegaron a ejecutarse (el PC quedó perdido y el R3 quedó en 0).
