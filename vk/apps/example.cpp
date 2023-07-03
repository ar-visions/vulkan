
#include <vk/vk.hpp>

/*
included from priors:
#include <iostream>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <array>
#include <optional>
#include <set>
#include <unordered_map>
*/

constexpr bool is_debug() {
#ifndef NDEBUG
    return true;
#else
    return false;
#endif
}


using vec2i  = glm::tvec2<int>;
using cstr   = char*;
using symbol = const char *;

const uint32_t    WIDTH         = 800;
const uint32_t    HEIGHT        = 600;
const std::string MODEL_PATH    = "models/viking_room.obj";
const std::string TEXTURE_PATH  = "textures/viking_room.png";

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    Vertex() { }

    /// load from obj (constructed for each vertex)
    Vertex(float *vertices, int vertex_index, float *uv, int uv_index) {
        pos = {
            vertices[3 * vertex_index + 0],
            vertices[3 * vertex_index + 1],
            vertices[3 * vertex_index + 2]
        };

        texCoord = {
                   uv[2 * uv_index + 0],
            1.0f - uv[2 * uv_index + 1]
        };

        color = {1.0f, 1.0f, 1.0f};
    }

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
        attributeDescriptions.resize(3);

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    }

    bool operator==(const Vertex& other) const {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

struct UniformBufferObject {
    alignas(16) glm::mat4 model;
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;

    void update(PipelineData *pipeline) {
        VkExtent2D &ext = pipeline->device->swapChainExtent;

        static auto startTime   = std::chrono::high_resolution_clock::now();
        auto        currentTime = std::chrono::high_resolution_clock::now();
        float       time        = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        
        model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        view  = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        proj  = glm::perspective(glm::radians(45.0f), ext.width / (float) ext.height, 0.1f, 10.0f);
        proj[1][1] *= -1;
    }
};

class HelloTriangleApplication {
public:
    void run() {
        //initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    GPU      *gpu;
    Device   *device;
    PipelineData *pipeline;
    
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
        app->device->framebufferResized = true;
    }

    void initVulkan() {
        //createInstance(); weird: you need to pick a gpu (With Vulkan) before you initialize vulkan.
        gpu = GPU::select({ WIDTH, HEIGHT });
        glfwSetWindowUserPointer(gpu->window, this);
        glfwSetFramebufferSizeCallback(gpu->window, framebufferResizeCallback);
        
        vulkan_init();

        device = Device::create(gpu);

        pipeline = new Pipeline<UniformBufferObject, Vertex>(device, "generic", MODEL_PATH.c_str(), TEXTURE_PATH.c_str());
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(gpu->window)) {
            glfwPollEvents();
            device->drawFrame(pipeline);
        }

        vkDeviceWaitIdle(*device);
    }

    void cleanup() {
        pipeline->cleanup();
        device->cleanup();
        gpu->destroy();

        vulkan_cleanup();
    }
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}