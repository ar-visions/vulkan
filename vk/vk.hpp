#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include <vk/stb_image.h>
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
        void init();
        ~impl();
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

struct PipelineData;

/// we need more key handlers
using ResizeFn = void(*)(vec2i&, void*);

struct GPU:mx {
    struct impl {
        VkPhysicalDevice        phys        = VK_NULL_HANDLE;
        VkSampleCountFlagBits   msaaSamples = VK_SAMPLE_COUNT_1_BIT;
        GLFWwindow*             window      = NULL;
        VkSurfaceKHR            surface     = 0;
        QueueFamilyIndices      indices;
        SwapChainSupportDetails details;
        void*                   user_data;
        ResizeFn                resize;
        vec2i                   sz;

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        static void framebuffer_resized(GLFWwindow *, int, int);

        ~impl();
        operator bool() { return phys != VK_NULL_HANDLE; }
        type_register(impl);
    };

    operator VkPhysicalDevice();

    VkSampleCountFlagBits getUsableSampling(VkSampleCountFlagBits max);

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
        bool                        framebufferResized;
        uint32_t                    mipLevels;

        std::vector<VkCommandBuffer> commandBuffers;

        std::vector<VkSemaphore>    imageAvailableSemaphores;
        std::vector<VkSemaphore>    renderFinishedSemaphores;
        std::vector<VkFence>        inFlightFences;
        uint32_t                    currentFrame = 0;

        void drawFrame(PipelineData &pipeline);
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
        operator bool() { return device != VK_NULL_HANDLE; }
        type_register(impl);
    };

    static Device create(GPU &gpu);
    operator VkDevice();
    mx_object(Device, mx, impl);
};

enums(Asset, undefined, "undefined, color, normal, material", undefined, color, normal, material);

struct Texture:mx {
    struct impl {
        Device          device;
        VkImage         image;
        VkDeviceMemory  memory;
        VkImageView     view;
        VkSampler       sampler;
        int             width;
        int             height;
        ion::path       path;
        Asset           asset_type;

        void create_image_view();
        void create_sampler();
        void create_image(ion::path texture_path, Asset type);
        ~impl();
        operator bool() { return image != VK_NULL_HANDLE; }
        type_register(impl);
    };
    mx_object(Texture, mx, impl);
    static Texture load(Device &dev, symbol name, Asset type);
};

struct PipelineData:mx {
    struct impl {
        Device                      device;
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
        size_t                      uniformSize;
        size_t                      vertexSize;
        cstr                        shader;
        size_t                      indicesSize;
        lambda<void()>              reload;
        watch                       watcher;

        std::vector<VkDescriptorSet>                   descriptorSets;
        VkVertexInputBindingDescription                binding_desc;
        std::vector<VkVertexInputAttributeDescription> attr_desc;

        Texture textures[Asset::count - 1];

        void start(symbol shader, symbol model);
        void cleanup(); /// impl calls cleanup, but cleanup is called prior to a reload

        ~impl();

        static std::vector<char> readFile(symbol filename);

        VkShaderModule createShaderModule(const std::vector<char>& code);

        void createDescriptorSetLayout();
        void createDescriptorSets();
        void createGraphicsPipeline();

        template <typename T>
        void createUniformBuffers() {
            VkDeviceSize bufferSize = sizeof(T); /// was UniformBufferObject

            uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
            uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
            uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                device->createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
                vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
            }
        }

        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
        operator bool() { return graphicsPipeline != VK_NULL_HANDLE; }
        type_register(impl);
    };

    virtual void updateUniformBuffer();

    mx_object(PipelineData, mx, impl);
};

/// this 'constructs' a PipelineData which is a struct that holds the pipeline info
template <typename U, typename V>
struct Pipeline:PipelineData {
    Pipeline():PipelineData() { }

