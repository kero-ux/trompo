#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Vertex Shader
const char* vertexShaderSource = R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoord;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
    TexCoord = aTexCoord;
}
)";

// Fragment Shader
const char* fragmentShaderSource = R"(#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform vec3 objectColor;
uniform sampler2D ourTexture;
uniform bool useTexture;

void main()
{
    if (useTexture)
        FragColor = texture(ourTexture, TexCoord);
    else
        FragColor = vec4(objectColor, 1.0);
}
)";

// ==========================================
// VARIABLES DE CÁMARA Y TIEMPO
// ==========================================
glm::vec3 cameraPos = glm::vec3(12.0f, 12.0f, 8.0f); // Posición inicial
glm::vec3 cameraFront = glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f) - glm::vec3(12.0f, 12.0f, 8.0f));
glm::vec3 cameraUp = glm::vec3(0.0f, 0.0f, 1.0f);   // Tu eje Z es arriba

// Control de tiempo para movimiento fluido
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// Variables de orientación con el mouse
bool firstMouse = true;
float lastX = 400.0f; // Centro de la pantalla
float lastY = 300.0f;
float yaw = -135.0f; // Orientación horizontal hacia el origen
float pitch = -25.0f;  // Inclinación hacia abajo

// Función para procesar movimiento de teclado
void processInput(GLFWwindow* window)
{
    // Salir con Escape
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float cameraSpeed = 10.0f * deltaTime;

    // W: Avanzar
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;

    // S: Retroceder
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;

    // A: Izquierda
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

    // D: Derecha
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

    // Espacio: Subir en Z
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cameraPos += cameraUp * cameraSpeed;

    // Left Shift: Bajar en Z
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cameraPos -= cameraUp * cameraSpeed;
}

