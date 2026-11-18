#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

// ---------- Constantes del tablero (compartidas con el equipo) ----------
const unsigned SCR_W = 600, SCR_H = 800;
const int   COLS = 10, ROWS = 20;
const float CELL_W = 0.1f, CELL_H = 0.075f;      // celdas cuadradas en píxeles (600x800)
const float BOARD_X0 = -0.9f, BOARD_Y0 = -0.75f; // esquina inferior izquierda

// ---------- Shaders ----------
const char* vsSrc = R"(#version 330 core
layout (location = 0) in vec3 aPos;
uniform vec2 uPos;    // desplazamiento
uniform vec2 uSize;   // escala
void main() {
    gl_Position = vec4(aPos.xy * uSize + uPos, aPos.z, 1.0);
}
)";
const char* fsSrc = R"(#version 330 core
out vec4 FragColor;
uniform vec3 uColor;
void main() {
    FragColor = vec4(uColor, 1.0);
}
)";

// ---------- Globales ----------
unsigned int prog;
int locColor, locPos, locSize;
unsigned int vaoCuad, vboCuad, eboCuad;           // cuadrado (VAO/VBO/EBO)
unsigned int vaoDin, vboDin;                      // triángulo / línea / punto (dinámico)
unsigned int vaoGrid, vboGrid; int gridVerts = 0; // cuadrícula (GL_LINES)
bool contorno = false;                            // W: relleno / contorno

// ---------- Utilidades ----------
unsigned int compilar(GLenum tipo, const char* src) {
    unsigned int s = glCreateShader(tipo);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    int ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[512]; glGetShaderInfoLog(s, 512, nullptr, log); std::cout << log << "\n"; }
    return s;
}
void setColor(float r, float g, float b) { glUniform3f(locColor, r, g, b); }
void setTransform(float x, float y, float w, float h) {
    glUniform2f(locPos, x, y); glUniform2f(locSize, w, h);
}

// ---------- Funciones de dibujo (API para Persona 2 y 3) ----------
// (x, y) = esquina inferior izquierda; todo en coordenadas normalizadas
void dibujarCuadrado(float x, float y, float w, float h, float r, float g, float b) {
    setColor(r, g, b); setTransform(x, y, w, h);
    glBindVertexArray(vaoCuad);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
void dibujarTriangulo(float x1, float y1, float x2, float y2, float x3, float y3,
                      float r, float g, float b) {
    float v[] = { x1,y1,0,  x2,y2,0,  x3,y3,0 };
    setColor(r, g, b); setTransform(0, 0, 1, 1);
    glBindVertexArray(vaoDin);
    glBindBuffer(GL_ARRAY_BUFFER, vboDin);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}
void dibujarLinea(float x1, float y1, float x2, float y2, float r, float g, float b) {
    float v[] = { x1,y1,0,  x2,y2,0 };
    setColor(r, g, b); setTransform(0, 0, 1, 1);
    glBindVertexArray(vaoDin);
    glBindBuffer(GL_ARRAY_BUFFER, vboDin);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v);
    glDrawArrays(GL_LINES, 0, 2);
}
void dibujarPunto(float x, float y, float tam, float r, float g, float b) {
    float v[] = { x,y,0 };
    setColor(r, g, b); setTransform(0, 0, 1, 1);
    glPointSize(tam);
    glBindVertexArray(vaoDin);
    glBindBuffer(GL_ARRAY_BUFFER, vboDin);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(v), v);
    glDrawArrays(GL_POINTS, 0, 1);
}
// Dibuja la celda (col,row) del tablero. row 0 = fila inferior.
void dibujarCelda(int col, int row, float r, float g, float b) {
    float m = 0.003f; // pequeño margen para que se vea la separación
    dibujarCuadrado(BOARD_X0 + col*CELL_W + m, BOARD_Y0 + row*CELL_H + m,
                    CELL_W - 2*m, CELL_H - 2*m, r, g, b);
}
void dibujarCuadricula(float r, float g, float b) {
    setColor(r, g, b); setTransform(0, 0, 1, 1);
    glBindVertexArray(vaoGrid);
    glDrawArrays(GL_LINES, 0, gridVerts);
}

