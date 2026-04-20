#include "vulkan_base.h"

VkShaderModule createShaderModule(VulkanContext* context, const char* shaderFileName) {
    VkShaderModule result = {};

    // Read shader file
    FILE* file;
    fopen_s(&file, shaderFileName, "rb");
    if (!file) {
        LOG_ERROR("Shader not found: ", shaderFileName);
        return result;
    }
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, 0, SEEK_SET);
    assert((fileSize & 0x03) == 0);
    uint8_t* buffer = new uint8_t[fileSize];
    fread(buffer, fileSize, 1, file);

    VkShaderModuleCreateInfo createInfo = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    createInfo.codeSize = fileSize;
    createInfo.pCode = reinterpret_cast<uint32_t *>(buffer);
    VKA(vkCreateShaderModule(context->device, &createInfo, nullptr, &result));

    delete[] buffer;
    fclose(file);

    return result;
}

VulkanPipeline createPipeline(VulkanContext* context, const char* vertexShaderFileName, const char* fragmentShaderFileName, VkRenderPass renderPass, uint32_t width, uint32_t height) {
    VkShaderModule vertexShaderModule = createShaderModule(context, vertexShaderFileName);
    VkShaderModule fragmentShaderModule = createShaderModule(context, fragmentShaderFileName);

    VkPipelineShaderStageCreateInfo shaderStages[2];
    shaderStages[0] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].module = vertexShaderModule;
    shaderStages[0].pName = "main";
    shaderStages[1] = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].module = fragmentShaderModule;
    shaderStages[1].pName = "main";

    // VkPipelineVertexInputStateCreateInfo
    VkPipelineVertexInputStateCreateInfo vertexInputState = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };

    // VkPipelineInputAssemblyStateCreateInfo
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
    inputAssemblyState.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    // VkPipelineViewportStateCreateInfo
    VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
    viewportState.viewportCount = 1;
    VkViewport viewport = { 0.0, 0.0, static_cast<float>(width), static_cast<float>(height) };
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    VkRect2D scissor = {{0, 0}, {width, height}};
    viewportState.pScissors = &scissor;

    // VkPipelineRasterizationStateCreateInfo
    VkPipelineRasterizationStateCreateInfo rasterizationState = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
    rasterizationState.lineWidth = 1.0f;

    // VkPipelineMultisampleStateCreateInfo
    VkPipelineMultisampleStateCreateInfo multisampleState = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
    multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // VkPipelineColorBlendAttachmentState
    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    // VkPipelineColorBlendStateCreateInfo
    VkPipelineColorBlendStateCreateInfo colorBlendState = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
    colorBlendState.attachmentCount = 1;
    colorBlendState.pAttachments = &colorBlendAttachment;

    // VkPipelineLayout
    VkPipelineLayout pipelineLayout;
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
    VKA(vkCreatePipelineLayout(context->device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));

    // VkPipeline
    VkPipeline pipeline;
    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
    graphicsPipelineCreateInfo.stageCount = std::size(shaderStages);
    graphicsPipelineCreateInfo.pStages = shaderStages;
    graphicsPipelineCreateInfo.pVertexInputState = &vertexInputState;
    graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
    graphicsPipelineCreateInfo.pViewportState = &viewportState;
    graphicsPipelineCreateInfo.pRasterizationState = &rasterizationState;
    graphicsPipelineCreateInfo.pMultisampleState = &multisampleState;
    graphicsPipelineCreateInfo.pColorBlendState = &colorBlendState;
    graphicsPipelineCreateInfo.layout = pipelineLayout;
    graphicsPipelineCreateInfo.renderPass = renderPass;
    graphicsPipelineCreateInfo.subpass = 0;
    VKA(vkCreateGraphicsPipelines(context->device, nullptr, 1, &graphicsPipelineCreateInfo, nullptr, &pipeline));

    VK(vkDestroyShaderModule(context->device, vertexShaderModule, nullptr));
    VK(vkDestroyShaderModule(context->device, fragmentShaderModule, nullptr));

    VulkanPipeline result = {};
    result.pipeline = pipeline;
    result.pipelineLayout = pipelineLayout;
    return result;
}

void destroyPipeline(VulkanContext* context, VulkanPipeline* pipeline) {
    VK(vkDestroyPipeline(context->device, pipeline->pipeline, nullptr));
    VK(vkDestroyPipelineLayout(context->device, pipeline->pipelineLayout, nullptr));
}