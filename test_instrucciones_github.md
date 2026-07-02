# Tests Agrupados RTM32

---

# Caso 1
## Descripción: Test de operaciones Aritméticas Básicas y de Inmediatos
## Instrucctions: ADDI, ADD, SUB, MUL
## Precondiciones:
- Reiniciar el emulador (cerrar y volver a abrir en la Terminal 1) para que el PC arranque en 0x0.
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
- R1 debe tener 10 (0x0A)
- R2 debe tener 5 (0x05)
- R3 debe tener 15 (0x0F)
- R4 debe tener 5 (0x05)
- R5 debe tener 50 (0x32)
## Conclusiones:
Anduvo. Las instrucciones se ejecutaron en cadena correctamente validando el pipeline de la CPU.


---

# Caso 2
## Descripción: Test de operaciones Lógicas y Desplazamientos
## Instrucctions: AND, OR, XOR, SLL
## Precondiciones:
- Reiniciar el emulador (cerrar y volver a abrir en la Terminal 1) para que el PC arranque en 0x0.
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
Anduvo. Las instrucciones se ejecutaron en cadena correctamente validando el pipeline de la CPU.


---

# Caso 3
## Descripción: Test de Memoria y nuevo comando examine
## Instrucctions: SW, LW, SB, LBU
## Precondiciones:
- Reiniciar el emulador (cerrar y volver a abrir en la Terminal 1) para que el PC arranque en 0x0.
## Code
```text
set [0x0] 0x08020100
set [0x4] 0x0805BEEF
set [0x8] 0x48440000
set [0xC] 0x40460000
step 4
r
x /xw 0x100 1
```
## Postcondiciones:
- R3 debe contener el valor guardado por R2.
- El comando examine debe devolver el contenido de la memoria 0x100 validando que se guardó correctamente.
## Conclusiones:
Anduvo. Las instrucciones se ejecutaron en cadena correctamente validando el pipeline de la CPU.