// ---------- Inicialización de buffers ----------
void initBuffers() {
    // Cuadrado unitario con EBO: 4 vértices + 6 índices (2 triángulos)
    float vc[] = { 1,1,0,  1,0,0,  0,0,0,  0,1,0 };
    unsigned int ic[] = { 0,1,3,  1,2,3 };
    glGenVertexArrays(1, &vaoCuad); glGenBuffers(1, &vboCuad); glGenBuffers(1, &eboCuad);
    glBindVertexArray(vaoCuad);
    glBindBuffer(GL_ARRAY_BUFFER, vboCuad);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vc), vc, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboCuad);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(ic), ic, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Buffer dinámico (reservado para hasta 3 vértices)
    glGenVertexArrays(1, &vaoDin); glGenBuffers(1, &vboDin);
    glBindVertexArray(vaoDin);
    glBindBuffer(GL_ARRAY_BUFFER, vboDin);
    glBufferData(GL_ARRAY_BUFFER, 9*sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Cuadrícula del tablero: líneas verticales y horizontales con GL_LINES
    std::vector<float> g;
    float x1 = BOARD_X0 + COLS*CELL_W, y1 = BOARD_Y0 + ROWS*CELL_H;
    for (int i = 0; i <= COLS; i++) { float x = BOARD_X0 + i*CELL_W;
        g.insert(g.end(), { x, BOARD_Y0, 0,  x, y1, 0 }); }
    for (int j = 0; j <= ROWS; j++) { float y = BOARD_Y0 + j*CELL_H;
        g.insert(g.end(), { BOARD_X0, y, 0,  x1, y, 0 }); }
    gridVerts = (int)g.size() / 3;
    glGenVertexArrays(1, &vaoGrid); glGenBuffers(1, &vboGrid);
    glBindVertexArray(vaoGrid);
    glBindBuffer(GL_ARRAY_BUFFER, vboGrid);
    glBufferData(GL_ARRAY_BUFFER, g.size()*sizeof(float), g.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

// ---------- Teclado ----------
void procesarEntrada(GLFWwindow* w) {
    if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(w, true);
    static bool wPrev = false;
    bool wNow = glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS;
    if (wNow && !wPrev) {                       // solo una vez por pulsación
        contorno = !contorno;
        glPolygonMode(GL_FRONT_AND_BACK, contorno ? GL_LINE : GL_FILL);
    }
    wPrev = wNow;
}

int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    GLFWwindow* w = glfwCreateWindow(SCR_W, SCR_H, "Tetris - Lab 7 IHC", nullptr, nullptr);
    if (!w) { std::cout << "Error ventana\n"; glfwTerminate(); return -1; }
    glfwMakeContextCurrent(w);
    if (!gladLoadGL(glfwGetProcAddress)) return -1;   // GLAD 2

    unsigned int vs = compilar(GL_VERTEX_SHADER, vsSrc);
    unsigned int fs = compilar(GL_FRAGMENT_SHADER, fsSrc);
    prog = glCreateProgram();
    glAttachShader(prog, vs); glAttachShader(prog, fs); glLinkProgram(prog);
    glDeleteShader(vs); glDeleteShader(fs);
    locColor = glGetUniformLocation(prog, "uColor");
    locPos   = glGetUniformLocation(prog, "uPos");
    locSize  = glGetUniformLocation(prog, "uSize");

    initBuffers();

    while (!glfwWindowShouldClose(w)) {
        procesarEntrada(w);
        glClearColor(0.10f, 0.12f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(prog);

        // Escena de prueba (la reemplazan Persona 2 y 3)
        dibujarCuadricula(0.35f, 0.38f, 0.42f);
        dibujarCelda(0, 0, 0.9f, 0.3f, 0.3f);
        dibujarCelda(1, 0, 0.9f, 0.3f, 0.3f);
        dibujarTriangulo(0.4f, 0.5f, 0.6f, 0.5f, 0.5f, 0.65f, 0.2f, 0.8f, 0.4f);
        dibujarLinea(0.3f, 0.2f, 0.8f, 0.2f, 1.0f, 1.0f, 1.0f);
        dibujarPunto(BOARD_X0, BOARD_Y0, 8.0f, 1.0f, 0.9f, 0.1f);

        glfwSwapBuffers(w);
        glfwPollEvents();
    }
    glfwTerminate();
    return 0;
}