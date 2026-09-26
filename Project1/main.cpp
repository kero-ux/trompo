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

int main()
{
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Piramide 3D - Textura en Cara Azul", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    glViewport(0, 0, 800, 600);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);


    // Triángulo A: Rojo 
    float triRojo_Vertices[] = {
        0.00f,  0.00f,  3.00f,  0.50f, 1.0f, // h
        0.00f,  4.00f,  0.00f,  1.0f,  0.0f, // a
        4.00f,  0.00f,  0.00f,  0.0f, 0.0f   // b
    };

    // Triángulo B: Verde 
    float triVerde_Vertices[] = {
        0.00f,  0.00f,  3.00f,   0.50f, 1.0f, // h
        0.00f,  4.00f,  0.00f,   1.0f,  0.0f, // a
       -4.00f,  0.00f,  0.00f,   0.0f, 0.0f   // d
    };

    // Triángulo C: Azul (con coordenadas UV para millos.png)
    float triAzul_Vertices[] = {
        0.00f,  0.00f,  3.00f,   0.50f, 1.0f, // h
        4.00f,  0.00f,  0.00f,   1.0f, 0.0f,  // b
        0.00f, -4.00f,  0.00f,   0.0f, 0.0f   // c
    };

    // Triángulo D: Amarillo 
    float triAmarillo_Vertices[] = {
        0.00f,  0.00f,  3.00f,   0.50f, 1.0f, // h
       -4.00f,  0.00f,  0.00f,   1.0f, 0.0f,  // d
        0.00f, -4.00f,  0.00f,   0.0f, 0.0f   // c
    };

    // VAOs y VBOs independientes para cada triángulo
    unsigned int triVAO[4], triVBO[4];
    glGenVertexArrays(4, triVAO);
    glGenBuffers(4, triVBO);

    // Configurar Triángulo Rojo (stride de 3)
    glBindVertexArray(triVAO[0]);
    glBindBuffer(GL_ARRAY_BUFFER, triVBO[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triRojo_Vertices), triRojo_Vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Configurar Triángulo Verde (stride de 3)
    glBindVertexArray(triVAO[1]);
    glBindBuffer(GL_ARRAY_BUFFER, triVBO[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triVerde_Vertices), triVerde_Vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Configurar Triángulo Azul (stride de 5 por las coordenadas de textura)
    glBindVertexArray(triVAO[2]);
    glBindBuffer(GL_ARRAY_BUFFER, triVBO[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triAzul_Vertices), triAzul_Vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Configurar Triángulo Amarillo (stride de 3)
    glBindVertexArray(triVAO[3]);
    glBindBuffer(GL_ARRAY_BUFFER, triVBO[3]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triAmarillo_Vertices), triAmarillo_Vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // 2. Geometría de los ejes rígidos:
    float axisVertices[] = {
        // [Vértices 0 a 1]: Eje Z positivo
        0.0f, 0.0f,  0.0f,   0.0f, 0.0f, 4.0f,

        // [Vértices 2 a 3]: Eje Z negativo
        0.0f, 0.0f, -2.0f,   0.0f, 0.0f, 0.0f,

        // [Vértices 4 a 5]: Eje Y
        0.0f, 0.0f,  0.0f,   0.0f, 3.5f, 0.0f,

        // [Vértices 6 a 7]: Eje X
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

    // Compilación de shaders
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
        std::cout << "millos.png cargado correctamente." << std::endl;
        stbi_image_free(data1);
    }
    else
    {
        std::cout << "Error al cargar millos.png" << std::endl;
    }


    unsigned int texture2;
    glGenTextures(1, &texture2);
    glBindTexture(GL_TEXTURE_2D, texture2);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    
    unsigned char* data2 = stbi_load("Diomedes Dias.jpg", &width, &height, &nrChannels, 0);
    if (data2)
    {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data2);
        glGenerateMipmap(GL_TEXTURE_2D);
        std::cout << "el cacique fue cargado correctamente." << std::endl;
        stbi_image_free(data2);
    }
    else
    {
        std::cout << "Error al cargar diomedes.jpg" << std::endl;
    }

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
        std::cout << "obama.jpg cargado correctamente." << std::endl;
        stbi_image_free(data3);
    }
    else
    {
        std::cout << "Error al cargar obama.jpg" << std::endl;
    }

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
        std::cout << "petrozki.jpg cargado correctamente." << std::endl;
        stbi_image_free(data4);
    }
    else
    {
        std::cout << "Error al cargar petrozki.jpg" << std::endl;
    }

    // Uniforms
    glUseProgram(prog);
    glUniform1i(glGetUniformLocation(prog, "ourTexture"), 0);

    unsigned int modelLoc = glGetUniformLocation(prog, "model");
    unsigned int viewLoc = glGetUniformLocation(prog, "view");
    unsigned int projLoc = glGetUniformLocation(prog, "projection");
    unsigned int colorLoc = glGetUniformLocation(prog, "objectColor");
    unsigned int useTexLoc = glGetUniformLocation(prog, "useTexture");

    glm::vec3 colors[4] = {
        glm::vec3(1.0f, 0.2f, 0.2f), // Rojo
        glm::vec3(0.2f, 0.9f, 0.3f), // Verde
        glm::vec3(0.2f, 0.4f, 1.0f), // Azul
        glm::vec3(1.0f, 1.0f, 0.0f)  // Amarillo
    };

    while (!glfwWindowShouldClose(window))
    {
        glClearColor(0.15f, 0.05f, 0.25f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(prog);

        glm::mat4 view = glm::lookAt(
            glm::vec3(12.0f, 12.0f, 8.0f),
            glm::vec3(0.0f, 0.0f, 1.0f),
            glm::vec3(0.0f, 0.0f, 1.0f)
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

        // 2. Dibujar las 4 caras independientes
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(systemRotation));
        for (int i = 0; i < 4; i++)
        {
            if (i == 2) // millos
            {
                glUniform1i(useTexLoc, true);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texture1);
            }
            else if (i == 1) // diomedes
            {
                glUniform1i(useTexLoc, true);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texture2);
            }
            else if (i == 0) // obama
            {
                glUniform1i(useTexLoc, true);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texture3);
            }
            else
            {
                glUniform1i(useTexLoc, true);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texture4);
            }

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