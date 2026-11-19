// =====================================================================
//  panel.cpp  -  Panel lateral de estado y control (Persona 3)
//
//  Elementos (de arriba hacia abajo, todos en coordenadas normalizadas):
//    1. Vista previa de la siguiente pieza (recuadro con GL_LINES)
//    2. Puntaje con display de 7 segmentos (rectángulos = GL_TRIANGLES)
//    3. Barra de progreso de líneas hacia el siguiente nivel
//    4. Nivel actual con puntos (GL_POINTS)
//    5. Indicador de estado: círculo verde / amarillo / rojo
//    6. Botones de Pausa (P) y Reiniciar (R) con íconos triangulares
//  Además: marco del tablero del color del estado y aviso de pausa.
// =====================================================================
#include "panel.h"
#include "juego.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>   // glfwGetTime() y constantes GLFW_KEY_*
#include <cmath>

// ---------- Paleta de la interfaz ----------
// Fondo de la ventana: (0.10, 0.12, 0.15). El panel es un poco más claro
// para agruparlo visualmente; los estados usan el código de semáforo.
static const float PANEL[3]    = {0.15f, 0.17f, 0.21f};  // fondo del panel
static const float MARCO[3]    = {0.45f, 0.48f, 0.53f};  // bordes neutros
static const float BOTON[3]    = {0.25f, 0.28f, 0.33f};  // botón en reposo
static const float BOTON_OFF[3]= {0.18f, 0.20f, 0.24f};  // botón desactivado
static const float APAGADO[3]  = {0.28f, 0.31f, 0.36f};  // segmentos/barras vacías
static const float BLANCO[3]   = {0.95f, 0.95f, 0.95f};
static const float OSCURO[3]   = {0.10f, 0.12f, 0.15f};  // ícono sobre fondo claro
static const float GRIS[3]     = {0.45f, 0.47f, 0.50f};  // ícono desactivado
static const float VERDE[3]    = {0.20f, 0.85f, 0.35f};  // JUGANDO
static const float AMARILLO[3] = {1.00f, 0.80f, 0.10f};  // PAUSA
static const float ROJO[3]     = {0.95f, 0.25f, 0.30f};  // GAME OVER
static const float CIAN[3]     = {0.00f, 0.80f, 0.90f};  // progreso de líneas

// ---------- Feedback de los botones ----------
static double tPausa = -10.0, tReinicio = -10.0;   // momento de la última pulsación
static const double DESTELLO = 0.25;              // el botón se ilumina 0.25 s

void panelTecla(int key) {
    double t = glfwGetTime();
    if (key == GLFW_KEY_P && estado != GAMEOVER) tPausa = t;   // P no hace nada en game over
    if (key == GLFW_KEY_R) tReinicio = t;
}

// ---------- Utilidades de dibujo ----------
static void rect(float x, float y, float w, float h, const float* c) {
    dibujarCuadrado(x, y, w, h, c[0], c[1], c[2]);
}
static void tri(float x1, float y1, float x2, float y2, float x3, float y3, const float* c) {
    dibujarTriangulo(x1, y1, x2, y2, x3, y3, c[0], c[1], c[2]);
}
// Contorno de un rectángulo con 4 líneas (GL_LINES)
static void marco(float x, float y, float w, float h, const float* c) {
    dibujarLinea(x,     y,     x + w, y,     c[0], c[1], c[2]);
    dibujarLinea(x + w, y,     x + w, y + h, c[0], c[1], c[2]);
    dibujarLinea(x + w, y + h, x,     y + h, c[0], c[1], c[2]);
    dibujarLinea(x,     y + h, x,     y,     c[0], c[1], c[2]);
}
// Círculo como abanico de N triángulos. rx y ry son distintos porque la
// ventana es 600x800: 0.1 en X = 30 px y 0.075 en Y = 30 px.
static void circulo(float cx, float cy, float rx, float ry, const float* c) {
    const int N = 24;
    const float PI2 = 6.2831853f;
    for (int i = 0; i < N; i++) {
        float a0 = PI2 * i / N, a1 = PI2 * (i + 1) / N;
        tri(cx, cy,
            cx + rx * std::cos(a0), cy + ry * std::sin(a0),
            cx + rx * std::cos(a1), cy + ry * std::sin(a1), c);
    }
}

