#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

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

glm::vec3 cameraPos = glm::vec3(12.0f, 12.0f, 8.0f);
glm::vec3 cameraFront = glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f) - glm::vec3(12.0f, 12.0f, 8.0f));
glm::vec3 cameraUp = glm::vec3(0.0f, 0.0f, 1.0f);
glm::vec3 getBezierPoint(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, float t)
{
    float u = 1.0f - t;
    float tt = t * t;
    float uu = u * u;
    float uuu = uu * u;
    float ttt = tt * t;

    glm::vec3 p = uuu * p0;
    p += 3.0f * uu * t * p1;
    p += 3.0f * u * tt * p2;
    p += ttt * p3;

    return p;
}

std::vector<float> generateBezierCircleVertices(glm::vec3 center, float radius, int segmentsPerCurve)
{
    std::vector<float> lineVertices;
    float k = (4.0f / 3.0f) * radius;

    // Semicírculo 1 (Lado derecho / positivo de X)
    glm::vec3 p0 = center + glm::vec3(0.0f, radius, 0.0f);
    glm::vec3 p1 = center + glm::vec3(k, radius, 0.0f);
    glm::vec3 p2 = center + glm::vec3(k, -radius, 0.0f);
    glm::vec3 p3 = center + glm::vec3(0.0f, -radius, 0.0f);

    for (int i = 0; i <= segmentsPerCurve; i++)
    {
        float t = (float)i / (float)segmentsPerCurve;
        glm::vec3 pt = getBezierPoint(p0, p1, p2, p3, t);
        lineVertices.push_back(pt.x);
        lineVertices.push_back(pt.y);
        lineVertices.push_back(pt.z);
    }

    // Semicírculo 2 (Lado izquierdo / negativo de X)
    glm::vec3 q0 = center + glm::vec3(0.0f, -radius, 0.0f);
    glm::vec3 q1 = center + glm::vec3(-k, -radius, 0.0f);
    glm::vec3 q2 = center + glm::vec3(-k, radius, 0.0f);
    glm::vec3 q3 = center + glm::vec3(0.0f, radius, 0.0f);

    for (int i = 0; i <= segmentsPerCurve; i++)
    {
        float t = (float)i / (float)segmentsPerCurve;
        glm::vec3 pt = getBezierPoint(q0, q1, q2, q3, t);
        lineVertices.push_back(pt.x);
        lineVertices.push_back(pt.y);
        lineVertices.push_back(pt.z);
    }

    return lineVertices;
}

float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool firstMouse = true;
float lastX = 400.0f;
float lastY = 300.0f;
float yaw = -135.0f;
float pitch = -25.0f;

struct SimpleMesh {
    unsigned int vao = 0;
    unsigned int vbo = 0;
    size_t vertexCount = 0;
};

