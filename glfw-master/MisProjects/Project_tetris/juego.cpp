#include "juego.h"
#define GLFW_INCLUDE_NONE  // no necesitamos OpenGL aquí, solo las teclas
#include <GLFW/glfw3.h>   // solo para las constantes GLFW_KEY_* y GLFW_PRESS
#include <algorithm>
#include <iostream>
#include <random>
#include <vector>

// ---------- Variables compartidas ----------
Estado estado = JUGANDO;
int tablero[ROWS][COLS];
int piezaActual = 0, rotacion = 0, posX = 0, posY = 0;
int siguientePieza = 0;
int lineasTotales = 0, puntaje = 0;

// Paleta (fondo oscuro -> colores saturados y claros para buen contraste)
const float COLORES[8][3] = {
    {0.00f, 0.00f, 0.00f},  // 0 vacío (no se dibuja)
    {0.00f, 0.80f, 0.90f},  // 1 I  cian
    {0.25f, 0.45f, 1.00f},  // 2 J  azul
    {1.00f, 0.55f, 0.10f},  // 3 L  naranja
    {1.00f, 0.85f, 0.10f},  // 4 O  amarillo
    {0.30f, 0.85f, 0.30f},  // 5 S  verde
    {0.70f, 0.30f, 0.90f},  // 6 T  morado
    {0.95f, 0.25f, 0.30f},  // 7 Z  rojo
};

// ---------- Las 7 piezas con sus 4 rotaciones (sentido horario) ----------
// Cada rotación es una matriz 4x4 escrita fila por fila (fila 0 = arriba).
const char* PIEZAS[7][4] = {
    { // I
      "...." "XXXX" "...." "....",
      "..X." "..X." "..X." "..X.",
      "...." "...." "XXXX" "....",
      ".X.." ".X.." ".X.." ".X.." },
    { // J
      "X..." "XXX." "...." "....",
      ".XX." ".X.." ".X.." "....",
      "...." "XXX." "..X." "....",
      ".X.." ".X.." "XX.." "...." },
    { // L
      "..X." "XXX." "...." "....",
      ".X.." ".X.." ".XX." "....",
      "...." "XXX." "X..." "....",
      "XX.." ".X.." ".X.." "...." },
    { // O (todas las rotaciones son iguales)
      ".XX." ".XX." "...." "....",
      ".XX." ".XX." "...." "....",
      ".XX." ".XX." "...." "....",
      ".XX." ".XX." "...." "...." },
    { // S
      ".XX." "XX.." "...." "....",
      ".X.." ".XX." "..X." "....",
      "...." ".XX." "XX.." "....",
      "X..." "XX.." ".X.." "...." },
    { // T
      ".X.." "XXX." "...." "....",
      ".X.." ".XX." ".X.." "....",
      "...." "XXX." ".X.." "....",
      ".X.." "XX.." ".X.." "...." },
    { // Z
      "XX.." ".XX." "...." "....",
      "..X." ".XX." ".X.." "....",
      "...." "XX.." ".XX." "....",
      ".X.." "XX.." "X..." "...." },
};

bool bloqueEn(int tipo, int rot, int fila, int col) {
    return PIEZAS[tipo][rot][fila * 4 + col] == 'X';
}

// ---------- Variables internas ----------
static double tiempoActual = 0.0;    // último glfwGetTime() recibido
static double ultimaCaida = 0.0;     // momento de la última caída automática
static std::vector<int> filasLlenas; // filas que se están borrando (animación)
static double finAnimacion = 0.0;    // hasta cuándo parpadean las filas llenas
static const double DURACION_ANIM = 0.20;
static std::mt19937 rng(std::random_device{}());
static std::vector<int> bolsa;       // "bolsa de 7": salen las 7 piezas antes de repetir

static int nivel() { return lineasTotales / 10; }
static double intervaloCaida() {     // más rápido cada 10 líneas
    return std::max(0.10, 0.60 - 0.05 * nivel());
}

static int sacarDeBolsa() {
    if (bolsa.empty()) {
        bolsa = {0, 1, 2, 3, 4, 5, 6};
        std::shuffle(bolsa.begin(), bolsa.end(), rng);
    }
    int p = bolsa.back();
    bolsa.pop_back();
    return p;
}

// ---------- Colisiones ----------
// ¿La pieza 'tipo' con rotación 'rot' cabe con su fila superior en (x, y)?
// En el tablero row 0 es la fila de ABAJO; en la matriz 4x4 la fila 0 es la de ARRIBA.
static bool cabe(int tipo, int rot, int x, int y) {
    for (int f = 0; f < 4; f++)
        for (int c = 0; c < 4; c++) {
            if (!bloqueEn(tipo, rot, f, c)) continue;
            int col = x + c, row = y - f;
            if (col < 0 || col >= COLS || row < 0 || row >= ROWS) return false; // fuera del tablero
            if (tablero[row][col] != 0) return false;                           // choca con una pieza
        }
    return true;
}

// ---------- Nueva pieza / game over ----------
static void nuevaPieza() {
    piezaActual = siguientePieza;
    siguientePieza = sacarDeBolsa();
    rotacion = 0;
    posX = COLS / 2 - 2;   // centrada (columna 3)
    posY = ROWS - 1;       // fila superior del tablero
    ultimaCaida = tiempoActual;
    if (!cabe(piezaActual, rotacion, posX, posY)) {
        estado = GAMEOVER;
        std::cout << "GAME OVER | Lineas: " << lineasTotales
                  << " | Puntaje: " << puntaje << " | Presiona R para reiniciar\n";
    }
}