// ---------- Display de 7 segmentos ----------
// Bits: 0=a(arriba) 1=b(der. arriba) 2=c(der. abajo) 3=d(abajo)
//       4=e(izq. abajo) 5=f(izq. arriba) 6=g(centro)
static const unsigned char SEGMENTOS[10] = {
    0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F };

static void digito(int d, float x, float y, const float* c) {
    const float w = 0.07f, h = 0.12f;        // tamaño del dígito
    const float tx = 0.016f, ty = 0.012f;    // grosor (≈5 px en ambos ejes)
    const float hm = h / 2;
    unsigned char s = SEGMENTOS[d];
    if (s & 0x01) rect(x,          y + h - ty,      w,  ty, c);  // a
    if (s & 0x02) rect(x + w - tx, y + hm,          tx, hm, c);  // b
    if (s & 0x04) rect(x + w - tx, y,               tx, hm, c);  // c
    if (s & 0x08) rect(x,          y,               w,  ty, c);  // d
    if (s & 0x10) rect(x,          y,               tx, hm, c);  // e
    if (s & 0x20) rect(x,          y + hm,          tx, hm, c);  // f
    if (s & 0x40) rect(x,          y + hm - ty / 2, w,  ty, c);  // g
}

// ---------- 1. Vista previa de la siguiente pieza ----------
static void dibujarSiguiente() {
    const float bx = 0.25f, by = 0.40f, bw = 0.60f, bh = 0.35f;
    rect(bx, by, bw, bh, OSCURO);
    marco(bx, by, bw, bh, MARCO);

    // Caja que ocupa la pieza en su matriz 4x4 (para centrarla)
    int fMin = 4, fMax = -1, cMin = 4, cMax = -1;
    for (int f = 0; f < 4; f++)
        for (int c = 0; c < 4; c++)
            if (bloqueEn(siguientePieza, 0, f, c)) {
                if (f < fMin) fMin = f;
                if (f > fMax) fMax = f;
                if (c < cMin) cMin = c;
                if (c > cMax) cMax = c;
            }
    const float pw = CELL_W * 0.8f, ph = CELL_H * 0.8f, m = 0.003f;
    float x0   = bx + bw / 2 - (cMax - cMin + 1) * pw / 2;
    float yTop = by + bh / 2 + (fMax - fMin + 1) * ph / 2;
    const float* col = COLORES[siguientePieza + 1];
    for (int f = fMin; f <= fMax; f++)
        for (int c = cMin; c <= cMax; c++)
            if (bloqueEn(siguientePieza, 0, f, c))
                rect(x0 + (c - cMin) * pw + m, yTop - (f - fMin + 1) * ph + m,
                     pw - 2 * m, ph - 2 * m, col);
}

// ---------- 2. Puntaje (6 dígitos) ----------
static void dibujarPuntaje() {
    int p = puntaje > 999999 ? 999999 : puntaje;
    int digitos[6];
    for (int i = 5; i >= 0; i--) { digitos[i] = p % 10; p /= 10; }
    bool inicio = true;   // los ceros a la izquierda se ven apagados
    for (int i = 0; i < 6; i++) {
        if (digitos[i] != 0 || i == 5) inicio = false;
        digito(digitos[i], 0.26f + i * 0.10f, 0.20f, inicio ? APAGADO : BLANCO);
    }
}

// ---------- 3 y 4. Líneas y nivel ----------
static void dibujarProgreso() {
    // Barra de 10 segmentos: líneas hechas en el nivel actual (cada 10 sube)
    int hechas = lineasTotales % 10;
    for (int i = 0; i < 10; i++)
        rect(0.25f + i * 0.061f, 0.08f, 0.05f, 0.04f, i < hechas ? CIAN : APAGADO);

    // Nivel: un punto por nivel (nivel 1 = 1 punto), máximo 10
    int nivel = lineasTotales / 10 + 1;
    if (nivel > 10) nivel = 10;
    for (int i = 0; i < nivel; i++)
        dibujarPunto(0.28f + i * 0.061f, 0.0f, 9.0f, AMARILLO[0], AMARILLO[1], AMARILLO[2]);
}

// ---------- 5. Indicador de estado ----------
static const float* colorEstado() {
    if (estado == JUGANDO) return VERDE;
    if (estado == PAUSA)   return AMARILLO;
    return ROJO;
}
static void dibujarIndicador() {
    circulo(0.55f, -0.20f, 0.11f, 0.0825f, MARCO);      // anillo exterior
    circulo(0.55f, -0.20f, 0.09f, 0.0675f, colorEstado());
}

