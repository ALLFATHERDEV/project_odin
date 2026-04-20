#include "vulkan_base.h"

bool initVulkanInstance(VulkanContext* context, uint32_t instanceExtensionCount, char const * const * instanceExtensions) {

    const char* enabledLayers[] = {
        "VK_LAYER_KHRONOS_validation",
    };

    VkApplicationInfo applicationInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
    applicationInfo.pApplicationName = "Project Odin";
    applicationInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    applicationInfo.apiVersion = VK_API_VERSION_1_4;

    VkInstanceCreateInfo createInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
    createInfo.pApplicationInfo = &applicationInfo;
    createInfo.enabledLayerCount = std::size(enabledLayers);
    createInfo.ppEnabledLayerNames = enabledLayers;
    createInfo.enabledExtensionCount = instanceExtensionCount;
    createInfo.ppEnabledExtensionNames = instanceExtensions;

    if (VK(vkCreateInstance(&createInfo, nullptr, &context->instance)) != VK_SUCCESS) {
        LOG_ERROR("Error creating vulkan instance");
        return false;
    }

    return true;
}

bool selectPhysicalDevice(VulkanContext* context) {
    uint32_t numDevices = 0;
    VKA(vkEnumeratePhysicalDevices(context->instance, &numDevices, nullptr));
    if (numDevices == 0) {
        LOG_ERROR("Failed to find GPUs with Vulkan Support!");
        context->physicalDevice = nullptr;
        return false;
    }
    VkPhysicalDevice* physicalDevices = new VkPhysicalDevice[numDevices];
    VKA(vkEnumeratePhysicalDevices(context->instance, &numDevices, physicalDevices));
    LOG_INFO("Found ", numDevices, " GPU(s):");

    for (uint32_t i = 0; i < numDevices; i++) {
        VkPhysicalDeviceProperties properties = {};
        vkGetPhysicalDeviceProperties(physicalDevices[i], &properties);
        LOG_INFO(properties.deviceName);
    }
    context->physicalDevice = physicalDevices[0];
    VK(vkGetPhysicalDeviceProperties(context->physicalDevice, &context->physicalDeviceProperties));
    LOG_INFO("Selected GPU: ", context->physicalDeviceProperties.deviceName);
    delete[] physicalDevices;
    return true;
}

bool createLogicalDevice(VulkanContext* context, uint32_t deviceExtensionCount, char const * const * deviceExtensions) {
    // Queues
    uint32_t numQueueFamilies = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(context->physicalDevice, &numQueueFamilies, nullptr);
    VkQueueFamilyProperties* queueFamilies = new VkQueueFamilyProperties[numQueueFamilies];
    vkGetPhysicalDeviceQueueFamilyProperties(context->physicalDevice, &numQueueFamilies, queueFamilies);

    uint32_t graphicsQueueIndex = 0;
    for (uint32_t i = 0; i < numQueueFamilies; i++) {
        VkQueueFamilyProperties queueFamily = queueFamilies[i];
        if (queueFamily.queueCount > 0) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                graphicsQueueIndex = i;
                break;
            }
        }
    }

    constexpr float priorities[] = { 1.0f };
    VkDeviceQueueCreateInfo queueCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO};
    queueCreateInfo.queueFamilyIndex = graphicsQueueIndex;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = priorities;

    VkPhysicalDeviceFeatures enabledFeatures = {};

    VkDeviceCreateInfo createInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
    createInfo.queueCreateInfoCount = 1;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.enabledExtensionCount = deviceExtensionCount;
    createInfo.ppEnabledExtensionNames = deviceExtensions;
    createInfo.pEnabledFeatures = &enabledFeatures;

    if (vkCreateDevice(context->physicalDevice, &createInfo, nullptr, &context->device) != VK_SUCCESS) {
        LOG_ERROR("Failed to create vulkan logical device");
        return false;
    }

    // Acquire queues
    context->graphicsQueue.familyIndex = graphicsQueueIndex;
    VK(vkGetDeviceQueue(context->device, graphicsQueueIndex, 0, &context->graphicsQueue.queue));

    return true;
}

VulkanContext* initVulkan(uint32_t instanceExtensionCount, char const * const * instanceExtensions, uint32_t deviceExtensionCount, char const * const * deviceExtensions) {
    VulkanContext* context = new VulkanContext();

    if (!initVulkanInstance(context, instanceExtensionCount, instanceExtensions)) {
        return nullptr;
    }

    if (!selectPhysicalDevice(context)) {
        return nullptr;
    }

    if (!createLogicalDevice(context, deviceExtensionCount, deviceExtensions)) {
        return nullptr;
    }

    return context;
}

void exitVulkan(VulkanContext* context) {
    VKA(vkDeviceWaitIdle(context->device));
    VK(vkDestroyDevice(context->device, nullptr));

    vkDestroyInstance(context->instance, nullptr);
}