#include <vk/vk.hpp>

using namespace ion;

const uint32_t    WIDTH         = 800;
const uint32_t    HEIGHT        = 600;
const std::string MODEL_PATH    = "models/viking_room.obj";
const std::string TEXTURE_PATH  = "textures/viking_room.png";

struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    Vertex() { }

    Vertex(float *vpos, int p_index, float *uv, int uv_index) {
        pos      = { vpos[3 * p_index  + 0], vpos[3 * p_index + 1], vpos[3 * p_index + 2] };
        texCoord = {   uv[2 * uv_index + 0], 1.0f - uv[2 * uv_index + 1] };
        color    = { 1.0f, 1.0f, 1.0f };
    }

    doubly<prop> meta() const {
        return {
            prop { "pos",      pos      },
            prop { "color",    color    },
            prop { "texCoord", texCoord }
        };
    }

    operator bool() { return true; } /// needs to be optional, but the design-time check doesnt seem to work

    register(Vertex);

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

    void update(PipelineData &pipeline) {
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

struct HelloTriangleApplication:mx {
    struct impl {
        Vulkan    vk { 1, 0 };
        vec2i     sz;
        GPU       gpu;
        Device    device;
        Pipeline<UniformBufferObject, Vertex> pipeline;

        static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
            auto *i = (HelloTriangleApplication::impl*)(glfwGetWindowUserPointer(window));
            i->device->framebufferResized = true;
        }

        void init(vec2i &sz) {
            gpu = GPU::select(sz);
            glfwSetWindowUserPointer(gpu->window, this);
            glfwSetFramebufferSizeCallback(gpu->window, framebufferResizeCallback);
            device = Device::create(gpu);
            pipeline = Pipeline<UniformBufferObject, Vertex>(device, "generic", MODEL_PATH.c_str(), TEXTURE_PATH.c_str());
        }

        void run() {
            while (!glfwWindowShouldClose(gpu->window)) {
                glfwPollEvents();
                device->drawFrame(pipeline);
            }
            vkDeviceWaitIdle(device);
        }
        operator bool() { return sz.x > 0; }
        register(impl);
    };
    
    ptr(HelloTriangleApplication, mx, impl);

    HelloTriangleApplication(vec2i sz):HelloTriangleApplication() {
        data->init(sz);
    }

    /// return the class in main() to loop and return code
    operator int() {
        try {
            data->run();
        } catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
};

int main() {
    return HelloTriangleApplication({ WIDTH, HEIGHT });
}