// ---------- 6. Botones ----------
static void iconoPausa(float cx, float cy, const float* c) {        // ▌▌
    rect(cx - 0.045f, cy - 0.04f, 0.03f, 0.08f, c);
    rect(cx + 0.015f, cy - 0.04f, 0.03f, 0.08f, c);
}
static void iconoPlay(float cx, float cy, const float* c) {         // ▶
    tri(cx - 0.035f, cy - 0.045f, cx - 0.035f, cy + 0.045f, cx + 0.05f, cy, c);
}
static void iconoReiniciar(float cx, float cy, const float* c) {    // ▌◀
    rect(cx - 0.06f, cy - 0.045f, 0.02f, 0.09f, c);
    tri(cx - 0.035f, cy, cx + 0.05f, cy + 0.045f, cx + 0.05f, cy - 0.045f, c);
}

static void dibujarBotones() {
    double t = glfwGetTime();
    const float y = -0.60f, w = 0.27f, h = 0.18f;
    const float xP = 0.25f, xR = 0.58f;

    // Botón P: muestra la acción disponible (pausar o continuar)
    const float* fondo; const float* icono;
    if (estado == GAMEOVER)            { fondo = BOTON_OFF; icono = GRIS;   }  // desactivado
    else if (t - tPausa < DESTELLO)    { fondo = BLANCO;    icono = OSCURO; }  // recién presionado
    else if (estado == PAUSA)          { fondo = AMARILLO;  icono = OSCURO; }  // pausa activa
    else                               { fondo = BOTON;     icono = BLANCO; }
    rect(xP, y, w, h, fondo);
    marco(xP, y, w, h, MARCO);
    if (estado == PAUSA) iconoPlay(xP + w / 2, y + h / 2, icono);   // P = continuar
    else                 iconoPausa(xP + w / 2, y + h / 2, icono);  // P = pausar

    // Botón R: en game over parpadea en rojo para indicar la salida
    bool parpadeo = (estado == GAMEOVER) && ((int)(t * 2) % 2 == 0);
    if (t - tReinicio < DESTELLO) { fondo = BLANCO; icono = OSCURO; }
    else if (parpadeo)            { fondo = ROJO;   icono = BLANCO; }
    else                          { fondo = BOTON;  icono = BLANCO; }
    rect(xR, y, w, h, fondo);
    marco(xR, y, w, h, MARCO);
    iconoReiniciar(xR + w / 2, y + h / 2, icono);
}

// ---------- Marco del tablero y aviso de pausa ----------
static void dibujarMarcoTablero() {
    const float x0 = BOARD_X0, y0 = BOARD_Y0;
    const float x1 = BOARD_X0 + COLS * CELL_W, y1 = BOARD_Y0 + ROWS * CELL_H;
    const float tx = 0.012f, ty = 0.009f;      // grosor ≈ 3.6 px
    const float* c = colorEstado();             // mismo color que el indicador
    rect(x0 - tx, y0 - ty, tx, y1 - y0 + 2 * ty, c);   // izquierda
    rect(x1,      y0 - ty, tx, y1 - y0 + 2 * ty, c);   // derecha
    rect(x0,      y0 - ty, x1 - x0, ty, c);            // abajo
    rect(x0,      y1,      x1 - x0, ty, c);            // arriba

    if (estado == PAUSA) {                      // ícono grande en el centro del tablero
        float cx = (x0 + x1) / 2, cy = (y0 + y1) / 2;
        rect(cx - 0.11f, cy - 0.09f, 0.22f, 0.18f, OSCURO);
        rect(cx - 0.07f, cy - 0.06f, 0.05f, 0.12f, AMARILLO);
        rect(cx + 0.02f, cy - 0.06f, 0.05f, 0.12f, AMARILLO);
    }
}

// ---------- API ----------
void dibujarPanel() {
    rect(0.17f, -0.75f, 0.76f, 1.55f, PANEL);   // fondo del panel (agrupa los controles)
    dibujarMarcoTablero();
    dibujarSiguiente();
    dibujarPuntaje();
    dibujarProgreso();
    dibujarIndicador();
    dibujarBotones();
}
