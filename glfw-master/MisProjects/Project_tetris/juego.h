// =====================================================================
//  juego.h  -  Lógica del Tetris (Persona 2)
//  Variables y funciones COMPARTIDAS por todo el equipo:
//    - Persona 1 (main.cpp) llama a iniciarJuego(), actualizarJuego()
//      y dibujarJuego() dentro del bucle de renderizado.
//    - Persona 3 (panel) lee 'estado' y 'siguientePieza' y usa COLORES.
// =====================================================================
#pragma once

// ---------- Constantes del tablero (antes estaban en main.cpp) ----------
const unsigned SCR_W = 600, SCR_H = 800;
const int   COLS = 10, ROWS = 20;
const float CELL_W = 0.1f, CELL_H = 0.075f;      // celdas cuadradas (30x30 px)
const float BOARD_X0 = -0.9f, BOARD_Y0 = -0.75f; // esquina inferior izquierda

// ---------- Estado del juego ----------
enum Estado { JUGANDO, PAUSA, GAMEOVER };
extern Estado estado;              // Persona 3 lo cambia con P; Persona 2 pone GAMEOVER

extern int tablero[ROWS][COLS];    // 0 = vacío, 1..7 = tipo de pieza fijada
extern int piezaActual, rotacion;  // pieza que está cayendo (0..6) y su rotación (0..3)
extern int posX, posY;             // posición en el tablero de la fila superior de la pieza
extern int siguientePieza;         // Persona 3 la dibuja en la vista previa
extern int lineasTotales, puntaje;

// Paleta: índice 0 = vacío, 1..7 = I, J, L, O, S, T, Z (Persona 3 puede ajustarla)
extern const float COLORES[8][3];

// Forma de las piezas: matriz 4x4 de texto, 'X' = bloque
extern const char* PIEZAS[7][4];
bool bloqueEn(int tipo, int rot, int fila, int col);   // true si hay 'X' en esa celda

// ---------- Funciones del juego ----------
void iniciarJuego();               // se llama una vez antes del bucle
void reiniciarJuego();             // R (Persona 3) y después de un game over
void actualizarJuego(double t);    // caída automática; t = glfwGetTime()
void teclaJuego(int key, int action); // flechas y Espacio (se llama desde el callback)
void dibujarJuego();               // piezas fijadas, sombra y pieza actual

// ---------- Función de dibujo de Persona 1 (definida en main.cpp) ----------
void dibujarCelda(int col, int row, float r, float g, float b);