// Función callback para mover la vista con el mouse
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Invertido porque en pantalla Y crece hacia abajo
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // Limitar el cabeceo vertical
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    // Cálculo trigonométrico de dirección donde Z es vertical
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.z = sin(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

int main()
{
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Piramide 3D - Camara FPS", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Bloquear cursor en la ventana y enlazar el callback del ratón
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    glViewport(0, 0, 800, 600);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // Triángulo A: Rojo (Obama)
    float triRojo_Vertices[] = {
        0.00f,  0.00f,  3.00f,  0.50f, 1.0f, // h
        0.00f,  4.00f,  0.00f,  1.0f,  0.0f, // a
        4.00f,  0.00f,  0.00f,  0.0f,  0.0f  // b
    };

    // Triángulo B: Verde (Diomedes)
    float triVerde_Vertices[] = {
        0.00f,  0.00f,  3.00f,  0.50f, 1.0f, // h
        0.00f,  4.00f,  0.00f,  1.0f,  0.0f, // a
       -4.00f,  0.00f,  0.00f,  0.0f,  0.0f  // d
    };

    // Triángulo C: Azul (Millonarios)
    float triAzul_Vertices[] = {
        0.00f,  0.00f,  3.00f,  0.50f, 1.0f, // h
        4.00f,  0.00f,  0.00f,  1.0f,  0.0f, // b
        0.00f, -4.00f,  0.00f,  0.0f,  0.0f  // c
    };

    // Triángulo D: Amarillo (Petrozki)
    float triAmarillo_Vertices[] = {
        0.00f,  0.00f,  3.00f,  0.50f, 1.0f, // h
       -4.00f,  0.00f,  0.00f,  1.0f,  0.0f, // d
        0.00f, -4.00f,  0.00f,  0.0f,  0.0f  // c
    };

    unsigned int triVAO[4], triVBO[4];
    glGenVertexArrays(4, triVAO);
    glGenBuffers(4, triVBO);

    // Configurar VAO/VBOs (todos con stride de 5: 3 pos + 2 UV)
    float* verticesArray[4] = { triRojo_Vertices, triVerde_Vertices, triAzul_Vertices, triAmarillo_Vertices };
    for (int i = 0; i < 4; i++)
    {
        glBindVertexArray(triVAO[i]);
        glBindBuffer(GL_ARRAY_BUFFER, triVBO[i]);
        glBufferData(GL_ARRAY_BUFFER, 15 * sizeof(float), verticesArray[i], GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
    }

    // Geometría de los ejes
    float axisVertices[] = {
        0.0f, 0.0f,  0.0f,   0.0f, 0.0f, 4.0f,
        0.0f, 0.0f, -2.0f,   0.0f, 0.0f, 0.0f,
        0.0f, 0.0f,  0.0f,   0.0f, 3.5f, 0.0f,
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
    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShaderSource, NULL);
    glCompileShader(vs);

    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentShaderSource, NULL);
    glCompileShader(fs);

    unsigned int prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    glDeleteShader(vs);
    glDeleteShader(fs);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    stbi_set_flip_vertically_on_load(true);

    int width, height, nrChannels;

    // Textura 1 (Millonarios)
    unsigned int texture1;
    glGenTextures(1, &texture1);
    glBindTexture(GL_TEXTURE_2D, texture1);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    unsigned char* data1 = stbi_load("millos.png", &width, &height, &nrChannels, 4);
    if (data1)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data1);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data1);
    }

    // Textura 2 (Diomedes)
    unsigned int texture2;
    glGenTextures(1, &texture2);
    glBindTexture(GL_TEXTURE_2D, texture2);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    unsigned char* data2 = stbi_load("Diomedes Dias.jpg", &width, &height, &nrChannels, 4);
    if (data2)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data2);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data2);
    }

    // Textura 3 (Obama)
    unsigned int texture3;
    glGenTextures(1, &texture3);
    glBindTexture(GL_TEXTURE_2D, texture3);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    unsigned char* data3 = stbi_load("obama.jpg", &width, &height, &nrChannels, 4);
    if (data3)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data3);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data3);
    }

    // Textura 4 (Petrozki)
    unsigned int texture4;
    glGenTextures(1, &texture4);
    glBindTexture(GL_TEXTURE_2D, texture4);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    unsigned char* data4 = stbi_load("petrozki.jpg", &width, &height, &nrChannels, 4);
    if (data4)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data4);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(data4);
    }

    // Uniforms
    glUseProgram(prog);
    glUniform1i(glGetUniformLocation(prog, "ourTexture"), 0);

    unsigned int modelLoc = glGetUniformLocation(prog, "model");
    unsigned int viewLoc = glGetUniformLocation(prog, "view");
    unsigned int projLoc = glGetUniformLocation(prog, "projection");
    unsigned int colorLoc = glGetUniformLocation(prog, "objectColor");
    unsigned int useTexLoc = glGetUniformLocation(prog, "useTexture");

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.15f, 0.05f, 0.25f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(prog);

        // Matriz de vista dinámica controlada por teclado y ratón
        glm::mat4 view = glm::lookAt(
            cameraPos,
            cameraPos + cameraFront,
            cameraUp
        );

        glm::mat4 projection = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        float angleZ = (float)glfwGetTime() * glm::radians(80.0f);
        glm::mat4 systemRotation = glm::rotate(glm::mat4(1.0f), angleZ, glm::vec3(0.0f, 0.0f, 1.0f));

        // 1. Dibujar los ejes
        glUniform1i(useTexLoc, false);
        glBindVertexArray(axisVAO);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(systemRotation));
        glUniform3f(colorLoc, 0.6f, 0.6f, 0.6f);
        glDrawArrays(GL_LINES, 0, 6);

        // 2. Dibujar las 4 caras texturizadas
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(systemRotation));
        glUniform1i(useTexLoc, true);
        glActiveTexture(GL_TEXTURE0);

        unsigned int textures[4] = { texture3, texture2, texture1, texture4 };
        for (int i = 0; i < 4; i++)
        {
            glBindTexture(GL_TEXTURE_2D, textures[i]);
            glBindVertexArray(triVAO[i]);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(4, triVAO);
    glDeleteBuffers(4, triVBO);
    glDeleteVertexArrays(1, &axisVAO);
    glDeleteBuffers(1, &axisVBO);
    glDeleteTextures(1, &texture1);
    glDeleteTextures(1, &texture2);
    glDeleteTextures(1, &texture3);
    glDeleteTextures(1, &texture4);
    glDeleteProgram(prog);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}