    /// pipelines track themselves, and notify the user (the app that a reload is happening [likely needed for status/logging alone?])
    /// important to note that Device is not reloaded
    /// if you have 40 pipelines and 1 file changes associated to one of them, we are not rebuilding all 40, just 1.
    Pipeline(Device device, symbol shader, symbol model, lambda<void()> on_change):PipelineData() { /// instead of a texture it needs flags for the resources to load
        data->uniformSize  = sizeof(U);
        data->vertexSize   = sizeof(V);
        data->shader       = (cstr)shader;
        data->device       = device;

        ///
        data->reload = [&]() {
            data->binding_desc = getBindingDescription();
            data->attr_desc    = getAttributeDescriptions();
            data->createUniformBuffers<U>();
            data->createDescriptorSetLayout();

            /// this function need not make the .spv, the watcher service does this
            data->createGraphicsPipeline();

            for (size_t i = 1; i < Asset::count; i++) {
                data->textures[i - 1] = Texture::load(device, model, Asset(i));
            }

            path p = fmt {"models/{0}.obj", {model}}; /// obj could have different level of details
            console.test(p.exists(), "model resource not found");
            loadModel(p.cs()); /// keeps vector in stack
            data->createDescriptorSets();
        };

        /// start monitoring for changes, also recompiles shaders each time (if a shader does not compile on first go, it will error right away and thats useful)
        data->start(shader, model); /// the monitor needs the shader and the model so it can extrapolate what to listen for
    }

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(V);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions() {
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions;
        size_t        index = 0;
        type_t       v_type = typeof(V);
        doubly<prop> &props = *(doubly<prop>*)v_type->meta;
        attributeDescriptions.resize(props->len());

        auto get_format = [](prop &p) {
            if (p.member_type == typeof(vec2f)) return VK_FORMAT_R32G32_SFLOAT;
            if (p.member_type == typeof(vec3f)) return VK_FORMAT_R32G32B32_SFLOAT;
            if (p.member_type == typeof(vec4f)) return VK_FORMAT_R32G32B32A32_SFLOAT;
            if (p.member_type == typeof(float)) return VK_FORMAT_R32_SFLOAT;
            return VK_FORMAT_UNDEFINED;
        };

        for (prop &p: props) {
            attributeDescriptions[index].binding  = 0;
            attributeDescriptions[index].location = index;
            attributeDescriptions[index].format   = get_format(p);
            attributeDescriptions[index].offset   = p.offset;
            index++;
        }
   
        return attributeDescriptions;
    }

    void updateUniformBuffer() {
        U ubo {};
        ubo.update(*this);
        memcpy(data->uniformBuffersMapped[data->device->currentFrame], &ubo, sizeof(ubo));
    }

    void loadModel(cstr obj) {
        tinyobj::attrib_t                   attrib;
        std::vector<tinyobj::shape_t>       shapes;
        std::vector<tinyobj::material_t>    materials;
        std::string                         warn, err;

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, (symbol)obj)) {
            throw std::runtime_error(warn + err);
        }

        std::vector<V> vertices(attrib.vertices.size());
        std::vector<bool> emitted(attrib.vertices.size());
        std::vector<uint32_t> indices (data->indicesSize);

        size_t cur = 0;

        std::unordered_map<V, uint32_t> uniqueVertices { };

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
        data->indicesSize = indices.size();

        createVertexBuffer(vertices);
        createIndexBuffer(indices);
    }

    void createVertexBuffer(std::vector<V> &vertices) {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        data->device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* vdata;
        vkMapMemory(data->device, stagingBufferMemory, 0, bufferSize, 0, &vdata);
            memcpy(vdata, vertices.data(), (size_t) bufferSize);
        vkUnmapMemory(data->device, stagingBufferMemory);

        data->device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, data->vertexBuffer, data->vertexBufferMemory);

        data->device->copyBuffer(stagingBuffer, data->vertexBuffer, bufferSize);

        vkDestroyBuffer(data->device, stagingBuffer, nullptr);
        vkFreeMemory(data->device, stagingBufferMemory, nullptr);
    }

    void createIndexBuffer(std::vector<uint32_t> &indices) {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        data->device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
        
        void* vdata;
        vkMapMemory(data->device, stagingBufferMemory, 0, bufferSize, 0, &vdata);
            memcpy(vdata, indices.data(), (size_t) bufferSize);
        vkUnmapMemory(data->device, stagingBufferMemory);
        
        data->device->createBuffer(bufferSize,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            data->indexBuffer, data->indexBufferMemory);
        data->device->copyBuffer(stagingBuffer, data->indexBuffer, bufferSize);
        vkDestroyBuffer(data->device, stagingBuffer, nullptr);
        vkFreeMemory(data->device, stagingBufferMemory, nullptr);
    }
};

}