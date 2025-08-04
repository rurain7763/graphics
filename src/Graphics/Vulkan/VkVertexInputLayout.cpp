#include "pch.h"
#include "VkBuffers.h"

#ifdef SUPPORT_VULKAN

#include "VkContext.h"
#include "Graphics/GraphicsFunc.h"

namespace flaw {
    VkVertexInputLayout::VkVertexInputLayout(VkContext& context, const Descriptor& descriptor)
        : _context(context)
    {
        uint32_t stride = 0;
        for (const auto& element : descriptor.inputElements) {
            vk::VertexInputAttributeDescription attributeDesc;
            attributeDesc.binding = descriptor.binding;
            attributeDesc.location = _attributeDescriptions.size();
            attributeDesc.format = ConvertToVkFormat(element.type, element.count);
            attributeDesc.offset = stride;

            _attributeDescriptions.push_back(attributeDesc);

            stride += GetElementSize(element.type) * element.count;
        }

        _bindingDescription.binding = descriptor.binding;
        _bindingDescription.stride = stride;
        _bindingDescription.inputRate = ConvertToVkVertexInputRate(descriptor.vertexInputRate);
    }
}

#endif  