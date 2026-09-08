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

    // 1. Geometría del triángulo base
    float triVertices[] = {
         0.0f,  0.5f, 0.0f,
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f
    };

    unsigned int triVBO, triVAO;
    glGenVertexArrays(1, &triVAO);
    glGenBuffers(1, &triVBO);
    glBindVertexArray(triVAO);
    glBindBuffer(GL_ARRAY_BUFFER, triVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triVertices), triVertices, GL_STATIC_DRAW);
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

    // Datos de los tres triángulos sobre el eje Y
    float distY[3] = { 0.4f, 0.8f, 1.2f };
    float scales[3] = { 1.5f, 1.0f, 0.5f };
    glm::vec3 colors[3] = {
        glm::vec3(1.0f, 0.2f, 0.2f), // Mayor (cerca al origen)
        glm::vec3(0.2f, 0.9f, 0.3f), // Medio
        glm::vec3(0.2f, 0.4f, 1.0f)  // Menor (en la punta)
    };

    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.08f, 0.08f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        // Cámara fija en X mirando al centro, pero con el eje Z apuntando hacia arriba
        glm::mat4 view = glm::lookAt(
            glm::vec3(5.0f, 0.0f, 0.0f), // Posición en X
            glm::vec3(0.0f, 0.0f, 0.0f), // Mirando al origen
            glm::vec3(0.0f, 0.0f, 1.0f)  // Vector UP: Z es ahora la vertical de la pantalla
        );

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        // Rotación general alrededor del eje Z ... velocidad
        float angleZ = (float)glfwGetTime() * glm::radians(80.0f);
        glm::mat4 systemRotation = glm::rotate(glm::mat4(1.0f), angleZ, glm::vec3(0.0f, 0.0f, 1.0f));

        // 1. Dibujar los palos/ejes del sistema giratorio
        glBindVertexArray(axisVAO);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(systemRotation));
        glUniform3f(colorLoc, 0.6f, 0.6f, 0.6f); // Líneas en gris
        glDrawArrays(GL_LINES, 0, 6);

        // 2. Dibujar los 3 triángulos sobre el brazo Y
        glBindVertexArray(triVAO);
        for (int i = 0; i < 3; i++)
        {
            glm::mat4 model = systemRotation;

            // Posicionar a lo largo del palo menor (Y)
            model = glm::translate(model, glm::vec3(0.0f, distY[i], 0.0f));

            // Alinear paralelos al eje Z (palo largo)
            model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

            // Escalar según su jerarquía de altura
            model = glm::scale(model, glm::vec3(scales[i], scales[i], scales[i]));

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glUniform3fv(colorLoc, 1, glm::value_ptr(colors[i]));

            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(1, &triVAO);
    glDeleteBuffers(1, &triVBO);
    glDeleteVertexArrays(1, &axisVAO);
    glDeleteBuffers(1, &axisVBO);
    glDeleteProgram(shaderProgram);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}