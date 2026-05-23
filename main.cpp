#include <iostream>
#include <cmath>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include "stb_image.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

const GLint WIDTH = 1080, HEIGHT = 720;

int direction = 1;

GLFWwindow* g_window = nullptr;
int g_gl_width = 1080;
int g_gl_height = 720;

float posX = 200, posY = HEIGHT - 500;
float scale = 1.0f;
float angle = 0.0f;
int filterMode = 0;

float cameraX = 0.0f;
float cameraY = 0.0f;

bool load_texture(const char* file_name, GLuint* tex) {
    int x, y, n;
    unsigned char* data = stbi_load(file_name, &x, &y, &n, 4);

    if (!data) {
        std::cout << "Erro ao carregar textura: " << file_name << std::endl;
        return false;
    }

    glGenTextures(1, tex);
    glBindTexture(GL_TEXTURE_2D, *tex);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);

    return true;
}

void desenharCamada(GLuint textura, GLuint shader, GLuint VAO, float fatorX, float fatorY, float cameraX, float cameraY) {
    float offsetX = std::fmod(-cameraX * fatorX, (float)WIDTH);
    float offsetY = -cameraY * fatorY;

    for (int i = -1; i <= 1; i++) {
        glm::mat4 model = glm::mat4(1.0f);

        model = glm::translate(model, glm::vec3(offsetX + i * WIDTH, offsetY, 0));
        model = glm::scale(model, glm::vec3(WIDTH, HEIGHT, 1));

        glBindTexture(GL_TEXTURE_2D, textura);
        glUniform1i(glGetUniformLocation(shader, "isSticker"), 0);
        glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
}

int main() {
    if (!glfwInit()) {
        std::cout << "Erro ao iniciar GLFW" << std::endl;
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Parallax", NULL, NULL);

    if (!window) {
        std::cout << "Erro ao criar janela GLFW" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        std::cout << "Erro ao iniciar GLEW" << std::endl;
        return -1;
    }

    const char* vs =
        "#version 410\n"
        "layout (location=0) in vec3 pos;"
        "layout (location=1) in vec2 tex;"
        "uniform mat4 proj;"
        "uniform mat4 model;"
        "out vec2 TexCoord;"
        "void main(){"
        "gl_Position = proj * model * vec4(pos,1.0);"
        "TexCoord = tex;"
        "}";

    const char* fs =
        "#version 410\n"
        "in vec2 TexCoord;"
        "uniform sampler2D tex;"
        "uniform int isSticker;"
        "uniform int filterMode;"
        "out vec4 FragColor;"
        "void main(){"
        "vec4 color = texture(tex, TexCoord);"
        "if(isSticker==1 && color.a < 0.1) discard;"
        "if(filterMode==1){"
        "float g=(color.r+color.g+color.b)/3.0;"
        "color=vec4(g,g,g,color.a);}"
        "else if(filterMode==2){"
        "color=vec4(1.0-color.rgb,color.a);}"
        "FragColor = color;"
        "}";

    GLuint vshader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vshader, 1, &vs, NULL);
    glCompileShader(vshader);

    GLuint fshader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fshader, 1, &fs, NULL);
    glCompileShader(fshader);

    GLuint shader = glCreateProgram();
    glAttachShader(shader, vshader);
    glAttachShader(shader, fshader);
    glLinkProgram(shader);

    float vertices[] = {
        0, 0, 0,       0, 0,
        1, 0, 0,       1, 0,
        1, 1, 0,       1, 1,
        0, 0, 0,       0, 0,
        1, 1, 0,       1, 1,
        0, 1, 0,       0, 1
    };

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    GLuint camadaLua;
    GLuint camadaCidade1;
    GLuint camadaCidade2;
    GLuint camadaCidade3;

    load_texture("w0.png", &camadaLua);
    load_texture("w1.png", &camadaCidade1);
    load_texture("w2.png", &camadaCidade2);
    load_texture("w3.png", &camadaCidade3);

    GLuint sullyDir, sullyEsq;
    load_texture("plane.png", &sullyDir);
    load_texture("plane.png", &sullyEsq);

    glm::mat4 proj = glm::ortho(0.0f, (float)WIDTH, (float)HEIGHT, 0.0f);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        float speed = 3.0f;

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            posY -= speed;
        }

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            posY += speed;

            if (posY > HEIGHT - 500 * scale) {
                posY = HEIGHT - 500 * scale;
            }
        }

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            posX -= speed;
            direction = -1;
        }

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            posX += speed;
            direction = 1;
        }

        if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
            scale += 0.01f;
        }

        if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
            scale -= 0.01f;
            if (scale < 0.2f) scale = 0.2f;
        }

        if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS) {
            angle += 0.01f;
        }

        if (glfwGetKey(window, GLFW_KEY_X) == GLFW_PRESS) {
            angle -= 0.01f;
        }

        if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS) {
            filterMode = 0;
        }

        if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS) {
            filterMode = 1;
        }

        if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS) {
            filterMode = 2;
        }

        float playerSize = 200 * scale;

        cameraX = posX - WIDTH / 2.0f + playerSize / 2.0f;
        cameraY = posY - HEIGHT / 2.0f + playerSize / 2.0f;

        glClearColor(0.2, 0.2, 0.2, 1);
        glClear(GL_COLOR_BUFFER_BIT);

        glUseProgram(shader);
        glUniformMatrix4fv(glGetUniformLocation(shader, "proj"), 1, GL_FALSE, glm::value_ptr(proj));
        glUniform1i(glGetUniformLocation(shader, "filterMode"), filterMode);

        glBindVertexArray(VAO);

        desenharCamada(camadaLua, shader, VAO, 0.05f, 0.02f, cameraX, cameraY);
        desenharCamada(camadaCidade1, shader, VAO, 0.15f, 0.05f, cameraX, cameraY);
        desenharCamada(camadaCidade2, shader, VAO, 0.35f, 0.10f, cameraX, cameraY);
        desenharCamada(camadaCidade3, shader, VAO, 0.70f, 0.20f, cameraX, cameraY);

        float groundY = HEIGHT - 200;



        glm::mat4 sticker = glm::mat4(1.0f);

        if (direction == 1) {
            sticker = glm::translate(sticker, glm::vec3(posX - cameraX, posY - cameraY, 0));
        } else {
            sticker = glm::translate(sticker, glm::vec3(posX - cameraX + 200 * scale, posY - cameraY, 0));
        }

        sticker = glm::translate(sticker, glm::vec3(100, 100, 0));
        sticker = glm::rotate(sticker, angle, glm::vec3(0, 0, 1));
        sticker = glm::translate(sticker, glm::vec3(-100, -100, 0));

        if (direction == 1) {
            sticker = glm::scale(sticker, glm::vec3(200 * scale, 200 * scale, 1));
            glBindTexture(GL_TEXTURE_2D, sullyDir);
        } else {
            sticker = glm::scale(sticker, glm::vec3(-200 * scale, 200 * scale, 1));
            glBindTexture(GL_TEXTURE_2D, sullyEsq);
        }

        glUniform1i(glGetUniformLocation(shader, "isSticker"), 1);
        glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(sticker));
        glDrawArrays(GL_TRIANGLES, 0, 6);


        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}