SimpleMesh loadOBJ(const std::string& path) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str())) {
        std::cerr << "Error al cargar OBJ: " << warn << err << std::endl;
        return {};
    }

    std::vector<float> bufferData;

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            bufferData.push_back(attrib.vertices[3 * index.vertex_index + 0]);
            bufferData.push_back(attrib.vertices[3 * index.vertex_index + 1]);
            bufferData.push_back(attrib.vertices[3 * index.vertex_index + 2]);

            if (index.texcoord_index >= 0) {
                bufferData.push_back(attrib.texcoords[2 * index.texcoord_index + 0]);
                bufferData.push_back(attrib.texcoords[2 * index.texcoord_index + 1]);
            }
            else {
                bufferData.push_back(0.0f);
                bufferData.push_back(0.0f);
            }
        }
    }

    SimpleMesh mesh;
    mesh.vertexCount = bufferData.size() / 5;

    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);

    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, bufferData.size() * sizeof(float), bufferData.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    return mesh;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    float cameraSpeed = 10.0f * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        cameraPos += cameraSpeed * cameraFront;

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        cameraPos -= cameraSpeed * cameraFront;

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        cameraPos += cameraUp * cameraSpeed;

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        cameraPos -= cameraUp * cameraSpeed;
}

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
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

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

    GLFWwindow* window = glfwCreateWindow(800, 600, "Piramide 3D - Textura en Caras", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    glViewport(0, 0, 800, 600);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    float triRojo_Vertices[] = {
        0.00f,  0.00f,  3.00f,   0.50f, 1.0f,
        0.00f,  4.00f,  0.00f,   1.0f,  0.0f,
        4.00f,  0.00f,  0.00f,   0.0f,  0.0f
    };

    float triVerde_Vertices[] = {
        0.00f,  0.00f,  3.00f,   0.50f, 1.0f,
        0.00f,  4.00f,  0.00f,   1.0f,  0.0f,
       -4.00f,  0.00f,  0.00f,   0.0f,  0.0f
    };

    float triAzul_Vertices[] = {
        0.00f,  0.00f,  3.00f,   0.50f, 1.0f,
        4.00f,  0.00f,  0.00f,   1.0f,  0.0f,
        0.00f, -4.00f,  0.00f,   0.0f,  0.0f
    };

    float triAmarillo_Vertices[] = {
        0.00f,  0.00f,  3.00f,   0.50f, 1.0f,
       -4.00f,  0.00f,  0.00f,   1.0f,  0.0f,
        0.00f, -4.00f,  0.00f,   0.0f,  0.0f
    };

    unsigned int triVAO[4], triVBO[4];
    glGenVertexArrays(4, triVAO);
    glGenBuffers(4, triVBO);

    glBindVertexArray(triVAO[0]);
    glBindBuffer(GL_ARRAY_BUFFER, triVBO[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triRojo_Vertices), triRojo_Vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(triVAO[1]);
    glBindBuffer(GL_ARRAY_BUFFER, triVBO[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triVerde_Vertices), triVerde_Vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(triVAO[2]);
    glBindBuffer(GL_ARRAY_BUFFER, triVBO[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triAzul_Vertices), triAzul_Vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(triVAO[3]);
    glBindBuffer(GL_ARRAY_BUFFER, triVBO[3]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triAmarillo_Vertices), triAmarillo_Vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

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
        stbi_image_free(data1);
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
        stbi_image_free(data2);
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
        stbi_image_free(data3);
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
        stbi_image_free(data4);
    }

    unsigned int textureEnanito;
    glGenTextures(1, &textureEnanito);
    glBindTexture(GL_TEXTURE_2D, textureEnanito);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    unsigned char* dataEnanito = stbi_load("barbasEnanito.jpg", &width, &height, &nrChannels, 4);
    if (dataEnanito)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, dataEnanito);
        glGenerateMipmap(GL_TEXTURE_2D);
        stbi_image_free(dataEnanito);
    }


    SimpleMesh miModelo = loadOBJ("enanito.obj");

    glUseProgram(prog);
    glUniform1i(glGetUniformLocation(prog, "ourTexture"), 0);

    unsigned int modelLoc = glGetUniformLocation(prog, "model");
    unsigned int viewLoc = glGetUniformLocation(prog, "view");
    unsigned int projLoc = glGetUniformLocation(prog, "projection");
    unsigned int colorLoc = glGetUniformLocation(prog, "objectColor");
    unsigned int useTexLoc = glGetUniformLocation(prog, "useTexture");


    glm::vec3 circleCenter = glm::vec3(0.0f, 0.0f, 7.0f);
    float circleRadius = 18.0f;
    int segmentsPerCurve = 50;

    std::vector<float> bezierCircleData = generateBezierCircleVertices(circleCenter, circleRadius, segmentsPerCurve);
    int bezierVertexCount = (int)bezierCircleData.size() / 3;

    unsigned int bezierVAO, bezierVBO;
    glGenVertexArrays(1, &bezierVAO);
    glGenBuffers(1, &bezierVBO);

    glBindVertexArray(bezierVAO);
    glBindBuffer(GL_ARRAY_BUFFER, bezierVBO);
    glBufferData(GL_ARRAY_BUFFER, bezierCircleData.size() * sizeof(float), bezierCircleData.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        glClearColor(0.15f, 0.05f, 0.25f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(prog);

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

        glUniform1i(useTexLoc, false);
        glBindVertexArray(axisVAO);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(systemRotation));
        glUniform3f(colorLoc, 0.6f, 0.6f, 0.6f);
        glDrawArrays(GL_LINES, 0, 6);

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(systemRotation));
        for (int i = 0; i < 4; i++)
        {
            if (i == 2)
            {
                glUniform1i(useTexLoc, true);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texture1);
            }
            else if (i == 1)
            {
                glUniform1i(useTexLoc, true);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texture2);
            }
            else if (i == 0)
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

        glm::mat4 dwarfModel = glm::mat4(1.0f);
        dwarfModel = glm::translate(dwarfModel, glm::vec3(0.0f, 0.0f, 7.0f));
        dwarfModel = glm::rotate(dwarfModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        dwarfModel = glm::scale(dwarfModel, glm::vec3(5.5f));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(dwarfModel));
        glUniform1i(useTexLoc, true);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureEnanito);

        glBindVertexArray(miModelo.vao);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(miModelo.vertexCount));
        glBindVertexArray(0);

        glm::mat4 bezierModel = glm::mat4(1.0f); 
        bezierModel = glm::translate(bezierModel, glm::vec3(0.0f, 0.0f, 9.0f));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(bezierModel));

        glUniform1i(useTexLoc, false);
        glUniform3f(colorLoc, 1.0f, 1.0f, 0.0f); 

        glBindVertexArray(bezierVAO);
        glDrawArrays(GL_LINE_STRIP, 0, bezierVertexCount);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glDeleteVertexArrays(4, triVAO);
    glDeleteBuffers(4, triVBO);
    glDeleteVertexArrays(1, &axisVAO);
    glDeleteBuffers(1, &axisVBO);
    glDeleteVertexArrays(1, &miModelo.vao);
    glDeleteBuffers(1, &miModelo.vbo);
    glDeleteTextures(1, &texture1);
    glDeleteTextures(1, &texture2);
    glDeleteTextures(1, &texture3);
    glDeleteTextures(1, &texture4);
    glDeleteTextures(1, &textureEnanito);
    glDeleteVertexArrays(1, &bezierVAO);
    glDeleteBuffers(1, &bezierVBO);
    glDeleteProgram(prog);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}