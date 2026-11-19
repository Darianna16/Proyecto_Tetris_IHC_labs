#pragma once

void dibujarPanel();          // dibuja todo el panel derecho y el marco del tablero
void panelTecla(int key);     // registra P y R para el destello de los botones

// ---------- Funciones de dibujo de Persona 1 (definidas en main.cpp) ----------
void dibujarCuadrado(float x, float y, float w, float h, float r, float g, float b);
void dibujarTriangulo(float x1, float y1, float x2, float y2, float x3, float y3,
                      float r, float g, float b);
void dibujarLinea(float x1, float y1, float x2, float y2, float r, float g, float b);
void dibujarPunto(float x, float y, float tam, float r, float g, float b);
