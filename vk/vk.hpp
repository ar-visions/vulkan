#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include <vk/tiny_obj_loader.h>

#include <mx/mx.hpp>
#include <math/math.hpp>
#include <watch/watch.hpp>

//#include <iostream>
//#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <vector>
//#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <limits>
#include <array>
#include <optional>
#include <set>
//#include <unordered_map>

namespace ion {

static const int MAX_FRAMES_IN_FLIGHT = 2;
static const int MAX_PBR_LIGHTS       = 3;

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct Vulkan:mx {
    struct impl {
        int v_major = 1;
        int v_minor = 0;
        VkApplicationInfo app_info;
        void init();
        ~impl();
        VkInstance inst();
        bool check_validation();
        std::vector<symbol> getRequiredExtensions();
        operator bool();
        type_register(impl);
    };
    mx_declare(Vulkan, mx, impl);

    /// need not construct unless we want a specific version other than 1.0
    Vulkan(int v_major, int v_minor):Vulkan() {
        data->v_major = v_major;
        data->v_minor = v_minor;
    }
};

template <> struct is_singleton<Vulkan::impl> : true_type { };

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR        capabilities = {};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   presentModes;
};

struct Pipeline;

/// we need more key handlers
using ResizeFn = void(*)(vec2i&, void*);

struct Texture;
struct Device;

struct GPU:mx {
    struct impl {
        VkInstance              instance;
        VkPhysicalDevice        phys        = VK_NULL_HANDLE;
        VkSampleCountFlagBits   msaaSamples = VK_SAMPLE_COUNT_1_BIT; /// max allowed by this GPU, not the actual sample counts used all the time; it can also force sampling down
        GLFWwindow*             window      = NULL;
        VkSurfaceKHR            surface     = 0;
        QueueFamilyIndices      indices;
        SwapChainSupportDetails details;
        ResizeFn                resize; /// deprecate
        vec2i                   sz;
        VkPhysicalDeviceFeatures support;
        vec2f                   dpi_scale;

        VkSampleCountFlagBits getUsableSampling(VkSampleCountFlagBits max);
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        static void framebuffer_resized(GLFWwindow *, int, int);

        ~impl();
        operator bool() { return phys != VK_NULL_HANDLE; }
        type_register(impl);

        Texture &texture(Device &dev, vec2i sz, bool sampling,
            VkImageUsageFlagBits usage = (VkImageUsageFlagBits)(VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT));
    };

    operator VkPhysicalDevice();

    static GLFWwindow *initWindow(vec2i &sz);
    
    static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice phys, VkSurfaceKHR surface, SwapChainSupportDetails &details);

    static SwapChainSupportDetails querySwapChainSupport(GPU &gpu);

    static bool checkDeviceExtensionSupport(VkPhysicalDevice phys);

    static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice phy, VkSurfaceKHR surface);

    static QueueFamilyIndices findQueueFamilies(GPU &gpu);

    static bool isDeviceSuitable(VkPhysicalDevice phys, VkSurfaceKHR surface, QueueFamilyIndices &indices, SwapChainSupportDetails &swapChainSupport);

    static GPU select(vec2i sz, ResizeFn resize, void *user_data);
    
    mx_object(GPU, mx, impl);
};

struct Device:mx {
    struct impl {
        GPU                         gpu;
        VkDevice                    device;
        VkQueue                     graphicsQueue;
        VkQueue                     presentQueue;
        VkSwapchainKHR              swapChain;
        std::vector<VkImage>        swapChainImages;
        VkFormat                    swapChainImageFormat;
        VkFormat                    textureFormat;
        VkExtent2D                  swapChainExtent;
        std::vector<VkImageView>    swapChainImageViews;
        std::vector<VkFramebuffer>  swapChainFramebuffers;
        VkRenderPass                renderPass;
        VkDescriptorPool            descriptorPool;
        VkCommandPool               commandPool;
        VkImage                     colorImage;
        VkDeviceMemory              colorImageMemory;
        VkImageView                 colorImageView;
        VkImage                     depthImage;
        VkDeviceMemory              depthImageMemory;
        VkImageView                 depthImageView;
        VkSemaphore                 semaphore;
        bool                        framebufferResized;
        uint32_t                    mipLevels;
        std::mutex                  mtx;

        std::vector<VkCommandBuffer> commandBuffers;

        std::vector<VkSemaphore>    imageAvailableSemaphores;
        std::vector<VkSemaphore>    renderFinishedSemaphores;
        std::vector<VkFence>        inFlightFences;
        uint32_t                    currentFrame = 0;

