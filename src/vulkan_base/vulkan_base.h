#pragma once

#include "../logger.h"

#include <vulkan/vulkan.h>
#include <cassert>
#include <vector>

#define ASSERT_VULKAN(val) if (val != VK_SUCCESS) {assert(false);}

#ifndef VK
#define VK(f) (f)
#endif

#ifndef VKA
#define VKA(f) ASSERT_VULKAN(VK(f))
#endif

#define ARRAY_COUNT(array) (sizeof(array) / sizeof((array)[0]))

struct VulkanQueue {
    VkQueue queue;
    uint32_t familyIndex;
};

struct VulkanSwapchain {
    VkSwapchainKHR swapchain;
    uint32_t width;
    uint32_t height;
    VkFormat format;
    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
};

struct VulkanPipeline {
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
};

struct VulkanContext {
    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceProperties physicalDeviceProperties;
    VkDevice device;
    VulkanQueue graphicsQueue;
};

VulkanContext* initVulkan(uint32_t instanceExtensionCount, char const * const * instanceExtensions, uint32_t deviceExtensionCount, char const * const * deviceExtensions);
void exitVulkan(VulkanContext* context);

VulkanSwapchain createSwapchain(VulkanContext* context, VkSurfaceKHR surface, VkImageUsageFlags usageFlags);
void destroySwapChain(VulkanContext* context, VulkanSwapchain* swapChain);

VkRenderPass createRenderPass(VulkanContext* context, VkFormat format);
void destroyRenderPass(VulkanContext* context, VkRenderPass renderPass);

VulkanPipeline createPipeline(VulkanContext* context, const char* vertexShaderFileName, const char* fragmentShaderFileName, VkRenderPass renderPass, uint32_t width, uint32_t height);
void destroyPipeline(VulkanContext* context, VulkanPipeline* pipeline);