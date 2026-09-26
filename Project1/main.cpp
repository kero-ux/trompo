#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Vertex Shader
const char* vertexShaderSource = R"(#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

// Fragment Shader
const char* fragmentShaderSource = R"(#version 330 core
out vec4 FragColor;
uniform vec3 objectColor;
void main()
{
    FragColor = vec4(objectColor, 1.0);
}
)";

int main()
{
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Camara en X - Giro sobre Z", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    glEnable(GL_DEPTH_TEST);

   

    // Triángulo A: Rojo 
    float triRojo_Vertices[] = {
        0.00f,  0.00f,  3.00f,  // h
        0.00f,  4.00f,  0.00f,  // a
        4.00f,  0.00f,  0.00f   // b
    };

    // Triángulo B: Verde 
    float triVerde_Vertices[] = {
        0.00f,  0.00f,  3.00f,  // h
        0.00f,  4.00f,  0.00f,  // a
        -4.00f,  0.00f, 0.00f,  // d
    };

    // Triángulo C: Azul 
    float triAzul_Vertices[] = {
        0.00f,  0.00f,  3.00f,   // h
        4.00f,  0.00f,  0.00f,   // b
        0.00f,  -4.00f, 0.00f,   // c
    };

    // Triángulo D: Amarillo 
    float triAmarillo_Vertices[] = {
        0.00f,  0.00f,  3.00f,   // h
       -4.00f,  0.00f,  0.00f,   // d
        0.00f,  -4.00f, 0.00f,   // c
    };

    // VAOs y VBOs independientes para cada triángulo
    unsigned int triVAO[4], triVBO[4];
    glGenVertexArrays(4, triVAO);
    glGenBuffers(4, triVBO);

    // Configurar Triángulo Rojo
    glBindVertexArray(triVAO[0]);
    glBindBuffer(GL_ARRAY_BUFFER, triVBO[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triRojo_Vertices), triRojo_Vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Configurar Triángulo Verde
    glBindVertexArray(triVAO[1]);
    glBindBuffer(GL_ARRAY_BUFFER, triVBO[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triVerde_Vertices), triVerde_Vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Configurar Triángulo Azul
    glBindVertexArray(triVAO[2]);
    glBindBuffer(GL_ARRAY_BUFFER, triVBO[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triAzul_Vertices), triAzul_Vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Configurar Triángulo Amarillo
    glBindVertexArray(triVAO[3]);
    glBindBuffer(GL_ARRAY_BUFFER, triVBO[3]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triAmarillo_Vertices), triAmarillo_Vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 2. Geometría de los ejes rígidos en L/trípode:
    float axisVertices[] = {
        // [Vértices 0 a 1]: Eje Z positivo (ESTE SÍ SE PINTA)
        0.0f, 0.0f,  0.0f,   0.0f, 0.0f, 4.0f,

        // [Vértices 2 a 3]: Eje Z negativo (EXISTE PERO NO SE PINTA)
        0.0f, 0.0f, -2.0f,   0.0f, 0.0f, 0.0f,

        // [Vértices 4 a 5]: Eje Y (brazo de los triángulos)
        0.0f, 0.0f,  0.0f,   0.0f, 3.5f, 0.0f,

        // [Vértices 6 a 7]: Eje X (EXISTE PERO NO SE PINTA)
        0.0f, 0.0f,  0.0f,   2.5f, 0.0f, 0.0f
    };

    unsigned int axisVBO, axisVAO;
    glGenVertexArrays(1, &axisVAO);
    glGenBuffers(1, &axisVBO);
    glBindVertexArray(axisVAO);
    glBindBuffer(GL_ARRAY_BUFFER, axisVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axisVertices), axisVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Shaders
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    unsigned int modelLoc = glGetUniformLocation(shaderProgram, "model");
    unsigned int viewLoc = glGetUniformLocation(shaderProgram, "view");
    unsigned int projLoc = glGetUniformLocation(shaderProgram, "projection");
    unsigned int colorLoc = glGetUniformLocation(shaderProgram, "objectColor");

    glm::vec3 colors[4] = {
        glm::vec3(1.0f, 0.2f, 0.2f), // Mayor (rojo)
        glm::vec3(0.2f, 0.9f, 0.3f), // Medio (verde)
        glm::vec3(0.2f, 0.4f, 1.0f),  // Menor (azul)
        glm::vec3(1.0f, 1.0f, 0.0f)  // Menor (azul)
    };

    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.08f, 0.08f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // Cámara fija en X mirando al centro, pero con el eje Z apuntando hacia arriba
        glm::mat4 view = glm::lookAt(
            glm::vec3(8.0f, 0.0f, 5.0f), // Posición en X
            glm::vec3(0.0f, 0.0f, 0.0f), // Mirando al origen
            glm::vec3(0.0f, 0.0f, 0.50f)  // Vector UP: Z es ahora la vertical de la pantalla
        );

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Rotación general idéntica alrededor del eje Z (la L gira como trompo)
        float angleZ = (float)glfwGetTime() * glm::radians(80.0f);
        glm::mat4 systemRotation = glm::rotate(glm::mat4(1.0f), angleZ, glm::vec3(0.0f, 0.0f, 1.0f));

        // 1. Dibujar los palos/ejes del sistema giratorio
        glBindVertexArray(axisVAO);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(systemRotation));
        glUniform3f(colorLoc, 0.6f, 0.6f, 0.6f); // Líneas en gris
        glDrawArrays(GL_LINES, 0, 6);

        // 2. Dibujar los 3 triángulos independientes (comparten la misma matriz rígida del sistema)
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(systemRotation));
        for (int i = 0; i < 4; i++)
        {
            glBindVertexArray(triVAO[i]);
            glUniform3fv(colorLoc, 1, glm::value_ptr(colors[i]));
            glDrawArrays(GL_TRIANGLES, 0, 4);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(3, triVAO);
    glDeleteBuffers(3, triVBO);
    glDeleteVertexArrays(1, &axisVAO);
    glDeleteBuffers(1, &axisVBO);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}