        lambda<void(VkImage, VkCommandBuffer)> preCommands;
        lambda<void(VkImage, VkCommandBuffer)> postCommands;

        void drawFrame(array<Pipeline> &pipelines);
        void recreateSwapChain();
        void createDescriptorPool();
        void createFramebuffers();
        void createSyncObjects();
        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
        VkCommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(VkCommandBuffer commandBuffer);
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        void createCommandBuffers();
        void createCommandPool();
        void createColorResources();
        void createDepthResources();
        VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
        VkFormat findDepthFormat();
        bool hasStencilComponent(VkFormat format);
        void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
        VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
        void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
        void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
        void createLogicalDevice();
        void createSwapChain();
        void createImageViews();
        void createRenderPass();
        void cleanupSwapChain();
        ~impl();
        void loop(lambda<void(array<Pipeline>&)>);
        operator bool() { return device != VK_NULL_HANDLE; }
        type_register(impl);
    };

    static Device create(GPU &gpu);

    operator VkDevice();
    mx_object(Device, mx, impl);
};

enums(Asset, undefined, 
    "undefined, color, normal, material, reflect",
     undefined, color, normal, material, reflect);

enums(VA, Position,
    "Position, Normal, UV, Color, Tangent, BiTangent",
     Position, Normal, UV, Color, Tangent, BiTangent);

enums(Rendition, none,
    "none, shader, wireframe",
     none, shader, wireframe);

enums(ShadeModule, undefined,
    "undefined, vertex, fragment, compute",
     undefined, vertex, fragment, compute);

/// this is not actually needed in our current model of Vertex, as it uses the meta()
/// just comment and deprecate
using VAttribs = states<VA>;

/// never support 16bit indices for obvious reasons.  you do 1 cmclark section too many and there it goes
struct ngon {
    size_t size;
    u32   *indices;
};

using vpair = std::pair<int, int>;

struct vpair_hash {
    std::size_t operator()(const vpair& p) const {
        return std::hash<int>()(p.first) ^ std::hash<int>()(p.second);
    }
};

using mesh  = array<ngon>;

//using u32   = uint32_t;

using face  = ngon;

struct Texture:mx {
    struct impl {
        Device          device;
        VkImageUsageFlagBits usage;
        VkFormat        format;
        VkImage         image;
        VkDeviceMemory  memory;
        VkImageView     view;
        VkSampler       sampler;
        VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
        int             width;
        int             height;
        ion::path       path;
        Asset           asset_type;
        vec2i           sz;

        void create_image_view();
        void create_sampler();
        void create_image(ion::path texture_path, Asset type); // VK_FORMAT_R8G8B8A8_SRGB
        void create_image(vec2i sz);
        ~impl();
        operator bool() { return image != VK_NULL_HANDLE; }
        type_register(impl);
    };
    mx_object(Texture, mx, impl);
    static Texture load(Device &dev, symbol name, Asset type);
};

struct GraphicsData {
    Device device;
    symbol shader;
    symbol model;
    type_t u_type;
    type_t v_type;
    ///
    type_register(GraphicsData);
};

/// graphics model
template <typename U, typename V>
struct Graphics:mx { /// dont need to store generic data; this is used as an argument for specifying Uniform and Vertex as well as model, and shader; its model of graphics

    ///
    Graphics(Device device, symbol model, symbol shader = "disney") : Graphics() {
        data->device = device;
        data->shader = shader;
        data->model  = model;
        data->u_type = typeof(U);
        data->v_type = typeof(V);
    }

    mx_object(Graphics, mx, GraphicsData);
};

struct Pipeline:mx {
    struct impl {
        Device                      device;
        memory*                     gmem; // graphics memory (grabbed)
        GraphicsData*               gfx;
        std::vector<VkBuffer>       uniformBuffers;
        std::vector<VkDeviceMemory> uniformBuffersMemory;
        std::vector<void*>          uniformBuffersMapped;
        VkDescriptorSetLayout       descriptorSetLayout;
        VkPipelineLayout            pipelineLayout;
        VkPipeline                  graphicsPipeline;
        VkBuffer                    vertexBuffer;
        VkDeviceMemory              vertexBufferMemory;
        VkBuffer                    indexBuffer;
        VkDeviceMemory              indexBufferMemory;
        size_t                      indicesSize;
        lambda<void()>              reload;
        lambda<void()>              uniform_update;
        bool                        init;
        watch                       watcher;

