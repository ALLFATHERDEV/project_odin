#include "vulkan_base.h"

VulkanSwapchain createSwapchain(VulkanContext* context, VkSurfaceKHR surface, VkImageUsageFlags usageFlags) {
    VulkanSwapchain result = {};
    VkBool32 supportsPresent = VK_FALSE;
    VKA(vkGetPhysicalDeviceSurfaceSupportKHR(context->physicalDevice, context->graphicsQueue.familyIndex, surface, &supportsPresent));
    if (!supportsPresent) {
        LOG_ERROR("Graphics queue does not support present");
        return result;
    }

    uint32_t numFormats = 0;
    VKA(vkGetPhysicalDeviceSurfaceFormatsKHR(context->physicalDevice, surface, &numFormats, nullptr));
    VkSurfaceFormatKHR* availableFormats = new VkSurfaceFormatKHR[numFormats];
    VKA(vkGetPhysicalDeviceSurfaceFormatsKHR(context->physicalDevice, surface, &numFormats, availableFormats));
    if (numFormats <= 0) {
        LOG_ERROR("No supported formats found");
        delete[] availableFormats;
        return result;
    }

    VkFormat format = availableFormats[0].format;
    VkColorSpaceKHR colorSpace = availableFormats[0].colorSpace;

    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    VKA(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(context->physicalDevice, surface, &surfaceCapabilities));
    if (surfaceCapabilities.currentExtent.width == 0xFFFFFFFF) {
        surfaceCapabilities.currentExtent.width = surfaceCapabilities.minImageExtent.width;
    }
    if (surfaceCapabilities.currentExtent.height == 0xFFFFFFFF) {
        surfaceCapabilities.currentExtent.height = surfaceCapabilities.minImageExtent.height;
    }
    if (surfaceCapabilities.maxImageCount == 0) {
        surfaceCapabilities.maxImageCount = 8;
    }

    VkSwapchainCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
    createInfo.surface = surface;
    createInfo.minImageCount = surfaceCapabilities.minImageCount;
    createInfo.imageFormat = format;
    createInfo.imageColorSpace = colorSpace;
    createInfo.imageExtent = surfaceCapabilities.currentExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = usageFlags;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    VKA(vkCreateSwapchainKHR(context->device, &createInfo, nullptr, &result.swapchain));

    result.format = format;
    result.width = surfaceCapabilities.currentExtent.width;
    result.height = surfaceCapabilities.currentExtent.height;

    uint32_t numImages;
    VKA(vkGetSwapchainImagesKHR(context->device, result.swapchain, &numImages, nullptr));

    // Acquire swapchain images
    result.images.resize(numImages);
    VKA(vkGetSwapchainImagesKHR(context->device, result.swapchain, &numImages, result.images.data()));

    result.imageViews.resize(numImages);
    // Create image views
    for (uint32_t i = 0; i < numImages; i++) {
        VkImageViewCreateInfo createInfo = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
        createInfo.image = result.images[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = format;
        createInfo.components = {};
        createInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 };

        VKA(vkCreateImageView(context->device, &createInfo, nullptr, &result.imageViews[i]));
    }

    delete[] availableFormats;
    return result;
}

void destroySwapChain(VulkanContext* context, VulkanSwapchain* swapChain) {
    for (uint32_t i = 0; i < swapChain->imageViews.size(); i++) {
        VK(vkDestroyImageView(context->device, swapChain->imageViews[i], nullptr));
    }
    VK(vkDestroySwapchainKHR(context->device, swapChain->swapchain, nullptr));
}