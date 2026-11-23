#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

const char* vertexShaderSource = R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoord;
out vec3 FragPos;
out vec3 Normal;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    TexCoord = aTexCoord;
    gl_Position = projection * view * vec4(FragPos, 1.0);
}
)";

const char* fragmentShaderSource = R"(#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;

uniform vec3 objectColor;
uniform sampler2D ourTexture;
uniform bool useTexture;

uniform bool enableLighting;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform float ambientStrength;
uniform float specularStrength;
uniform int shininess;

void main()
{
    vec4 baseColor = useTexture ? texture(ourTexture, TexCoord) : vec4(objectColor, 1.0);

    if (!enableLighting)
    {
        FragColor = baseColor;
        return;
    }

    vec3 ambient = ambientStrength * lightColor;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * lightColor;

    vec3 result = (ambient + diffuse + specular) * baseColor.rgb;
    FragColor = vec4(result, baseColor.a);
}
)";

glm::vec3 cameraPos = glm::vec3(12.0f, 12.0f, 8.0f);
glm::vec3 cameraFront = glm::normalize(glm::vec3(0.0f, 0.0f, 1.0f) - glm::vec3(12.0f, 12.0f, 8.0f));
glm::vec3 cameraUp = glm::vec3(0.0f, 0.0f, 1.0f);

float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool firstMouse = true;
float lastX = 400.0f;
float lastY = 300.0f;
float yaw = -135.0f;
float pitch = -25.0f;

bool menuMode = false;
bool toggleKeyPressed = false;

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

glm::vec3 getBezierCirclePoint(glm::vec3 center, float radius, float t)
{
    float k = (4.0f / 3.0f) * radius;
    if (t <= 0.5f)
    {
        float localT = t * 2.0f;
        glm::vec3 p0 = center + glm::vec3(0.0f, radius, 0.0f);
        glm::vec3 p1 = center + glm::vec3(k, radius, 0.0f);
        glm::vec3 p2 = center + glm::vec3(k, -radius, 0.0f);
        glm::vec3 p3 = center + glm::vec3(0.0f, -radius, 0.0f);
        return getBezierPoint(p0, p1, p2, p3, localT);
    }
    else
    {
        float localT = (t - 0.5f) * 2.0f;
        glm::vec3 q0 = center + glm::vec3(0.0f, -radius, 0.0f);
        glm::vec3 q1 = center + glm::vec3(-k, -radius, 0.0f);
        glm::vec3 q2 = center + glm::vec3(-k, radius, 0.0f);
        glm::vec3 q3 = center + glm::vec3(0.0f, radius, 0.0f);
        return getBezierPoint(q0, q1, q2, q3, localT);
    }
}