// ---------- Borrar filas ----------
static void eliminarFilasLlenas() {
    // Se recorren de arriba hacia abajo para que los índices no se muevan
    std::sort(filasLlenas.rbegin(), filasLlenas.rend());
    for (int fila : filasLlenas)
        for (int r = fila; r < ROWS - 1; r++)
            for (int c = 0; c < COLS; c++)
                tablero[r][c] = tablero[r + 1][c];   // baja todo lo que está encima
    for (int c = 0; c < COLS; c++) tablero[ROWS - 1][c] = 0;

    static const int PUNTOS[5] = {0, 100, 300, 500, 800};
    int n = (int)filasLlenas.size();
    puntaje += PUNTOS[n] * (nivel() + 1);
    lineasTotales += n;
    std::cout << "Filas borradas: " << n << " | Lineas: " << lineasTotales
              << " | Nivel: " << nivel() << " | Puntaje: " << puntaje << "\n";
    filasLlenas.clear();
}

// ---------- Fijar la pieza cuando ya no puede bajar ----------
static void fijarPieza() {
    for (int f = 0; f < 4; f++)
        for (int c = 0; c < 4; c++)
            if (bloqueEn(piezaActual, rotacion, f, c))
                tablero[posY - f][posX + c] = piezaActual + 1;   // 1..7 = color

    for (int r = 0; r < ROWS; r++) {
        bool llena = true;
        for (int c = 0; c < COLS; c++) if (tablero[r][c] == 0) { llena = false; break; }
        if (llena) filasLlenas.push_back(r);
    }
    if (!filasLlenas.empty()) finAnimacion = tiempoActual + DURACION_ANIM; // feedback: parpadeo
    else nuevaPieza();
}

// ---------- Movimientos ----------
static bool mover(int dx, int dy) {
    if (cabe(piezaActual, rotacion, posX + dx, posY + dy)) {
        posX += dx; posY += dy;
        return true;
    }
    return false;
}

static void rotar() {
    int nueva = (rotacion + 1) % 4;
    // "Wall kick": si choca con una pared, prueba desplazándola un poco
    const int ajustes[] = {0, -1, 1, -2, 2};
    for (int dx : ajustes)
        if (cabe(piezaActual, nueva, posX + dx, posY)) {
            rotacion = nueva; posX += dx;
            return;
        }
}

static void soltar() {          // Espacio: baja de golpe y se fija
    int bajadas = 0;
    while (mover(0, -1)) bajadas++;
    puntaje += 2 * bajadas;
    fijarPieza();
}

// ---------- API pública ----------
void reiniciarJuego() {
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++) tablero[r][c] = 0;
    lineasTotales = 0; puntaje = 0;
    filasLlenas.clear();
    bolsa.clear();
    siguientePieza = sacarDeBolsa();
    estado = JUGANDO;
    nuevaPieza();
    std::cout << "Juego iniciado\n";
}

void iniciarJuego() { reiniciarJuego(); }

void actualizarJuego(double t) {
    tiempoActual = t;
    if (!filasLlenas.empty()) {                 // terminando el parpadeo de filas
        if (t >= finAnimacion) {
            eliminarFilasLlenas();
            nuevaPieza();
        }
        return;
    }
    if (estado != JUGANDO) {                     // en pausa o game over no cae nada
        ultimaCaida = t;
        return;
    }
    if (t - ultimaCaida >= intervaloCaida()) {   // caída automática
        ultimaCaida = t;
        if (!mover(0, -1)) fijarPieza();
    }
}

void teclaJuego(int key, int action) {
    if (estado != JUGANDO || !filasLlenas.empty()) return;
    bool presiona = (action == GLFW_PRESS);
    bool mantiene = (action == GLFW_PRESS || action == GLFW_REPEAT);

    if      (key == GLFW_KEY_LEFT  && mantiene) mover(-1, 0);
    else if (key == GLFW_KEY_RIGHT && mantiene) mover(1, 0);
    else if (key == GLFW_KEY_DOWN  && mantiene) {         // bajar 1 fila
        if (mover(0, -1)) { puntaje += 1; ultimaCaida = tiempoActual; }
        else fijarPieza();
    }
    else if (key == GLFW_KEY_UP    && presiona) rotar();   // una vez por pulsación
    else if (key == GLFW_KEY_SPACE && presiona) soltar();
}

void dibujarJuego() {
    // 1) Piezas fijadas (las filas llenas parpadean en blanco antes de borrarse)
    bool parpadeo = !filasLlenas.empty() && ((int)(tiempoActual * 20) % 2 == 0);
    for (int r = 0; r < ROWS; r++) {
        bool esLlena = std::find(filasLlenas.begin(), filasLlenas.end(), r) != filasLlenas.end();
        for (int c = 0; c < COLS; c++) {
            int v = tablero[r][c];
            if (v == 0) continue;
            if (esLlena && parpadeo) dibujarCelda(c, r, 1.0f, 1.0f, 1.0f);
            else dibujarCelda(c, r, COLORES[v][0], COLORES[v][1], COLORES[v][2]);
        }
    }
    if (estado == GAMEOVER || !filasLlenas.empty()) return;

    const float* col = COLORES[piezaActual + 1];

    // 2) Sombra: muestra dónde caerá la pieza (mismo color, más oscuro)
    int caida = 0;
    while (cabe(piezaActual, rotacion, posX, posY - caida - 1)) caida++;
    for (int f = 0; f < 4; f++)
        for (int c = 0; c < 4; c++)
            if (bloqueEn(piezaActual, rotacion, f, c))
                dibujarCelda(posX + c, posY - caida - f, col[0] * 0.3f, col[1] * 0.3f, col[2] * 0.3f);

    // 3) Pieza actual
    for (int f = 0; f < 4; f++)
        for (int c = 0; c < 4; c++)
            if (bloqueEn(piezaActual, rotacion, f, c))
                dibujarCelda(posX + c, posY - f, col[0], col[1], col[2]);
}
