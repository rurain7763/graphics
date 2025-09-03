#include "pch.h"
#include "VkBuffers.h"

#ifdef SUPPORT_VULKAN

#include "VkContext.h"
#include "Graphics/GraphicsFunc.h"

namespace flaw {
    VkVertexInputLayout::VkVertexInputLayout(VkContext& context, const Descriptor& descriptor)
        : _context(context)
		, _stride(0)
		, _vertexInputRate(ConvertToVkVertexInputRate(descriptor.vertexInputRate))
		, _inputElements(descriptor.inputElements)
    {
        for (const auto& element : descriptor.inputElements) {
            _stride += GetElementSize(element.type) * element.count;
        }
    }

	vk::VertexInputBindingDescription VkVertexInputLayout::GetVkVertexInputBindingDescription(uint32_t binding) const {
		vk::VertexInputBindingDescription bindDesc;
		bindDesc.binding = binding;
        bindDesc.stride = _stride;
        bindDesc.inputRate = _vertexInputRate;

		return bindDesc;
	}

	void VkVertexInputLayout::GetVkVertexInputAttributeDescriptions(uint32_t binding, std::vector<vk::VertexInputAttributeDescription>& attrs) const {
        uint32_t location = attrs.size();
		uint32_t offset = 0;
        for (const auto& element : _inputElements) {
            vk::VertexInputAttributeDescription attributeDesc;
            attributeDesc.binding = binding;
            attributeDesc.location = location;
            attributeDesc.format = ConvertToVkFormat(element.type, element.count);
            attributeDesc.offset = offset;

            attrs.push_back(attributeDesc);

			location++;
			offset += GetElementSize(element.type) * element.count;
        }
	}
}

#endif  