        std::vector<VkDescriptorSet>                   descriptorSets;
        VkVertexInputBindingDescription                binding_desc;
        std::vector<VkVertexInputAttributeDescription> attr_desc;

        Texture textures[Asset::count - 1];

        void start();
        void cleanup(); /// impl calls cleanup, but cleanup is called prior to a reload
        void createUniformBuffers();

        ~impl();

        VkShaderModule createShaderModule(const array<char>& code);

        void createDescriptorSetLayout();
        void createDescriptorSets();
        void createGraphicsPipeline();

        template <typename V>
        void loadModel(cstr obj) {
            tinyobj::attrib_t                   attrib;
            std::vector<tinyobj::shape_t>       shapes;
            std::vector<tinyobj::material_t>    materials;
            std::string                         warn, err;

            if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, (symbol)obj)) {
                throw std::runtime_error(warn + err);
            }

            std::vector<V>        vertices(attrib.vertices.size());
            std::vector<bool>     emitted(attrib.vertices.size());
            std::vector<uint32_t> indices (indicesSize);
            size_t                cur = 0;
            auto                  uniqueVertices = std::unordered_map<V, uint32_t> { };

            for (const auto& shape : shapes) {
                for (const auto& index : shape.mesh.indices) {
                    V vertex {
                        attrib.vertices.data(),  index.vertex_index,
                        attrib.texcoords.data(), index.texcoord_index,
                        attrib.normals.data(),   index.normal_index };

                    if (uniqueVertices.count(vertex) == 0) {
                        uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                        vertices.push_back(vertex);
                    }
                    indices.push_back(uniqueVertices[vertex]);
                }
            }
            indicesSize = indices.size();
            createVertexBuffer(vertices);
            createIndexBuffer(indices);
        }
        
        template <typename V>
        void createVertexBuffer(std::vector<V> &vertices) {
            VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
            VkBuffer stagingBuffer;
            VkDeviceMemory stagingBufferMemory;
            device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                stagingBuffer, stagingBufferMemory);
            void* vdata;
            vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &vdata);
                memcpy(vdata, vertices.data(), (size_t) bufferSize);
            vkUnmapMemory(device, stagingBufferMemory);
            device->createBuffer(bufferSize,
                VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);
            device->copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
            vkDestroyBuffer(device, stagingBuffer, nullptr);
            vkFreeMemory(device, stagingBufferMemory, nullptr);
        }

        void createIndexBuffer(std::vector<uint32_t> &indices);

        VkVertexInputBindingDescription getBindingDescription();
        std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
        operator bool() { return graphicsPipeline != VK_NULL_HANDLE; }
        type_register(impl);
    };

    mx_object(Pipeline, mx, impl);

    /// pipelines track themselves, and notify the user (the app that a reload is happening [likely needed for status/logging alone?])
    /// important to note that Device is not reloaded
    /// if you have 40 pipelines and 1 file changes associated to one of them, we are not rebuilding all 40, just 1.
    template <typename U, typename V>
    Pipeline(Graphics<U,V> graphics):Pipeline() { /// instead of a texture it needs flags for the resources to load
        data->gmem           = graphics.grab(); /// just so we can hold onto the data; we must drop this in our dtr
        data->gfx            = graphics.data;
        data->device         = data->gfx->device;
        
        data->uniform_update = [data=data]() {
            U ubo {};
            ubo.update(data);
            memcpy(data->uniformBuffersMapped[data->device->currentFrame], &ubo, sizeof(ubo));
        };

        data->reload       = [data=data]() { /// this will need a proper cleanup proc in itsself
            data->binding_desc = data->getBindingDescription();
            data->attr_desc    = data->getAttributeDescriptions();
            
            data->createUniformBuffers();
            data->createDescriptorSetLayout();
            data->createGraphicsPipeline();

            /// load texture assets for all enumerables (minus undefined)
            for (size_t i = 1; i < Asset::count; i++)
                data->textures[i - 1] = Texture::load(
                    data->gfx->device, data->gfx->model, Asset(i));

            path p = fmt {"models/{0}.obj", { str(data->gfx->model) }}; /// obj could have different level of details
            console.test(p.exists(), "model resource not found");
            data->template loadModel<V>(p.cs()); /// keeps vector in stack
            data->createDescriptorSets();
        };

        /// start monitoring for changes, also recompiles shaders each time (if a shader does not compile on first go, it will error right away and thats useful)
        data->start(); /// the monitor needs the shader and the model so it can extrapolate what to listen for
    }
};

}