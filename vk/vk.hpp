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

using namespace ion;

static const int MAX_FRAMES_IN_FLIGHT = 2;


struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

std::vector<const char*> getRequiredExtensions();

void vulkan_init();
bool vulkan_check_validation();
void vulkan_cleanup();

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR        capabilities = {};
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR>   presentModes;
};

struct PipelineData;

struct GPU:mx {
    struct impl {
        VkPhysicalDevice        phys        = VK_NULL_HANDLE;
        VkSampleCountFlagBits   msaaSamples = VK_SAMPLE_COUNT_1_BIT;
        GLFWwindow*             window      = NULL;
        VkSurfaceKHR            surface     = 0;
        QueueFamilyIndices      indices;
        SwapChainSupportDetails details;

        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);

        void destroy();
        operator bool() { return phys != VK_NULL_HANDLE; }
        register1(impl);
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

    static GPU select(vec2i sz);

    ptr(GPU, mx, impl);
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

        void drawFrame(PipelineData *pipeline);
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
        void cleanup();
        operator bool() { return device != VK_NULL_HANDLE; }
        register1(impl);
    };

    static Device create(GPU &gpu);
    operator VkDevice();
    ptr(Device, mx, impl);
};


struct PipelineData {
    Device                      device;
    std::vector<VkBuffer>       uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*>          uniformBuffersMapped;
    VkDescriptorSetLayout       descriptorSetLayout;
    VkPipelineLayout            pipelineLayout;
    VkPipeline                  graphicsPipeline;
    VkVertexInputBindingDescription binding_desc;
    std::vector<VkVertexInputAttributeDescription> attr_desc;

    std::vector<VkDescriptorSet> descriptorSets;
    VkBuffer                    vertexBuffer;
    VkDeviceMemory              vertexBufferMemory;
    VkBuffer                    indexBuffer;
    VkDeviceMemory              indexBufferMemory;
    size_t                      uniformSize;
    size_t                      vertexSize;
    cstr                        shader;
    size_t                      indicesSize;

    VkImage                     textureImage;
    VkDeviceMemory              textureImageMemory;
    VkImageView                 textureImageView;
    VkSampler                   textureSampler;

    void cleanup();

    static std::vector<char> readFile(symbol filename);

    VkShaderModule createShaderModule(const std::vector<char>& code);

    void createGraphicsPipeline();

    void createDescriptorSets();

    virtual void updateUniformBuffer();

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

    virtual void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
};

/// this 'constructs' a PipelineData which is a struct that holds the pipeline info
template <typename U, typename V>
struct Pipeline:PipelineData {
    Pipeline(Device device, symbol shader, symbol obj, symbol texture):PipelineData() {
        this->uniformSize = sizeof(U);
        this->vertexSize  = sizeof(V);
        this->shader      = (cstr)shader;
        this->device      = device;
        
        binding_desc = getBindingDescription();
        attr_desc    = getAttributeDescriptions();

        createUniformBuffers<U>();

        createDescriptorSetLayout();
        createGraphicsPipeline();
        createTextureImage((cstr)texture);
        createTextureImageView();
        createTextureSampler();

        loadModel((cstr)obj); /// keeps vector in stack

        createDescriptorSets();
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
        };

        for (prop &p: props) {
            attributeDescriptions[index].binding  = 0;
            attributeDescriptions[index].location = index;
            attributeDescriptions[index].format   = get_format(p);
            attributeDescriptions[index].offset   = p.offset;
            index++;
        }
        /*
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(V, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(V, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(V, texCoord);
        */
        return attributeDescriptions;
    }

    void updateUniformBuffer() {
        U ubo {};
        ubo.update(this);
        memcpy(uniformBuffersMapped[device->currentFrame], &ubo, sizeof(ubo));
    }

    void createTextureImageView() {
        textureImageView = device->createImageView(
            textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, device->mipLevels);
    }

    void createTextureSampler() {
        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(device->gpu, &properties);

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = static_cast<float>(device->mipLevels);
        samplerInfo.mipLodBias = 0.0f;

        if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }
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
        std::vector<uint32_t> indices (indicesSize);

        size_t cur = 0;

        std::unordered_map<V, uint32_t> uniqueVertices { };

        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                V vertex { attrib.vertices.data(), index.vertex_index, attrib.texcoords.data(), index.texcoord_index };

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

    void createVertexBuffer(std::vector<V> &vertices) {
        VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
            memcpy(data, vertices.data(), (size_t) bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

        device->copyBuffer(stagingBuffer, vertexBuffer, bufferSize);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void createIndexBuffer(std::vector<uint32_t> &indices) {
        VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;

        device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
        
        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
            memcpy(data, indices.data(), (size_t) bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);
        
        device->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);
        device->copyBuffer(stagingBuffer, indexBuffer, bufferSize);
        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }

    void createDescriptorSetLayout() {
        VkDescriptorSetLayoutBinding uboLayoutBinding{};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        VkDescriptorSetLayoutBinding samplerLayoutBinding{};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }
    
    void createTextureImage(cstr texture_path) {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load((symbol)texture_path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;
        device->mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        device->createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
            memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(device, stagingBufferMemory);

        stbi_image_free(pixels);

        device->createImage(texWidth, texHeight, device->mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);

        device->transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, device->mipLevels);
        device->copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
        //transitioned to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL while generating mipmaps

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);

        device->generateMipmaps(textureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, device->mipLevels);
    }
};