void setupTriangleWithNormal(unsigned int vao, unsigned int vbo, glm::vec3 v0, glm::vec2 uv0, glm::vec3 v1, glm::vec2 uv1, glm::vec3 v2, glm::vec2 uv2)
{
    glm::vec3 normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));
    glm::vec3 faceCenter = (v0 + v1 + v2) / 3.0f;
    if (glm::dot(normal, faceCenter) < 0.0f)
        normal = -normal;

    float vertices[] = {
        v0.x, v0.y, v0.z,  uv0.x, uv0.y,  normal.x, normal.y, normal.z,
        v1.x, v1.y, v1.z,  uv1.x, uv1.y,  normal.x, normal.y, normal.z,
        v2.x, v2.y, v2.z,  uv2.x, uv2.y,  normal.x, normal.y, normal.z
    };

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);
}

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
        for (size_t i = 0; i < shape.mesh.indices.size(); i += 3) {
            tinyobj::index_t idx[3] = {
                shape.mesh.indices[i],
                shape.mesh.indices[i + 1],
                shape.mesh.indices[i + 2]
            };

            glm::vec3 v[3];
            for (int k = 0; k < 3; k++) {
                v[k] = glm::vec3(
                    attrib.vertices[3 * idx[k].vertex_index + 0],
                    attrib.vertices[3 * idx[k].vertex_index + 1],
                    attrib.vertices[3 * idx[k].vertex_index + 2]
                );
            }

            glm::vec3 fallbackNormal = glm::normalize(glm::cross(v[1] - v[0], v[2] - v[0]));

            for (int k = 0; k < 3; k++) {
                bufferData.push_back(v[k].x);
                bufferData.push_back(v[k].y);
                bufferData.push_back(v[k].z);

                if (idx[k].texcoord_index >= 0) {
                    bufferData.push_back(attrib.texcoords[2 * idx[k].texcoord_index + 0]);
                    bufferData.push_back(attrib.texcoords[2 * idx[k].texcoord_index + 1]);
                }
                else {
                    bufferData.push_back(0.0f);
                    bufferData.push_back(0.0f);
                }

                if (idx[k].normal_index >= 0) {
                    bufferData.push_back(attrib.normals[3 * idx[k].normal_index + 0]);
                    bufferData.push_back(attrib.normals[3 * idx[k].normal_index + 1]);
                    bufferData.push_back(attrib.normals[3 * idx[k].normal_index + 2]);
                }
                else {
                    bufferData.push_back(fallbackNormal.x);
                    bufferData.push_back(fallbackNormal.y);
                    bufferData.push_back(fallbackNormal.z);
                }
            }
        }
    }

    SimpleMesh mesh;
    mesh.vertexCount = bufferData.size() / 8;

    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);

    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, bufferData.size() * sizeof(float), bufferData.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    return mesh;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS)
    {
        if (!toggleKeyPressed)
        {
            menuMode = !menuMode;
            if (menuMode)
            {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            }
            else
            {
                glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
                firstMouse = true;
            }
            toggleKeyPressed = true;
        }
    }
    else
    {
        toggleKeyPressed = false;
    }

    if (menuMode) return;

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
    if (menuMode) return;

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

    GLFWwindow* window = glfwCreateWindow(1000, 750, "Piramide 3D - Dear ImGui + Phong + Bezier", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    glViewport(0, 0, 1000, 750);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    unsigned int triVAO[4], triVBO[4];
    glGenVertexArrays(4, triVAO);
    glGenBuffers(4, triVBO);

    setupTriangleWithNormal(triVAO[0], triVBO[0], { 0, 0, 3 }, { 0.5f, 1 }, { 0, 4, 0 }, { 1, 0 }, { 4,  0, 0 }, { 0, 0 });
    setupTriangleWithNormal(triVAO[1], triVBO[1], { 0, 0, 3 }, { 0.5f, 1 }, { 0, 4, 0 }, { 1, 0 }, { -4,  0, 0 }, { 0, 0 });
    setupTriangleWithNormal(triVAO[2], triVBO[2], { 0, 0, 3 }, { 0.5f, 1 }, { 4, 0, 0 }, { 1, 0 }, { 0, -4, 0 }, { 0, 0 });
    setupTriangleWithNormal(triVAO[3], triVBO[3], { 0, 0, 3 }, { 0.5f, 1 }, { -4, 0, 0 }, { 1, 0 }, { 0, -4, 0 }, { 0, 0 });

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
    unsigned int enableLightLoc = glGetUniformLocation(prog, "enableLighting");

    glm::vec3 circleCenter = glm::vec3(0.0f, 0.0f, 0.0f);
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

    float bgColor[3] = { 0.15f, 0.05f, 0.25f };
    float pyramidRotSpeed = 80.0f;
    float currentPyramidAngle = 0.0f;
    bool pauseRotation = false;

    float dwarfPos[3] = { 0.0f, 0.0f, 7.0f };
    float dwarfScale = 5.5f;
    float dwarfRotZ = 0.0f;

    float bezierColor[3] = { 1.0f, 1.0f, 0.0f };
    float bezierHeightZ = 9.0f;

    bool enableLighting = true;
    float bezierT = 0.0f;
    bool autoMoveLight = false;
    float lightOrbitSpeed = 0.15f;
    glm::vec3 lightPos = glm::vec3(0.0f);
    float lightColor[3] = { 1.0f, 1.0f, 1.0f };
    float ambientStrength = 0.25f;
    float specularStrength = 0.6f;
    int shininess = 32;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Panel de Control - Parcial OpenGL");
        ImGui::Text("Presiona [M] para alternar Mouse / Camara");
        ImGui::Text("Modo actual: %s", menuMode ? "INTERFAZ (Mouse libre)" : "CAMARA (WASD + Mouse)");
        ImGui::Separator();

        if (ImGui::CollapsingHeader("1. Piramide y Escena", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox("Pausar Rotacion Piramide", &pauseRotation);
            ImGui::SliderFloat("Velocidad Rotacion", &pyramidRotSpeed, -300.0f, 300.0f, "%.1f deg/s");
            ImGui::ColorEdit3("Color de Fondo", bgColor);
        }

        if (ImGui::CollapsingHeader("2. Modelo 3D (Enanito)", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::SliderFloat3("Posicion (X, Y, Z)", dwarfPos, -15.0f, 15.0f);
            ImGui::SliderFloat("Escala", &dwarfScale, 0.5f, 15.0f);
            ImGui::SliderFloat("Rotacion Z", &dwarfRotZ, 0.0f, 360.0f);
        }

        if (ImGui::CollapsingHeader("3. Curva de Bezier", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::ColorEdit3("Color Curva", bezierColor);
            ImGui::SliderFloat("Altura Z Curva", &bezierHeightZ, 0.0f, 20.0f);
        }

        if (ImGui::CollapsingHeader("4. Camara", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Text("Pos: (%.1f, %.1f, %.1f)", cameraPos.x, cameraPos.y, cameraPos.z);
            if (ImGui::Button("Reiniciar Camara"))
            {
                cameraPos = glm::vec3(12.0f, 12.0f, 8.0f);
                yaw = -135.0f;
                pitch = -25.0f;
                glm::vec3 front;
                front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
                front.y = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
                front.z = sin(glm::radians(pitch));
                cameraFront = glm::normalize(front);
            }
        }

        if (ImGui::CollapsingHeader("5. Iluminacion Phong (Sobre Bezier)", ImGuiTreeNodeFlags_DefaultOpen))
        {
            ImGui::Checkbox("Activar Luz Phong", &enableLighting);
            ImGui::Checkbox("Animar Luz en Curva", &autoMoveLight);
            if (autoMoveLight)
            {
                ImGui::SliderFloat("Velocidad Orbita", &lightOrbitSpeed, 0.05f, 1.0f);
            }
            ImGui::SliderFloat("Posicion en Bezier (t)", &bezierT, 0.0f, 1.0f, "%.3f");
            ImGui::Text("Coord Luz: (%.1f, %.1f, %.1f)", lightPos.x, lightPos.y, lightPos.z);
            ImGui::ColorEdit3("Color Luz", lightColor);
            ImGui::SliderFloat("Ambiental", &ambientStrength, 0.0f, 1.0f);
            ImGui::SliderFloat("Especular", &specularStrength, 0.0f, 2.0f);
            ImGui::SliderInt("Brillo (Shininess)", &shininess, 2, 128);
        }

        ImGui::Separator();
        ImGui::Text("Rendimiento: %.1f FPS", io.Framerate);
        ImGui::End();

        glClearColor(bgColor[0], bgColor[1], bgColor[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(prog);

        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);

        glm::mat4 view = glm::lookAt(
            cameraPos,
            cameraPos + cameraFront,
            cameraUp
        );

        float aspect = (display_h > 0) ? (float)display_w / (float)display_h : 1.0f;
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.0f);

        glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));

        if (autoMoveLight)
        {
            bezierT += deltaTime * lightOrbitSpeed;
            if (bezierT > 1.0f) bezierT -= 1.0f;
        }

        glm::vec3 actualBezierCenter = glm::vec3(0.0f, 0.0f, bezierHeightZ);
        lightPos = getBezierCirclePoint(actualBezierCenter, circleRadius, bezierT);

        glUniform3fv(glGetUniformLocation(prog, "lightPos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(prog, "viewPos"), 1, glm::value_ptr(cameraPos));
        glUniform3fv(glGetUniformLocation(prog, "lightColor"), 1, lightColor);
        glUniform1f(glGetUniformLocation(prog, "ambientStrength"), ambientStrength);
        glUniform1f(glGetUniformLocation(prog, "specularStrength"), specularStrength);
        glUniform1i(glGetUniformLocation(prog, "shininess"), shininess);

        if (!pauseRotation)
        {
            currentPyramidAngle += deltaTime * glm::radians(pyramidRotSpeed);
        }
        glm::mat4 systemRotation = glm::rotate(glm::mat4(1.0f), currentPyramidAngle, glm::vec3(0.0f, 0.0f, 1.0f));

        glUniform1i(enableLightLoc, false);
        glUniform1i(useTexLoc, false);
        glBindVertexArray(axisVAO);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(systemRotation));
        glUniform3f(colorLoc, 0.6f, 0.6f, 0.6f);
        glDrawArrays(GL_LINES, 0, 6);

        glUniform1i(enableLightLoc, enableLighting);

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(systemRotation));
        for (int i = 0; i < 4; i++)
        {
            glUniform1i(useTexLoc, true);
            glActiveTexture(GL_TEXTURE0);
            if (i == 2) glBindTexture(GL_TEXTURE_2D, texture1);
            else if (i == 1) glBindTexture(GL_TEXTURE_2D, texture2);
            else if (i == 0) glBindTexture(GL_TEXTURE_2D, texture3);
            else glBindTexture(GL_TEXTURE_2D, texture4);

            glBindVertexArray(triVAO[i]);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }

        glm::mat4 dwarfModel = glm::mat4(1.0f);
        dwarfModel = glm::translate(dwarfModel, glm::vec3(dwarfPos[0], dwarfPos[1], dwarfPos[2]));
        dwarfModel = glm::rotate(dwarfModel, glm::radians(dwarfRotZ), glm::vec3(0.0f, 0.0f, 1.0f));
        dwarfModel = glm::rotate(dwarfModel, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        dwarfModel = glm::scale(dwarfModel, glm::vec3(dwarfScale));

        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(dwarfModel));
        glUniform1i(useTexLoc, true);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureEnanito);

        glBindVertexArray(miModelo.vao);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(miModelo.vertexCount));
        glBindVertexArray(0);

        glUniform1i(enableLightLoc, false);
        glm::mat4 bezierModel = glm::mat4(1.0f);
        bezierModel = glm::translate(bezierModel, glm::vec3(0.0f, 0.0f, bezierHeightZ));
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(bezierModel));

        glUniform1i(useTexLoc, false);
        glUniform3f(colorLoc, bezierColor[0], bezierColor[1], bezierColor[2]);

        glBindVertexArray(bezierVAO);
        glDrawArrays(GL_LINE_STRIP, 0, bezierVertexCount);
        glBindVertexArray(0);

        glPointSize(14.0f);
        glm::mat4 lightMarkerModel = glm::translate(glm::mat4(1.0f), lightPos);
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(lightMarkerModel));
        glUniform3fv(colorLoc, 1, lightColor);
        glBindVertexArray(axisVAO);
        glDrawArrays(GL_POINTS, 0, 1);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

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