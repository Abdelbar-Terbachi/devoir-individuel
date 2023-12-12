#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

// Define a structure for a 3D vertex
struct Vertex {
    float x, y, z;
};

// Define a structure for a face (a collection of vertex indices)
struct Face {
    std::vector<int> indices;
};

// Define a structure for a 3D mesh, including its name, vertices, and faces
struct Mesh {
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<Face> faces;
};

// Function to load an OBJ file and return a vector of Mesh objects
std::vector<Mesh> loadObj(const std::string& filePath) {
    std::ifstream file(filePath);
    std::vector<Mesh> meshes;
    Mesh currentMesh;
    currentMesh.name = "Default";

    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string token;
        iss >> token;

        if (token == "v") {
            // Read vertex coordinates and add them to the current mesh
            Vertex vertex;
            iss >> vertex.x >> vertex.y >> vertex.z;
            currentMesh.vertices.push_back(vertex);
        } else if (token == "f") {
            // Read face indices and add them to the current mesh
            Face face;
            int index;
            while (iss >> index) {
                face.indices.push_back(index - 1); // OBJ indices start from 1
            }
            currentMesh.faces.push_back(face);
        } else if (token == "g") {
            // If a new group (object) starts, add the current mesh to the vector
            if (!currentMesh.vertices.empty() && !currentMesh.faces.empty()) {
                meshes.push_back(currentMesh);
            }
            // Create a new mesh for the new group and set its name
            currentMesh = Mesh();
            iss >> currentMesh.name;
        }
    }

    // Add the last mesh if it is not empty
    if (!currentMesh.vertices.empty() && !currentMesh.faces.empty()) {
        meshes.push_back(currentMesh);
    }

    return meshes;
}

// Function to display a mesh using immediate mode rendering
void displayMesh(const Mesh& mesh) {
    glBegin(GL_TRIANGLES);
    for (const auto& face : mesh.faces) {
        for (int index : face.indices) {
            const Vertex& vertex = mesh.vertices[index];
            glVertex3f(vertex.x, vertex.y, vertex.z);
        }
    }
    glEnd();
}

// Function to save a mesh to an OBJ file
void saveObj(const Mesh& mesh, const std::string& filePath) {
    std::ofstream outFile(filePath);
    outFile << "g " << mesh.name << std::endl;

    // Write vertex coordinates to the file
    for (const auto& vertex : mesh.vertices) {
        outFile << "v " << vertex.x << " " << vertex.y << " " << vertex.z << std::endl;
    }

    // Write face indices to the file
    for (const auto& face : mesh.faces) {
        outFile << "f ";
        for (int index : face.indices) {
            outFile << index + 1 << " "; // Adding 1 because OBJ indices start from 1
        }
        outFile << std::endl;
    }
}

// Main function
int main() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // Set GLFW window hints
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    // Create a GLFW window
    GLFWwindow* window = glfwCreateWindow(800, 600, "OBJ Viewer", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    // Make the window's context the current one
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to initialize GLEW" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return -1;
    }

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);

    // Load OBJ file
    std::vector<Mesh> meshes = loadObj("Objets3D.obj");

    // Set up shaders
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        void main() {
            gl_Position = vec4(aPos, 1.0);
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        out vec4 FragColor;
        void main() {
            FragColor = vec4(1.0, 1.0, 1.0, 1.0);
        }
    )";

    // Create and compile vertex shader
    GLuint vertexShader, fragmentShader, shaderProgram;
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    // Create and compile fragment shader
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    // Create shader program, attach shaders, and link program
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // Delete shaders as they are linked into the program
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Set up Vertex Array Object (VAO) and Vertex Buffer Object (VBO)
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // Buffer vertex data for the first mesh into the VBO
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * meshes[0].vertices.size(), meshes[0].vertices.data(), GL_STATIC_DRAW);
    // Define attribute pointers for the vertex data
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    // Rendering loop
    while (!glfwWindowShouldClose(window)) {
        // Clear the color and depth buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Use the shader program
        glUseProgram(shaderProgram);
        // Bind the VAO
        glBindVertexArray(VAO);
        // Draw the first mesh
        glDrawArrays(GL_TRIANGLES, 0, meshes[0].vertices.size());

        // Swap the front and back buffers
        glfwSwapBuffers(window);
        // Poll for and process events
        glfwPollEvents();
    }

    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);

    // Destroy the window
    glfwDestroyWindow(window);
    // Terminate GLFW
    glfwTerminate();

    return 0;
}
