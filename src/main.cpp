#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include "logger.h"
#include "vulkan_base/vulkan_base.h"

VulkanContext* context;
VkSurfaceKHR surface;
VkRenderPass renderPass;
VulkanSwapchain swapChain;
std::vector<VkFramebuffer> frameBuffers;
VulkanPipeline pipeline;
VkCommandPool commandPool;
VkCommandBuffer commandBuffer;
VkFence fence;
VkSemaphore acquireSemaphore;
VkSemaphore releaseSemaphore;

bool handleMessage() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                return false;
        }
    }
    return true;
}

void initApplication(SDL_Window* window) {
    uint32_t instanceExtensionCount;
    char const * const * enabledInstanceExtensions = SDL_Vulkan_GetInstanceExtensions(&instanceExtensionCount);

    const char* enabledDeviceExtensions[] { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    context = initVulkan(instanceExtensionCount, enabledInstanceExtensions, std::size(enabledDeviceExtensions), enabledDeviceExtensions);
    SDL_Vulkan_CreateSurface(window, context->instance, nullptr, &surface);
    swapChain = createSwapchain(context, surface, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT);
    renderPass = createRenderPass(context, swapChain.format);
    frameBuffers.resize(swapChain.images.size());

    for (uint32_t i = 0; i < swapChain.images.size(); i++) {
        VkFramebufferCreateInfo createInfo = { VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO };
        createInfo.renderPass = renderPass;
        createInfo.attachmentCount = 1;
        createInfo.pAttachments = &swapChain.imageViews[i];
        createInfo.width = swapChain.width;
        createInfo.height = swapChain.height;
        createInfo.layers = 1;
        VKA(vkCreateFramebuffer(context->device, &createInfo, nullptr, &frameBuffers[i]));
    }

    pipeline = createPipeline(context, "../shaders/triangle_vert.spv", "../shaders/triangle_frag.spv", renderPass, swapChain.width, swapChain.height);

    VkFenceCreateInfo fenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    VKA(vkCreateFence(context->device, &fenceCreateInfo, nullptr, &fence));
    VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
    VKA(vkCreateSemaphore(context->device, &semaphoreCreateInfo, nullptr, &acquireSemaphore));
    VKA(vkCreateSemaphore(context->device, &semaphoreCreateInfo, nullptr, &releaseSemaphore));

    // Creating command pools
    VkCommandPoolCreateInfo commandPoolCreateInfo = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
    commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
    commandPoolCreateInfo.queueFamilyIndex = context->graphicsQueue.familyIndex;
    VKA(vkCreateCommandPool(context->device, &commandPoolCreateInfo, nullptr, &commandPool));

    // Creating command buffer
    VkCommandBufferAllocateInfo allocateInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
    allocateInfo.commandPool = commandPool;
    allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocateInfo.commandBufferCount = 1;
    VKA(vkAllocateCommandBuffers(context->device, &allocateInfo, &commandBuffer));

}

void renderApplication() {
    uint32_t imageIndex = 0;
    VK(vkAcquireNextImageKHR(context->device, swapChain.swapchain, UINT64_MAX, acquireSemaphore, nullptr, &imageIndex));

    VKA(vkWaitForFences(context->device, 1, &fence, VK_TRUE, UINT64_MAX));
    VKA(vkResetFences(context->device, 1, &fence));
    VKA(vkResetCommandPool(context->device, commandPool, 0));

    // BEGIN COMMAND BUFFER
    VkCommandBufferBeginInfo beginInfo = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    VKA(vkBeginCommandBuffer(commandBuffer, &beginInfo));

    VkClearValue clearValue = {1.0f, 0.0, 1.0f, 1.0f};
    VkRenderPassBeginInfo renderPassBeginInfo = { VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO };
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.framebuffer = frameBuffers[imageIndex];
    renderPassBeginInfo.renderArea = {{0, 0}, {swapChain.width, swapChain.height}};
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearValue;
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.pipeline);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    VKA(vkEndCommandBuffer(commandBuffer));
    // END COMMAND BUFFER

    VkSubmitInfo submitInfo = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &acquireSemaphore;
    VkPipelineStageFlags waitMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.pWaitDstStageMask = &waitMask;
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = &releaseSemaphore;
    VKA(vkQueueSubmit(context->graphicsQueue.queue, 1, &submitInfo, fence));

    VkPresentInfoKHR presentInfo = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &swapChain.swapchain;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = &releaseSemaphore;
    VK(vkQueuePresentKHR(context->graphicsQueue.queue, &presentInfo));
}

void shutdownApplication() {
    VKA(vkDeviceWaitIdle(context->device));

    VK(vkDestroyFence(context->device, fence, nullptr));
    VK(vkDestroyCommandPool(context->device, commandPool, nullptr));

    destroyPipeline(context, &pipeline);

    for (uint32_t i = 0; i < frameBuffers.size(); i++) {
        VK(vkDestroyFramebuffer(context->device, frameBuffers[i], nullptr));
    }
    frameBuffers.clear();
    destroyRenderPass(context, renderPass);
    destroySwapChain(context, &swapChain);
    VK(vkDestroySurfaceKHR(context->instance, surface, nullptr));
    exitVulkan(context);
}

int main() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        LOG_ERROR("SDL_Init Error: ", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Project Odin", 1240, 720, SDL_WINDOW_VULKAN);
    if (!window) {
        LOG_ERROR("SDL_CreateWindow Error: ", SDL_GetError());
        return 1;
    }

    initApplication(window);

    while (handleMessage()) {
        renderApplication();
    }

    shutdownApplication();

    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}