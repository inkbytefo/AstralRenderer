#include "astral/renderer/descriptor_manager.hpp"
#include "astral/core/context.hpp"
#include <stdexcept>
#include <vector>
#include <string>

namespace astral {

DescriptorManager::DescriptorManager(Context* context) : m_context(context) {
    createLayout();
    createPoolAndSet();
}

DescriptorManager::~DescriptorManager() {
    vkDestroyDescriptorPool(m_context->getDevice(), m_pool, nullptr);
    vkDestroyDescriptorSetLayout(m_context->getDevice(), m_layout, nullptr);
}

void DescriptorManager::createLayout() {
    std::vector<VkDescriptorSetLayoutBinding> bindings(6); // 0: Images, 1: Global Buffers, 2: Material Buffers, 3: Light Buffers, 4: Array Images, 5: Storage Images
    
    // Binding 0: Images
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[0].descriptorCount = MAX_BINDLESS_IMAGES;
    bindings[0].stageFlags = VK_SHADER_STAGE_ALL;

    // Binding 1: Global Buffers (SceneData etc)
    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].descriptorCount = MAX_BINDLESS_BUFFERS;
    bindings[1].stageFlags = VK_SHADER_STAGE_ALL;

    // Binding 2: Material Buffers
    bindings[2].binding = 2;
    bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[2].descriptorCount = MAX_BINDLESS_BUFFERS;
    bindings[2].stageFlags = VK_SHADER_STAGE_ALL;

    // Binding 3: Light Buffers
    bindings[3].binding = 3;
    bindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[3].descriptorCount = MAX_BINDLESS_BUFFERS;
    bindings[3].stageFlags = VK_SHADER_STAGE_ALL;

    // Binding 4: Array Images (sampler2DArray)
    bindings[4].binding = 4;
    bindings[4].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    bindings[4].descriptorCount = MAX_BINDLESS_IMAGES;
    bindings[4].stageFlags = VK_SHADER_STAGE_ALL;

    // Binding 5: Storage Images
    bindings[5].binding = 5;
    bindings[5].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    bindings[5].descriptorCount = MAX_BINDLESS_IMAGES;
    bindings[5].stageFlags = VK_SHADER_STAGE_ALL;

    std::vector<VkDescriptorBindingFlags> flags = {
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT,
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT | VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT | VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT
    };

    VkDescriptorSetLayoutBindingFlagsCreateInfo flagsInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO};
    flagsInfo.bindingCount = static_cast<uint32_t>(flags.size());
    flagsInfo.pBindingFlags = flags.data();

    VkDescriptorSetLayoutCreateInfo layoutInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layoutInfo.pNext = &flagsInfo;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();
    layoutInfo.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;

    if (vkCreateDescriptorSetLayout(m_context->getDevice(), &layoutInfo, nullptr, &m_layout) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create bindless descriptor set layout!");
    }
}

void DescriptorManager::createPoolAndSet() {
    std::vector<VkDescriptorPoolSize> poolSizes = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, MAX_BINDLESS_IMAGES * 2}, // Binding 0 and 4
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MAX_BINDLESS_BUFFERS * 3}, // Three buffer bindings (Scene, Material, Light)
        {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, MAX_BINDLESS_IMAGES} // Binding 5
    };

    VkDescriptorPoolCreateInfo poolInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
    poolInfo.maxSets = 1;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();

    if (vkCreateDescriptorPool(m_context->getDevice(), &poolInfo, nullptr, &m_pool) != VK_SUCCESS) {
        throw std::runtime_error("Failed to create bindless descriptor pool!");
    }

    uint32_t counts[] = { MAX_BINDLESS_IMAGES, MAX_BINDLESS_BUFFERS, MAX_BINDLESS_BUFFERS, MAX_BINDLESS_BUFFERS, MAX_BINDLESS_IMAGES, MAX_BINDLESS_IMAGES };
    VkDescriptorSetVariableDescriptorCountAllocateInfo variableInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO};
    variableInfo.descriptorSetCount = 1;
    variableInfo.pDescriptorCounts = &counts[5]; // Only the last binding (5: Storage Images) has variable count

    VkDescriptorSetAllocateInfo allocInfo = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    allocInfo.pNext = &variableInfo;
    allocInfo.descriptorPool = m_pool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_layout;

    if (vkAllocateDescriptorSets(m_context->getDevice(), &allocInfo, &m_set) != VK_SUCCESS) {
        throw std::runtime_error("Failed to allocate bindless descriptor set!");
    }
}

uint32_t DescriptorManager::registerImage(VkImageView view, VkSampler sampler) {
    if (m_nextImageIndex >= MAX_BINDLESS_IMAGES) {
        throw std::runtime_error("Maximum bindless images reached!");
    }

    uint32_t index = m_nextImageIndex++;
    
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = view;
    imageInfo.sampler = sampler;

    VkWriteDescriptorSet write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet = m_set;
    write.dstBinding = 0;
    write.dstArrayElement = index;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(m_context->getDevice(), 1, &write, 0, nullptr);

    return index;
}

uint32_t DescriptorManager::registerImageArray(VkImageView view, VkSampler sampler) {
    if (m_nextArrayImageIndex >= MAX_BINDLESS_IMAGES) {
        throw std::runtime_error("Maximum bindless array images reached!");
    }

    uint32_t index = m_nextArrayImageIndex++;
    
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    imageInfo.imageView = view;
    imageInfo.sampler = sampler;

    VkWriteDescriptorSet write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet = m_set;
    write.dstBinding = 4; // Array Images
    write.dstArrayElement = index;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.descriptorCount = 1;
    write.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(m_context->getDevice(), 1, &write, 0, nullptr);

    return index;
}

uint32_t DescriptorManager::registerStorageImage(VkImageView view) {
    if (m_nextStorageImageIndex >= MAX_BINDLESS_IMAGES) {
        throw std::runtime_error("Maximum bindless storage images reached!");
    }

    uint32_t index = m_nextStorageImageIndex++;
    
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageView = view;
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;

    VkWriteDescriptorSet write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet = m_set;
    write.dstBinding = 5; // Storage Images
    write.dstArrayElement = index;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    write.descriptorCount = 1;
    write.pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(m_context->getDevice(), 1, &write, 0, nullptr);

    return index;
}

uint32_t DescriptorManager::registerBuffer(VkBuffer buffer, VkDeviceSize offset, VkDeviceSize range, uint32_t binding) {
    if (binding == 0 || binding > 3) {
        throw std::runtime_error("Invalid binding index for registerBuffer! (Expected 1, 2, or 3)");
    }
    
    if (m_nextBufferIndices[binding] >= MAX_BINDLESS_BUFFERS) {
        throw std::runtime_error("Maximum bindless buffers reached for binding " + std::to_string(binding));
    }

    uint32_t index = m_nextBufferIndices[binding]++;
    
    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = buffer;
    bufferInfo.offset = offset;
    bufferInfo.range = range;

    VkWriteDescriptorSet write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    write.dstSet = m_set;
    write.dstBinding = binding;
    write.dstArrayElement = index;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.descriptorCount = 1;
    write.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(m_context->getDevice(), 1, &write, 0, nullptr);

    return index;
}

} // namespace astral
