/*------------------------------------------------------------------------
 * Vulkan Conformance Tests
 * ------------------------
 *
 * Copyright (c) 2015 Google Inc.
 * Copyright (c) 2015 Mobica Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice(s) and this permission notice shall be included
 * in all copies or substantial portions of the Materials.
 *
 * The Materials are Confidential Information as defined by the
 * Khronos Membership Agreement until designated non-confidential by Khronos,
 * at which point this condition clause shall be removed.
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 *
 *//*!
 * \file
 * \brief Compute Shader Tests
 *//*--------------------------------------------------------------------*/

#include "vktComputeBasicComputeShaderTests.hpp"
#include "vktTestCase.hpp"
#include "vktTestCaseUtil.hpp"

#include "vkDefs.hpp"
#include "vkRef.hpp"
#include "vkRefUtil.hpp"
#include "vkPlatform.hpp"
#include "vkPrograms.hpp"
#include "vkRefUtil.hpp"
#include "vkMemUtil.hpp"
#include "vkQueryUtil.hpp"
#include "vkBuilderUtil.hpp"
#include "vkTypeUtil.hpp"

#include "deStringUtil.hpp"
#include "deUniquePtr.hpp"
#include "deRandom.hpp"

#include <vector>

using namespace vk;

namespace vkt
{
namespace compute
{
namespace
{

// Helper functions

template<typename T, int size>
T multiplyComponents (const tcu::Vector<T, size>& v)
{
	T accum = 1;
	for (int i = 0; i < size; ++i)
		accum *= v[i];
	return accum;
}

template<typename T>
inline T squared (const T& a)
{
	return a * a;
}

Move<VkCmdPool> makeCommandPool (const DeviceInterface& vk, const VkDevice device, const deUint32 queueFamilyIndex)
{
	const VkCmdPoolCreateInfo cmdPoolParams =
	{
		VK_STRUCTURE_TYPE_CMD_POOL_CREATE_INFO,			// VkStructureType		sType;
		DE_NULL,										// const void*			pNext;
		queueFamilyIndex,								// deUint32				queueFamilyIndex;
		VK_CMD_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,	// VkCmdPoolCreateFlags	flags;
	};
	return createCommandPool(vk, device, &cmdPoolParams);
}

Move<VkCmdBuffer> makeCommandBuffer (const DeviceInterface& vk, const VkDevice device, const VkCmdPool cmdPool)
{
	const VkCmdBufferCreateInfo cmdBufParams =
	{
		VK_STRUCTURE_TYPE_CMD_BUFFER_CREATE_INFO,		//	VkStructureType			sType;
		DE_NULL,										//	const void*				pNext;
		cmdPool,										//	VkCmdPool				pool;
		VK_CMD_BUFFER_LEVEL_PRIMARY,					//	VkCmdBufferLevel		level;
		0u,												//	VkCmdBufferCreateFlags	flags;
	};
	return createCommandBuffer(vk, device, &cmdBufParams);
}

Move<VkShader> makeComputeShader (const DeviceInterface& vk, const VkDevice device, const VkShaderModule shaderModule)
{
	const VkShaderCreateInfo shaderParams =
	{
		VK_STRUCTURE_TYPE_SHADER_CREATE_INFO,	// VkStructureType		sType;
		DE_NULL,								// const void*			pNext;
		shaderModule,							// VkShaderModule		module;
		"main",									// const char*			pName;
		0u,										// VkShaderCreateFlags	flags;
		VK_SHADER_STAGE_COMPUTE,				// VkShaderStage		stage;
	};
	return createShader(vk, device, &shaderParams);
}

Move<VkPipelineLayout> makePipelineLayout (const DeviceInterface&		vk,
										   const VkDevice				device,
										   const VkDescriptorSetLayout*	descriptorSetLayout)
{
	const deUint32 descriptorSetCount = (descriptorSetLayout != DE_NULL ? 1u : 0);
	const VkPipelineLayoutCreateInfo pipelineLayoutParams =
	{
		VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,	// VkStructureType				sType;
		DE_NULL,										// const void*					pNext;
		descriptorSetCount,								// deUint32						descriptorSetCount;
		descriptorSetLayout,							// const VkDescriptorSetLayout*	pSetLayouts;
		0u,												// deUint32						pushConstantRangeCount;
		DE_NULL,										// const VkPushConstantRange*	pPushConstantRanges;
	};
	return createPipelineLayout(vk, device, &pipelineLayoutParams);
}

Move<VkPipeline> makeComputePipeline (const DeviceInterface&	vk,
									  const VkDevice			device,
									  const VkPipelineLayout	pipelineLayout,
									  const VkShader			shader)
{
	const VkPipelineShaderStageCreateInfo pipelineShaderStageParams =
	{
		VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,	// VkStructureType				sType;
		DE_NULL,												// const void*					pNext;
		VK_SHADER_STAGE_COMPUTE,								// VkShaderStage				stage;
		shader,													// VkShader						shader;
		DE_NULL,												// const VkSpecializationInfo*	pSpecializationInfo;
	};
	const VkComputePipelineCreateInfo pipelineCreateInfo =
	{
		VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,		// VkStructureType					sType;
		DE_NULL,											// const void*						pNext;
		pipelineShaderStageParams,							// VkPipelineShaderStageCreateInfo	stage;
		0,													// VkPipelineCreateFlags			flags;
		pipelineLayout,										// VkPipelineLayout					layout;
		DE_NULL,											// VkPipeline						basePipelineHandle;
		0,													// deInt32							basePipelineIndex;
	};
	return createComputePipeline(vk, device, DE_NULL , &pipelineCreateInfo);
}

VkDescriptorInfo makeDescriptorInfoForBuffer (const VkBuffer& buffer, const VkDeviceSize offset, const VkDeviceSize range)
{
	const VkDescriptorInfo descriptorInfo =
	{
		0,							// VkBufferView				bufferView;
		0,							// VkSampler				sampler;
		0,							// VkImageView				imageView;
		VK_IMAGE_LAYOUT_UNDEFINED,	// VkImageLayout			imageLayout;
		{ buffer, offset, range },	// VkDescriptorBufferInfo	bufferInfo;
	};
	return descriptorInfo;
}

VkDescriptorInfo makeDescriptorInfoForImageView (const VkImageView imageView, const VkImageLayout imageLayout)
{
	const VkDescriptorInfo descriptorInfo =
	{
		0,					// VkBufferView				bufferView;
		0,					// VkSampler				sampler;
		imageView,			// VkImageView				imageView;
		imageLayout,		// VkImageLayout			imageLayout;
		0,					// VkDescriptorBufferInfo	bufferInfo;
	};
	return descriptorInfo;
}

VkBufferMemoryBarrier makeBufferMemoryBarrier (const VkMemoryOutputFlags	outputFlags,
											   const VkMemoryInputFlags		inputFlags,
											   const VkBuffer				buffer,
											   const VkDeviceSize			offset,
											   const VkDeviceSize			bufferSizeBytes)
{
	const VkBufferMemoryBarrier barrier =
	{
		VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,	// VkStructureType		sType;
		DE_NULL,									// const void*			pNext;
		outputFlags,								// VkMemoryOutputFlags	outputMask;
		inputFlags,									// VkMemoryInputFlags	inputMask;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32				srcQueueFamilyIndex;
		VK_QUEUE_FAMILY_IGNORED,					// deUint32				destQueueFamilyIndex;
		buffer,										// VkBuffer				buffer;
		offset,										// VkDeviceSize			offset;
		bufferSizeBytes,							// VkDeviceSize			size;
	};
	return barrier;
}

VkBufferImageCopy makeBufferImageCopy (const tcu::UVec2& imageSize)
{
	const VkBufferImageCopy copyParams =
	{
		0ull,															//	VkDeviceSize			bufferOffset;
		imageSize.x(),													//	deUint32				bufferRowLength;
		imageSize.y(),													//	deUint32				bufferImageHeight;
		makeImageSubresourceCopy(VK_IMAGE_ASPECT_COLOR, 0u, 0u,	1u),	//	VkImageSubresourceCopy	imageSubresource;
		makeOffset3D(0u, 0u, 0u),										//	VkOffset3D				imageOffset;
		makeExtent3D(imageSize.x(), imageSize.y(), 1u),					//	VkExtent3D				imageExtent;
	};
	return copyParams;
}

void beginCommandBuffer (const DeviceInterface& vk, const VkCmdBuffer cmdBuffer)
{
	const VkCmdBufferBeginInfo cmdBufBeginParams =
	{
		VK_STRUCTURE_TYPE_CMD_BUFFER_BEGIN_INFO,		//	VkStructureType				sType;
		DE_NULL,										//	const void*					pNext;
		0u,												//	VkCmdBufferOptimizeFlags	flags;
		DE_NULL,										//	VkRenderPass				renderPass;
		0u,												//	deUint32					subpass;
		DE_NULL,										//	VkFramebuffer				framebuffer;
	};
	VK_CHECK(vk.beginCommandBuffer(cmdBuffer, &cmdBufBeginParams));
}

void endCommandBuffer (const DeviceInterface& vk, const VkCmdBuffer cmdBuffer)
{
	VK_CHECK(vk.endCommandBuffer(cmdBuffer));
}

void submitCommandsAndWait (const DeviceInterface&	vk,
							const VkDevice			device,
							const VkQueue			queue,
							const VkCmdBuffer		cmdBuffer)
{
	const VkFenceCreateInfo	fenceParams =
	{
		VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,	// VkStructureType		sType;
		DE_NULL,								// const void*			pNext;
		0u,										// VkFenceCreateFlags	flags;
	};
	const Unique<VkFence> fence(createFence(vk, device, &fenceParams));

	VK_CHECK(vk.queueSubmit(queue, 1u, &cmdBuffer, *fence));
	VK_CHECK(vk.waitForFences(device, 1u, &fence.get(), DE_TRUE, ~0ull));
}

// Helper classes

class Buffer
{
public:
										Buffer			(const DeviceInterface&		vk,
														 const VkDevice				device,
														 Allocator&					allocator,
														 const VkDeviceSize			sizeBytes,
														 const VkBufferUsageFlags	usage,
														 const MemoryRequirement	memoryRequirement);

	VkBuffer							get				(void) const { return *m_buffer; }
	VkBuffer							operator*		(void) const { return get(); }
	Allocation&							getAllocation	(void) const { return *m_allocation; }

private:
	de::MovePtr<Allocation>				m_allocation;
	Move<VkBuffer>						m_buffer;

	Buffer(const Buffer&);
	Buffer& operator=(const Buffer&);
};

Buffer::Buffer (const DeviceInterface&		vk,
				const VkDevice				device,
				Allocator&					allocator,
				const VkDeviceSize			sizeBytes,
				const VkBufferUsageFlags	usage,
				const MemoryRequirement		memoryRequirement)
{
	const VkBufferCreateInfo bufferParams =
	{
		VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,		// VkStructureType		sType;
		DE_NULL,									// const void*			pNext;
		sizeBytes,									// VkDeviceSize			size;
		usage,										// VkBufferUsageFlags	usage;
		0u,											// VkBufferCreateFlags	flags;
		VK_SHARING_MODE_EXCLUSIVE,					// VkSharingMode		sharingMode;
		0u,											// deUint32				queueFamilyCount;
		DE_NULL,									// const deUint32*		pQueueFamilyIndices;
	};

	m_buffer = createBuffer(vk, device, &bufferParams);
	m_allocation = allocator.allocate(getBufferMemoryRequirements(vk, device, *m_buffer), memoryRequirement);
	VK_CHECK(vk.bindBufferMemory(device, *m_buffer, m_allocation->getMemory(), m_allocation->getOffset()));
}

class Image
{
public:
										Image			(const DeviceInterface&		vk,
														 const VkDevice				device,
														 Allocator&					allocator,
														 const VkImageCreateInfo&	imageCreateInfo,
														 const MemoryRequirement	memoryRequirement);

	VkImage								get				(void) const { return *m_image; }
	VkImage								operator*		(void) const { return get(); }
	Allocation&							getAllocation	(void) const { return *m_allocation; }

private:
	de::MovePtr<Allocation>				m_allocation;
	Move<VkImage>						m_image;

	Image(const Image&);
	Image& operator=(const Image&);
};

Image::Image (const DeviceInterface&	vk,
			  const VkDevice			device,
			  Allocator&				allocator,
			  const VkImageCreateInfo&	imageCreateInfo,
			  const MemoryRequirement	memoryRequirement)
{
	m_image = createImage(vk, device, &imageCreateInfo);
	m_allocation = allocator.allocate(getImageMemoryRequirements(vk, device, *m_image), memoryRequirement);
	VK_CHECK(vk.bindImageMemory(device, *m_image, m_allocation->getMemory(), m_allocation->getOffset()));
}

// Test classes

enum BufferType
{
	BUFFER_TYPE_UNIFORM,
	BUFFER_TYPE_SSBO,
};

class SharedVarTest : public vkt::TestCase
{
public:
						SharedVarTest	(tcu::TestContext&	testCtx,
										 const std::string&	name,
										 const std::string&	description,
										 const tcu::UVec3&	localSize,
										 const tcu::UVec3&	workSize);

	void				initPrograms	(SourceCollections& sourceCollections) const;
	TestInstance*		createInstance	(Context& context) const;

private:
	const tcu::UVec3	m_localSize;
	const tcu::UVec3	m_workSize;
};

class SharedVarTestInstance : public vkt::TestInstance
{
public:
									SharedVarTestInstance	(Context&			context,
															 const tcu::UVec3&	localSize,
															 const tcu::UVec3&	workSize);

	tcu::TestStatus					iterate					(void);

private:
	const tcu::UVec3				m_localSize;
	const tcu::UVec3				m_workSize;
};

SharedVarTest::SharedVarTest (tcu::TestContext&		testCtx,
							  const std::string&	name,
							  const std::string&	description,
							  const tcu::UVec3&		localSize,
							  const tcu::UVec3&		workSize)
	: TestCase		(testCtx, name, description)
	, m_localSize	(localSize)
	, m_workSize	(workSize)
{
}

void SharedVarTest::initPrograms (SourceCollections& sourceCollections) const
{
	const int workGroupSize = multiplyComponents(m_localSize);
	const int workGroupCount = multiplyComponents(m_workSize);
	const int numValues = workGroupSize * workGroupCount;

	std::ostringstream src;
	src << "#version 310 es\n"
		<< "layout (local_size_x = " << m_localSize.x() << ", local_size_y = " << m_localSize.y() << ", local_size_z = " << m_localSize.z() << ") in;\n"
		<< "layout(binding = 0) writeonly buffer Output {\n"
		<< "    uint values[" << numValues << "];\n"
		<< "} sb_out;\n\n"
		<< "shared uint offsets[" << workGroupSize << "];\n\n"
		<< "void main (void) {\n"
		<< "    uint localSize  = gl_WorkGroupSize.x*gl_WorkGroupSize.y*gl_WorkGroupSize.z;\n"
		<< "    uint globalNdx  = gl_NumWorkGroups.x*gl_NumWorkGroups.y*gl_WorkGroupID.z + gl_NumWorkGroups.x*gl_WorkGroupID.y + gl_WorkGroupID.x;\n"
		<< "    uint globalOffs = localSize*globalNdx;\n"
		<< "    uint localOffs  = gl_WorkGroupSize.x*gl_WorkGroupSize.y*gl_LocalInvocationID.z + gl_WorkGroupSize.x*gl_LocalInvocationID.y + gl_LocalInvocationID.x;\n"
		<< "\n"
		<< "    offsets[localSize-localOffs-1u] = globalOffs + localOffs*localOffs;\n"
		<< "    memoryBarrierShared();\n"
		<< "    barrier();\n"
		<< "    sb_out.values[globalOffs + localOffs] = offsets[localOffs];\n"
		<< "}\n";

	sourceCollections.glslSources.add("comp") << glu::ComputeSource(src.str());
}

TestInstance* SharedVarTest::createInstance (Context& context) const
{
	return new SharedVarTestInstance(context, m_localSize, m_workSize);
}

SharedVarTestInstance::SharedVarTestInstance (Context& context, const tcu::UVec3& localSize, const tcu::UVec3& workSize)
	: TestInstance	(context)
	, m_localSize	(localSize)
	, m_workSize	(workSize)
{
}

tcu::TestStatus SharedVarTestInstance::iterate (void)
{
	const VkDevice			device				= m_context.getDevice();
	const DeviceInterface&	vk					= m_context.getDeviceInterface();
	const VkQueue			queue				= m_context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= m_context.getDefaultAllocator();

	const Unique<VkShaderModule>	shaderModule(createShaderModule(vk, device, m_context.getBinaryCollection().get("comp"), 0));
	const Unique<VkShader>			shader(makeComputeShader(vk, device, *shaderModule));

	const int workGroupSize = multiplyComponents(m_localSize);
	const int workGroupCount = multiplyComponents(m_workSize);

	// Create a buffer and host-visible memory for it

	const VkDeviceSize bufferSizeBytes = sizeof(deUint32) * workGroupSize * workGroupCount;
	const Buffer buffer(vk, device, allocator, bufferSizeBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryRequirement::HostVisible);

	// Create descriptor set

	const Unique<VkDescriptorSetLayout> descriptorSetLayout(
		DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(vk, device));

	const Unique<VkDescriptorPool> descriptorPool(
		DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		.build(vk, device, VK_DESCRIPTOR_POOL_USAGE_DYNAMIC, 1u));

	const Unique<VkDescriptorSet> descriptorSet = allocDescriptorSet(vk, device, *descriptorPool, VK_DESCRIPTOR_SET_USAGE_ONE_SHOT, *descriptorSetLayout);

	const VkDescriptorInfo descriptorInfo = makeDescriptorInfoForBuffer(*buffer, 0ull, bufferSizeBytes);
	DescriptorSetUpdateBuilder()
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &descriptorInfo)
		.update(vk, device);

	// Perform the computation

	const Unique<VkPipelineLayout> pipelineLayout(makePipelineLayout(vk, device, &descriptorSetLayout.get()));
	const Unique<VkPipeline> pipeline(makeComputePipeline(vk, device, *pipelineLayout, *shader));

	const VkBufferMemoryBarrier computeFinishBarrier = makeBufferMemoryBarrier(VK_MEMORY_OUTPUT_SHADER_WRITE_BIT, VK_MEMORY_INPUT_HOST_READ_BIT, *buffer, 0ull, bufferSizeBytes);
	const void* barriers[] = { &computeFinishBarrier };

	const Unique<VkCmdPool> cmdPool(makeCommandPool(vk, device, queueFamilyIndex));
	const Unique<VkCmdBuffer> cmdBuffer(makeCommandBuffer(vk, device, *cmdPool));

	// Start recording commands

	beginCommandBuffer(vk, *cmdBuffer);

	vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);
	vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);

	vk.cmdDispatch(*cmdBuffer, m_workSize.x(), m_workSize.y(), m_workSize.z());

	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, DE_FALSE, DE_LENGTH_OF_ARRAY(barriers), barriers);

	endCommandBuffer(vk, *cmdBuffer);

	// Wait for completion

	submitCommandsAndWait(vk, device, queue, *cmdBuffer);

	// Validate the results

	const Allocation& bufferAllocation = buffer.getAllocation();
	invalidateMappedMemoryRange(vk, device, bufferAllocation.getMemory(), bufferAllocation.getOffset(), bufferSizeBytes);

	const deUint32* bufferPtr = static_cast<deUint32*>(bufferAllocation.getHostPtr());

	for (int groupNdx = 0; groupNdx < workGroupCount; ++groupNdx)
	{
		const int globalOffset = groupNdx * workGroupSize;
		for (int localOffset = 0; localOffset < workGroupSize; ++localOffset)
		{
			const deUint32 res = bufferPtr[globalOffset + localOffset];
			const deUint32 ref = globalOffset + squared(workGroupSize - localOffset - 1);

			if (res != ref)
			{
				std::ostringstream msg;
				msg << "Comparison failed for Output.values[" << (globalOffset + localOffset) << "]";
				return tcu::TestStatus::fail(msg.str());
			}
		}
	}
	return tcu::TestStatus::pass("Compute succeeded");
}

class SharedVarAtomicOpTest : public vkt::TestCase
{
public:
						SharedVarAtomicOpTest	(tcu::TestContext&	testCtx,
												 const std::string&	name,
												 const std::string&	description,
												 const tcu::UVec3&	localSize,
												 const tcu::UVec3&	workSize);

	void				initPrograms			(SourceCollections& sourceCollections) const;
	TestInstance*		createInstance			(Context& context) const;

private:
	const tcu::UVec3	m_localSize;
	const tcu::UVec3	m_workSize;
};

class SharedVarAtomicOpTestInstance : public vkt::TestInstance
{
public:
									SharedVarAtomicOpTestInstance	(Context&			context,
																	 const tcu::UVec3&	localSize,
																	 const tcu::UVec3&	workSize);

	tcu::TestStatus					iterate							(void);

private:
	const tcu::UVec3				m_localSize;
	const tcu::UVec3				m_workSize;
};

SharedVarAtomicOpTest::SharedVarAtomicOpTest (tcu::TestContext&		testCtx,
											  const std::string&	name,
											  const std::string&	description,
											  const tcu::UVec3&		localSize,
											  const tcu::UVec3&		workSize)
	: TestCase		(testCtx, name, description)
	, m_localSize	(localSize)
	, m_workSize	(workSize)
{
}

void SharedVarAtomicOpTest::initPrograms (SourceCollections& sourceCollections) const
{
	const int workGroupSize = multiplyComponents(m_localSize);
	const int workGroupCount = multiplyComponents(m_workSize);
	const int numValues = workGroupSize * workGroupCount;

	std::ostringstream src;
	src << "#version 310 es\n"
		<< "layout (local_size_x = " << m_localSize.x() << ", local_size_y = " << m_localSize.y() << ", local_size_z = " << m_localSize.z() << ") in;\n"
		<< "layout(binding = 0) writeonly buffer Output {\n"
		<< "    uint values[" << numValues << "];\n"
		<< "} sb_out;\n\n"
		<< "shared uint count;\n\n"
		<< "void main (void) {\n"
		<< "    uint localSize  = gl_WorkGroupSize.x*gl_WorkGroupSize.y*gl_WorkGroupSize.z;\n"
		<< "    uint globalNdx  = gl_NumWorkGroups.x*gl_NumWorkGroups.y*gl_WorkGroupID.z + gl_NumWorkGroups.x*gl_WorkGroupID.y + gl_WorkGroupID.x;\n"
		<< "    uint globalOffs = localSize*globalNdx;\n"
		<< "\n"
		<< "    count = 0u;\n"
		<< "    memoryBarrierShared();\n"
		<< "    barrier();\n"
		<< "    uint oldVal = atomicAdd(count, 1u);\n"
		<< "    sb_out.values[globalOffs+oldVal] = oldVal+1u;\n"
		<< "}\n";

	sourceCollections.glslSources.add("comp") << glu::ComputeSource(src.str());
}

TestInstance* SharedVarAtomicOpTest::createInstance (Context& context) const
{
	return new SharedVarAtomicOpTestInstance(context, m_localSize, m_workSize);
}

SharedVarAtomicOpTestInstance::SharedVarAtomicOpTestInstance (Context& context, const tcu::UVec3& localSize, const tcu::UVec3& workSize)
	: TestInstance	(context)
	, m_localSize	(localSize)
	, m_workSize	(workSize)
{
}

tcu::TestStatus SharedVarAtomicOpTestInstance::iterate (void)
{
	const VkDevice			device				= m_context.getDevice();
	const DeviceInterface&	vk					= m_context.getDeviceInterface();
	const VkQueue			queue				= m_context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= m_context.getDefaultAllocator();

	const Unique<VkShaderModule>	shaderModule(createShaderModule(vk, device, m_context.getBinaryCollection().get("comp"), 0));
	const Unique<VkShader>			shader(makeComputeShader(vk, device, *shaderModule));

	const int workGroupSize = multiplyComponents(m_localSize);
	const int workGroupCount = multiplyComponents(m_workSize);

	// Create a buffer and host-visible memory for it

	const VkDeviceSize bufferSizeBytes = sizeof(deUint32) * workGroupSize * workGroupCount;
	const Buffer buffer(vk, device, allocator, bufferSizeBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryRequirement::HostVisible);

	// Create descriptor set

	const Unique<VkDescriptorSetLayout> descriptorSetLayout(
		DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(vk, device));

	const Unique<VkDescriptorPool> descriptorPool(
		DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		.build(vk, device, VK_DESCRIPTOR_POOL_USAGE_DYNAMIC, 1u));

	const Unique<VkDescriptorSet> descriptorSet = allocDescriptorSet(vk, device, *descriptorPool, VK_DESCRIPTOR_SET_USAGE_ONE_SHOT, *descriptorSetLayout);

	const VkDescriptorInfo descriptorInfo = makeDescriptorInfoForBuffer(*buffer, 0ull, bufferSizeBytes);
	DescriptorSetUpdateBuilder()
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &descriptorInfo)
		.update(vk, device);

	// Perform the computation

	const Unique<VkPipelineLayout> pipelineLayout(makePipelineLayout(vk, device, &descriptorSetLayout.get()));
	const Unique<VkPipeline> pipeline(makeComputePipeline(vk, device, *pipelineLayout, *shader));

	const VkBufferMemoryBarrier computeFinishBarrier = makeBufferMemoryBarrier(VK_MEMORY_OUTPUT_SHADER_WRITE_BIT, VK_MEMORY_INPUT_HOST_READ_BIT, *buffer, 0ull, bufferSizeBytes);
	const void* barriers[] = { &computeFinishBarrier };

	const Unique<VkCmdPool> cmdPool(makeCommandPool(vk, device, queueFamilyIndex));
	const Unique<VkCmdBuffer> cmdBuffer(makeCommandBuffer(vk, device, *cmdPool));

	// Start recording commands

	beginCommandBuffer(vk, *cmdBuffer);

	vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);
	vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);

	vk.cmdDispatch(*cmdBuffer, m_workSize.x(), m_workSize.y(), m_workSize.z());

	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, DE_FALSE, DE_LENGTH_OF_ARRAY(barriers), barriers);

	endCommandBuffer(vk, *cmdBuffer);

	// Wait for completion

	submitCommandsAndWait(vk, device, queue, *cmdBuffer);

	// Validate the results

	const Allocation& bufferAllocation = buffer.getAllocation();
	invalidateMappedMemoryRange(vk, device, bufferAllocation.getMemory(), bufferAllocation.getOffset(), bufferSizeBytes);

	const deUint32* bufferPtr = static_cast<deUint32*>(bufferAllocation.getHostPtr());

	for (int groupNdx = 0; groupNdx < workGroupCount; ++groupNdx)
	{
		const int globalOffset = groupNdx * workGroupSize;
		for (int localOffset = 0; localOffset < workGroupSize; ++localOffset)
		{
			const deUint32 res = bufferPtr[globalOffset + localOffset];
			const deUint32 ref = localOffset + 1;

			if (res != ref)
			{
				std::ostringstream msg;
				msg << "Comparison failed for Output.values[" << (globalOffset + localOffset) << "]";
				return tcu::TestStatus::fail(msg.str());
			}
		}
	}
	return tcu::TestStatus::pass("Compute succeeded");
}

class SSBOLocalBarrierTest : public vkt::TestCase
{
public:
						SSBOLocalBarrierTest	(tcu::TestContext&	testCtx,
												 const std::string& name,
												 const std::string&	description,
												 const tcu::UVec3&	localSize,
												 const tcu::UVec3&	workSize);

	void				initPrograms			(SourceCollections& sourceCollections) const;
	TestInstance*		createInstance			(Context& context) const;

private:
	const tcu::UVec3	m_localSize;
	const tcu::UVec3	m_workSize;
};

class SSBOLocalBarrierTestInstance : public vkt::TestInstance
{
public:
									SSBOLocalBarrierTestInstance	(Context&			context,
																	 const tcu::UVec3&	localSize,
																	 const tcu::UVec3&	workSize);

	tcu::TestStatus					iterate							(void);

private:
	const tcu::UVec3				m_localSize;
	const tcu::UVec3				m_workSize;
};

SSBOLocalBarrierTest::SSBOLocalBarrierTest (tcu::TestContext&	testCtx,
											const std::string&	name,
											const std::string&	description,
											const tcu::UVec3&	localSize,
											const tcu::UVec3&	workSize)
	: TestCase		(testCtx, name, description)
	, m_localSize	(localSize)
	, m_workSize	(workSize)
{
}

void SSBOLocalBarrierTest::initPrograms (SourceCollections& sourceCollections) const
{
	const int workGroupSize = multiplyComponents(m_localSize);
	const int workGroupCount = multiplyComponents(m_workSize);
	const int numValues = workGroupSize * workGroupCount;

	std::ostringstream src;
	src << "#version 310 es\n"
		<< "layout (local_size_x = " << m_localSize.x() << ", local_size_y = " << m_localSize.y() << ", local_size_z = " << m_localSize.z() << ") in;\n"
		<< "layout(binding = 0) coherent buffer Output {\n"
		<< "    uint values[" << numValues << "];\n"
		<< "} sb_out;\n\n"
		<< "void main (void) {\n"
		<< "    uint localSize  = gl_WorkGroupSize.x*gl_WorkGroupSize.y*gl_WorkGroupSize.z;\n"
		<< "    uint globalNdx  = gl_NumWorkGroups.x*gl_NumWorkGroups.y*gl_WorkGroupID.z + gl_NumWorkGroups.x*gl_WorkGroupID.y + gl_WorkGroupID.x;\n"
		<< "    uint globalOffs = localSize*globalNdx;\n"
		<< "    uint localOffs  = gl_WorkGroupSize.x*gl_WorkGroupSize.y*gl_LocalInvocationID.z + gl_WorkGroupSize.x*gl_LocalInvocationID.y + gl_LocalInvocationID.x;\n"
		<< "\n"
		<< "    sb_out.values[globalOffs + localOffs] = globalOffs;\n"
		<< "    memoryBarrierBuffer();\n"
		<< "    barrier();\n"
		<< "    sb_out.values[globalOffs + ((localOffs+1u)%localSize)] += localOffs;\n"		// += so we read and write
		<< "    memoryBarrierBuffer();\n"
		<< "    barrier();\n"
		<< "    sb_out.values[globalOffs + ((localOffs+2u)%localSize)] += localOffs;\n"
		<< "}\n";

	sourceCollections.glslSources.add("comp") << glu::ComputeSource(src.str());
}

TestInstance* SSBOLocalBarrierTest::createInstance (Context& context) const
{
	return new SSBOLocalBarrierTestInstance(context, m_localSize, m_workSize);
}

SSBOLocalBarrierTestInstance::SSBOLocalBarrierTestInstance (Context& context, const tcu::UVec3& localSize, const tcu::UVec3& workSize)
	: TestInstance	(context)
	, m_localSize	(localSize)
	, m_workSize	(workSize)
{
}

tcu::TestStatus SSBOLocalBarrierTestInstance::iterate (void)
{
	const VkDevice			device				= m_context.getDevice();
	const DeviceInterface&	vk					= m_context.getDeviceInterface();
	const VkQueue			queue				= m_context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= m_context.getDefaultAllocator();

	const Unique<VkShaderModule>	shaderModule(createShaderModule(vk, device, m_context.getBinaryCollection().get("comp"), 0));
	const Unique<VkShader>			shader(makeComputeShader(vk, device, *shaderModule));

	const int workGroupSize = multiplyComponents(m_localSize);
	const int workGroupCount = multiplyComponents(m_workSize);

	// Create a buffer and host-visible memory for it

	const VkDeviceSize bufferSizeBytes = sizeof(deUint32) * workGroupSize * workGroupCount;
	const Buffer buffer(vk, device, allocator, bufferSizeBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryRequirement::HostVisible);

	// Create descriptor set

	const Unique<VkDescriptorSetLayout> descriptorSetLayout(
		DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(vk, device));

	const Unique<VkDescriptorPool> descriptorPool(
		DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		.build(vk, device, VK_DESCRIPTOR_POOL_USAGE_DYNAMIC, 1u));

	const Unique<VkDescriptorSet> descriptorSet = allocDescriptorSet(vk, device, *descriptorPool, VK_DESCRIPTOR_SET_USAGE_ONE_SHOT, *descriptorSetLayout);

	const VkDescriptorInfo descriptorInfo = makeDescriptorInfoForBuffer(*buffer, 0ull, bufferSizeBytes);
	DescriptorSetUpdateBuilder()
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &descriptorInfo)
		.update(vk, device);

	// Perform the computation

	const Unique<VkPipelineLayout> pipelineLayout(makePipelineLayout(vk, device, &descriptorSetLayout.get()));
	const Unique<VkPipeline> pipeline(makeComputePipeline(vk, device, *pipelineLayout, *shader));

	const VkBufferMemoryBarrier computeFinishBarrier = makeBufferMemoryBarrier(VK_MEMORY_OUTPUT_SHADER_WRITE_BIT, VK_MEMORY_INPUT_HOST_READ_BIT, *buffer, 0ull, bufferSizeBytes);
	const void* barriers[] = { &computeFinishBarrier };

	const Unique<VkCmdPool> cmdPool(makeCommandPool(vk, device, queueFamilyIndex));
	const Unique<VkCmdBuffer> cmdBuffer(makeCommandBuffer(vk, device, *cmdPool));

	// Start recording commands

	beginCommandBuffer(vk, *cmdBuffer);

	vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);
	vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);

	vk.cmdDispatch(*cmdBuffer, m_workSize.x(), m_workSize.y(), m_workSize.z());

	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, DE_FALSE, DE_LENGTH_OF_ARRAY(barriers), barriers);

	endCommandBuffer(vk, *cmdBuffer);

	// Wait for completion

	submitCommandsAndWait(vk, device, queue, *cmdBuffer);

	// Validate the results

	const Allocation& bufferAllocation = buffer.getAllocation();
	invalidateMappedMemoryRange(vk, device, bufferAllocation.getMemory(), bufferAllocation.getOffset(), bufferSizeBytes);

	const deUint32* bufferPtr = static_cast<deUint32*>(bufferAllocation.getHostPtr());

	for (int groupNdx = 0; groupNdx < workGroupCount; ++groupNdx)
	{
		const int globalOffset = groupNdx * workGroupSize;
		for (int localOffset = 0; localOffset < workGroupSize; ++localOffset)
		{
			const deUint32	res		= bufferPtr[globalOffset + localOffset];
			const int		offs0	= localOffset - 1 < 0 ? ((localOffset + workGroupSize - 1) % workGroupSize) : ((localOffset - 1) % workGroupSize);
			const int		offs1	= localOffset - 2 < 0 ? ((localOffset + workGroupSize - 2) % workGroupSize) : ((localOffset - 2) % workGroupSize);
			const deUint32	ref		= static_cast<deUint32>(globalOffset + offs0 + offs1);

			if (res != ref)
			{
				std::ostringstream msg;
				msg << "Comparison failed for Output.values[" << (globalOffset + localOffset) << "]";
				return tcu::TestStatus::fail(msg.str());
			}
		}
	}
	return tcu::TestStatus::pass("Compute succeeded");
}

class CopyImageToSSBOTest : public vkt::TestCase
{
public:
						CopyImageToSSBOTest		(tcu::TestContext&	testCtx,
												 const std::string&	name,
												 const std::string&	description,
												 const tcu::UVec2&	localSize,
												 const tcu::UVec2&	imageSize);

	void				initPrograms			(SourceCollections& sourceCollections) const;
	TestInstance*		createInstance			(Context& context) const;

private:
	const tcu::UVec2	m_localSize;
	const tcu::UVec2	m_imageSize;
};

class CopyImageToSSBOTestInstance : public vkt::TestInstance
{
public:
									CopyImageToSSBOTestInstance		(Context&			context,
																	 const tcu::UVec2&	localSize,
																	 const tcu::UVec2&	imageSize);

	tcu::TestStatus					iterate							(void);

private:
	const tcu::UVec2				m_localSize;
	const tcu::UVec2				m_imageSize;
};

CopyImageToSSBOTest::CopyImageToSSBOTest (tcu::TestContext&		testCtx,
										  const std::string&	name,
										  const std::string&	description,
										  const tcu::UVec2&		localSize,
										  const tcu::UVec2&		imageSize)
	: TestCase		(testCtx, name, description)
	, m_localSize	(localSize)
	, m_imageSize	(imageSize)
{
	DE_ASSERT(m_imageSize.x() % m_localSize.x() == 0);
	DE_ASSERT(m_imageSize.y() % m_localSize.y() == 0);
}

void CopyImageToSSBOTest::initPrograms (SourceCollections& sourceCollections) const
{
	std::ostringstream src;
	src << "#version 310 es\n"
		<< "layout (local_size_x = " << m_localSize.x() << ", local_size_y = " << m_localSize.y() << ") in;\n"
		<< "layout(binding = 1, r32ui) readonly uniform highp uimage2D u_srcImg;\n"
		<< "layout(binding = 0) writeonly buffer Output {\n"
		<< "    uint values[" << (m_imageSize.x() * m_imageSize.y()) << "];\n"
		<< "} sb_out;\n\n"
		<< "void main (void) {\n"
		<< "    uint stride = gl_NumWorkGroups.x*gl_WorkGroupSize.x;\n"
		<< "    uint value  = imageLoad(u_srcImg, ivec2(gl_GlobalInvocationID.xy)).x;\n"
		<< "    sb_out.values[gl_GlobalInvocationID.y*stride + gl_GlobalInvocationID.x] = value;\n"
		<< "}\n";

	sourceCollections.glslSources.add("comp") << glu::ComputeSource(src.str());
}

TestInstance* CopyImageToSSBOTest::createInstance (Context& context) const
{
	return new CopyImageToSSBOTestInstance(context, m_localSize, m_imageSize);
}

CopyImageToSSBOTestInstance::CopyImageToSSBOTestInstance (Context& context, const tcu::UVec2& localSize, const tcu::UVec2& imageSize)
	: TestInstance	(context)
	, m_localSize	(localSize)
	, m_imageSize	(imageSize)
{
}

tcu::TestStatus CopyImageToSSBOTestInstance::iterate (void)
{
	const VkDevice			device				= m_context.getDevice();
	const DeviceInterface&	vk					= m_context.getDeviceInterface();
	const VkQueue			queue				= m_context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= m_context.getDefaultAllocator();

	const Unique<VkShaderModule>	shaderModule(createShaderModule(vk, device, m_context.getBinaryCollection().get("comp"), 0));
	const Unique<VkShader>			shader(makeComputeShader(vk, device, *shaderModule));

	// Create an image

	const VkImageCreateInfo imageParams =
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,									//	VkStructureType		sType;
		DE_NULL,																//	const void*			pNext;
		VK_IMAGE_TYPE_2D,														//	VkImageType			imageType;
		VK_FORMAT_R32_UINT,														//	VkFormat			format;
		{ m_imageSize.x(), m_imageSize.y(), 1 },								//	VkExtent3D			extent;
		1u,																		//	deUint32			mipLevels;
		1u,																		//	deUint32			arraySize;
		1u,																		//	deUint32			samples;
		VK_IMAGE_TILING_OPTIMAL,												//	VkImageTiling		tiling;
		VK_IMAGE_USAGE_TRANSFER_DESTINATION_BIT | VK_IMAGE_USAGE_STORAGE_BIT,	//	VkImageUsageFlags	usage;
		0u,																		//	VkImageCreateFlags	flags;
		VK_SHARING_MODE_EXCLUSIVE,												//	VkSharingMode		sharingMode;
		1u,																		//	deUint32			queueFamilyCount;
		&queueFamilyIndex,													//	const deUint32*		pQueueFamilyIndices;
		VK_IMAGE_LAYOUT_UNDEFINED,												//	VkImageLayout		initialLayout;
	};

	const Image image(vk, device, allocator, imageParams, MemoryRequirement::Any);

	// Staging buffer (source data for image)

	const deUint32 imageArea = multiplyComponents(m_imageSize);
	const VkDeviceSize bufferSizeBytes = sizeof(deUint32) * imageArea;

	const Buffer stagingBuffer(vk, device, allocator, bufferSizeBytes, VK_BUFFER_USAGE_TRANSFER_SOURCE_BIT, MemoryRequirement::HostVisible);

	// Populate the staging buffer with test data
	{
		de::Random rnd(0xab2c7);
		const Allocation& stagingBufferAllocation = stagingBuffer.getAllocation();
		deUint32* bufferPtr = static_cast<deUint32*>(stagingBufferAllocation.getHostPtr());
		for (deUint32 i = 0; i < imageArea; ++i)
			*bufferPtr++ = rnd.getUint32();

		flushMappedMemoryRange(vk, device, stagingBufferAllocation.getMemory(), stagingBufferAllocation.getOffset(), bufferSizeBytes);
	}

	// Create a buffer to store shader output

	const Buffer outputBuffer(vk, device, allocator, bufferSizeBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryRequirement::HostVisible);

	// Create descriptor set

	const Unique<VkDescriptorSetLayout> descriptorSetLayout(
		DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(vk, device));

	const Unique<VkDescriptorPool> descriptorPool(
		DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		.build(vk, device, VK_DESCRIPTOR_POOL_USAGE_DYNAMIC, 1u));

	const Unique<VkDescriptorSet> descriptorSet = allocDescriptorSet(vk, device, *descriptorPool, VK_DESCRIPTOR_SET_USAGE_ONE_SHOT, *descriptorSetLayout);

	// Set the bindings

	const VkImageSubresourceRange subresourceRange = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u);
	const VkImageViewCreateInfo imageViewParams =
	{
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,		// VkStructureType				sType;
		DE_NULL,										// const void*					pNext;
		*image,											// VkImage						image;
		VK_IMAGE_VIEW_TYPE_2D,							// VkImageViewType				viewType;
		VK_FORMAT_R32_UINT,								// VkFormat						format;
		makeChannelMappingRGBA(),						// VkChannelMapping				channels;
		subresourceRange,								// VkImageSubresourceRange		subresourceRange;
		0u,												// VkImageViewCreateFlags		flags;
	};
	const Unique<VkImageView> imageView(createImageView(vk, device, &imageViewParams));

	const VkDescriptorInfo imageDescriptorInfo = makeDescriptorInfoForImageView(*imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
	const VkDescriptorInfo bufferDescriptorInfo = makeDescriptorInfoForBuffer(*outputBuffer, 0ull, bufferSizeBytes);

	DescriptorSetUpdateBuilder()
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &bufferDescriptorInfo)
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1u), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &imageDescriptorInfo)
		.update(vk, device);

	// Perform the computation
	{
		const Unique<VkPipelineLayout> pipelineLayout(makePipelineLayout(vk, device, &descriptorSetLayout.get()));
		const Unique<VkPipeline> pipeline(makeComputePipeline(vk, device, *pipelineLayout, *shader));

		const VkBufferMemoryBarrier stagingBufferPostHostWriteBarrier = makeBufferMemoryBarrier(VK_MEMORY_OUTPUT_HOST_WRITE_BIT, VK_MEMORY_INPUT_TRANSFER_BIT, *stagingBuffer, 0ull, bufferSizeBytes);

		const VkImageMemoryBarrier imagePreCopyBarrier =
		{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,			// VkStructureType			sType;
			DE_NULL,										// const void*				pNext;
			0u,												// VkMemoryOutputFlags		outputMask;
			0u,												// VkMemoryInputFlags		inputMask;
			VK_IMAGE_LAYOUT_UNDEFINED,						// VkImageLayout			oldLayout;
			VK_IMAGE_LAYOUT_TRANSFER_DESTINATION_OPTIMAL,	// VkImageLayout			newLayout;
			VK_QUEUE_FAMILY_IGNORED,						// deUint32					srcQueueFamilyIndex;
			VK_QUEUE_FAMILY_IGNORED,						// deUint32					destQueueFamilyIndex;
			*image,											// VkImage					image;
			subresourceRange,								// VkImageSubresourceRange	subresourceRange;
		};

		const VkImageMemoryBarrier imagePostCopyBarrier =
		{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,			// VkStructureType			sType;
			DE_NULL,										// const void*				pNext;
			VK_MEMORY_OUTPUT_TRANSFER_BIT,					// VkMemoryOutputFlags		outputMask;
			VK_MEMORY_INPUT_SHADER_READ_BIT,				// VkMemoryInputFlags		inputMask;
			VK_IMAGE_LAYOUT_TRANSFER_DESTINATION_OPTIMAL,	// VkImageLayout			oldLayout;
			VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,		// VkImageLayout			newLayout;
			VK_QUEUE_FAMILY_IGNORED,						// deUint32					srcQueueFamilyIndex;
			VK_QUEUE_FAMILY_IGNORED,						// deUint32					destQueueFamilyIndex;
			*image,											// VkImage					image;
			subresourceRange,								// VkImageSubresourceRange	subresourceRange;
		};

		const void* preCopyBarriers[] = { &stagingBufferPostHostWriteBarrier, &imagePreCopyBarrier };
		const void* postCopyBarriers[] = { &imagePostCopyBarrier };

		const VkBufferMemoryBarrier computeFinishBarrier = makeBufferMemoryBarrier(VK_MEMORY_OUTPUT_SHADER_WRITE_BIT, VK_MEMORY_INPUT_HOST_READ_BIT, *outputBuffer, 0ull, bufferSizeBytes);
		const void* postComputeBarriers[] = { &computeFinishBarrier };

		const VkBufferImageCopy copyParams = makeBufferImageCopy(m_imageSize);
		const tcu::UVec2 workSize = m_imageSize / m_localSize;

		// Prepare the command buffer

		const Unique<VkCmdPool> cmdPool(makeCommandPool(vk, device, queueFamilyIndex));
		const Unique<VkCmdBuffer> cmdBuffer(makeCommandBuffer(vk, device, *cmdPool));

		// Start recording commands

		beginCommandBuffer(vk, *cmdBuffer);

		vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);
		vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);

		vk.cmdPipelineBarrier(*cmdBuffer, 0u, VK_PIPELINE_STAGE_TRANSFER_BIT, DE_FALSE, DE_LENGTH_OF_ARRAY(preCopyBarriers), preCopyBarriers);
		vk.cmdCopyBufferToImage(*cmdBuffer, *stagingBuffer, *image, VK_IMAGE_LAYOUT_TRANSFER_DESTINATION_OPTIMAL, 1u, &copyParams);
		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, DE_FALSE, DE_LENGTH_OF_ARRAY(postCopyBarriers), postCopyBarriers);

		vk.cmdDispatch(*cmdBuffer, workSize.x(), workSize.y(), 1u);
		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, DE_FALSE, DE_LENGTH_OF_ARRAY(postComputeBarriers), postComputeBarriers);

		endCommandBuffer(vk, *cmdBuffer);

		// Wait for completion

		submitCommandsAndWait(vk, device, queue, *cmdBuffer);
	}

	// Validate the results

	const Allocation& outputBufferAllocation = outputBuffer.getAllocation();
	invalidateMappedMemoryRange(vk, device, outputBufferAllocation.getMemory(), outputBufferAllocation.getOffset(), bufferSizeBytes);

	const deUint32* bufferPtr = static_cast<deUint32*>(outputBufferAllocation.getHostPtr());
	const deUint32* refBufferPtr = static_cast<deUint32*>(stagingBuffer.getAllocation().getHostPtr());

	for (deUint32 ndx = 0; ndx < imageArea; ++ndx)
	{
		const deUint32 res = *(bufferPtr + ndx);
		const deUint32 ref = *(refBufferPtr + ndx);

		if (res != ref)
		{
			std::ostringstream msg;
			msg << "Comparison failed for Output.values[" << ndx << "]";
			return tcu::TestStatus::fail(msg.str());
		}
	}
	return tcu::TestStatus::pass("Compute succeeded");
}

class CopySSBOToImageTest : public vkt::TestCase
{
public:
						CopySSBOToImageTest	(tcu::TestContext&	testCtx,
											 const std::string&	name,
											 const std::string&	description,
											 const tcu::UVec2&	localSize,
											 const tcu::UVec2&	imageSize);

	void				initPrograms		(SourceCollections& sourceCollections) const;
	TestInstance*		createInstance		(Context& context) const;

private:
	const tcu::UVec2	m_localSize;
	const tcu::UVec2	m_imageSize;
};

class CopySSBOToImageTestInstance : public vkt::TestInstance
{
public:
									CopySSBOToImageTestInstance	(Context&			context,
																 const tcu::UVec2&	localSize,
																 const tcu::UVec2&	imageSize);

	tcu::TestStatus					iterate						(void);

private:
	const tcu::UVec2				m_localSize;
	const tcu::UVec2				m_imageSize;
};

CopySSBOToImageTest::CopySSBOToImageTest (tcu::TestContext&		testCtx,
										  const std::string&	name,
										  const std::string&	description,
										  const tcu::UVec2&		localSize,
										  const tcu::UVec2&		imageSize)
	: TestCase		(testCtx, name, description)
	, m_localSize	(localSize)
	, m_imageSize	(imageSize)
{
	DE_ASSERT(m_imageSize.x() % m_localSize.x() == 0);
	DE_ASSERT(m_imageSize.y() % m_localSize.y() == 0);
}

void CopySSBOToImageTest::initPrograms (SourceCollections& sourceCollections) const
{
	std::ostringstream src;
	src << "#version 310 es\n"
		<< "layout (local_size_x = " << m_localSize.x() << ", local_size_y = " << m_localSize.y() << ") in;\n"
		<< "layout(binding = 1, r32ui) writeonly uniform highp uimage2D u_dstImg;\n"
		<< "layout(binding = 0) readonly buffer Input {\n"
		<< "    uint values[" << (m_imageSize.x() * m_imageSize.y()) << "];\n"
		<< "} sb_in;\n\n"
		<< "void main (void) {\n"
		<< "    uint stride = gl_NumWorkGroups.x*gl_WorkGroupSize.x;\n"
		<< "    uint value  = sb_in.values[gl_GlobalInvocationID.y*stride + gl_GlobalInvocationID.x];\n"
		<< "    imageStore(u_dstImg, ivec2(gl_GlobalInvocationID.xy), uvec4(value, 0, 0, 0));\n"
		<< "}\n";

	sourceCollections.glslSources.add("comp") << glu::ComputeSource(src.str());
}

TestInstance* CopySSBOToImageTest::createInstance (Context& context) const
{
	return new CopySSBOToImageTestInstance(context, m_localSize, m_imageSize);
}

CopySSBOToImageTestInstance::CopySSBOToImageTestInstance (Context& context, const tcu::UVec2& localSize, const tcu::UVec2& imageSize)
	: TestInstance	(context)
	, m_localSize	(localSize)
	, m_imageSize	(imageSize)
{
}

tcu::TestStatus CopySSBOToImageTestInstance::iterate (void)
{
	const VkDevice			device				= m_context.getDevice();
	const DeviceInterface&	vk					= m_context.getDeviceInterface();
	const VkQueue			queue				= m_context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= m_context.getDefaultAllocator();

	const Unique<VkShaderModule>	shaderModule(createShaderModule(vk, device, m_context.getBinaryCollection().get("comp"), 0));
	const Unique<VkShader>			shader(makeComputeShader(vk, device, *shaderModule));

	// Create an image

	const VkImageCreateInfo imageParams =
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,									//	VkStructureType		sType;
		DE_NULL,																//	const void*			pNext;
		VK_IMAGE_TYPE_2D,														//	VkImageType			imageType;
		VK_FORMAT_R32_UINT,														//	VkFormat			format;
		{ m_imageSize.x(), m_imageSize.y(), 1 },								//	VkExtent3D			extent;
		1u,																		//	deUint32			mipLevels;
		1u,																		//	deUint32			arraySize;
		1u,																		//	deUint32			samples;
		VK_IMAGE_TILING_OPTIMAL,												//	VkImageTiling		tiling;
		VK_IMAGE_USAGE_TRANSFER_SOURCE_BIT | VK_IMAGE_USAGE_STORAGE_BIT,		//	VkImageUsageFlags	usage;
		0u,																		//	VkImageCreateFlags	flags;
		VK_SHARING_MODE_EXCLUSIVE,												//	VkSharingMode		sharingMode;
		1u,																		//	deUint32			queueFamilyCount;
		&queueFamilyIndex,													//	const deUint32*		pQueueFamilyIndices;
		VK_IMAGE_LAYOUT_UNDEFINED,												//	VkImageLayout		initialLayout;
	};

	const Image image(vk, device, allocator, imageParams, MemoryRequirement::Any);

	// Create an input buffer (data to be read in the shader)

	const deUint32 imageArea = multiplyComponents(m_imageSize);
	const VkDeviceSize bufferSizeBytes = sizeof(deUint32) * imageArea;

	const Buffer inputBuffer(vk, device, allocator, bufferSizeBytes, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, MemoryRequirement::HostVisible);

	// Populate the buffer with test data
	{
		de::Random rnd(0x77238ac2);
		const Allocation& inputBufferAllocation = inputBuffer.getAllocation();
		deUint32* bufferPtr = static_cast<deUint32*>(inputBufferAllocation.getHostPtr());
		for (deUint32 i = 0; i < imageArea; ++i)
			*bufferPtr++ = rnd.getUint32();

		flushMappedMemoryRange(vk, device, inputBufferAllocation.getMemory(), inputBufferAllocation.getOffset(), bufferSizeBytes);
	}

	// Create a buffer to store shader output (copied from image data)

	const Buffer outputBuffer(vk, device, allocator, bufferSizeBytes, VK_BUFFER_USAGE_TRANSFER_DESTINATION_BIT, MemoryRequirement::HostVisible);

	// Create descriptor set

	const Unique<VkDescriptorSetLayout> descriptorSetLayout(
		DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(vk, device));

	const Unique<VkDescriptorPool> descriptorPool(
		DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		.build(vk, device, VK_DESCRIPTOR_POOL_USAGE_DYNAMIC, 1u));

	const Unique<VkDescriptorSet> descriptorSet = allocDescriptorSet(vk, device, *descriptorPool, VK_DESCRIPTOR_SET_USAGE_ONE_SHOT, *descriptorSetLayout);

	// Set the bindings

	const VkImageSubresourceRange subresourceRange = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u);
	const VkImageViewCreateInfo imageViewParams =
	{
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,		// VkStructureType				sType;
		DE_NULL,										// const void*					pNext;
		*image,											// VkImage						image;
		VK_IMAGE_VIEW_TYPE_2D,							// VkImageViewType				viewType;
		VK_FORMAT_R32_UINT,								// VkFormat						format;
		makeChannelMappingRGBA(),						// VkChannelMapping				channels;
		subresourceRange,								// VkImageSubresourceRange		subresourceRange;
		0u,												// VkImageViewCreateFlags		flags;
	};
	const Unique<VkImageView> imageView(createImageView(vk, device, &imageViewParams));

	const VkDescriptorInfo imageDescriptorInfo = makeDescriptorInfoForImageView(*imageView, VK_IMAGE_LAYOUT_GENERAL);
	const VkDescriptorInfo bufferDescriptorInfo = makeDescriptorInfoForBuffer(*inputBuffer, 0ull, bufferSizeBytes);

	DescriptorSetUpdateBuilder()
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &bufferDescriptorInfo)
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1u), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &imageDescriptorInfo)
		.update(vk, device);

	// Perform the computation
	{
		const Unique<VkPipelineLayout> pipelineLayout(makePipelineLayout(vk, device, &descriptorSetLayout.get()));
		const Unique<VkPipeline> pipeline(makeComputePipeline(vk, device, *pipelineLayout, *shader));

		const VkBufferMemoryBarrier inputBufferPostHostWriteBarrier = makeBufferMemoryBarrier(VK_MEMORY_OUTPUT_HOST_WRITE_BIT, VK_MEMORY_INPUT_SHADER_READ_BIT, *inputBuffer, 0ull, bufferSizeBytes);

		const VkImageMemoryBarrier imageLayoutBarrier =
		{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,			// VkStructureType			sType;
			DE_NULL,										// const void*				pNext;
			0u,												// VkMemoryOutputFlags		outputMask;
			0u,												// VkMemoryInputFlags		inputMask;
			VK_IMAGE_LAYOUT_UNDEFINED,						// VkImageLayout			oldLayout;
			VK_IMAGE_LAYOUT_GENERAL,						// VkImageLayout			newLayout;
			VK_QUEUE_FAMILY_IGNORED,						// deUint32					srcQueueFamilyIndex;
			VK_QUEUE_FAMILY_IGNORED,						// deUint32					destQueueFamilyIndex;
			*image,											// VkImage					image;
			subresourceRange,								// VkImageSubresourceRange	subresourceRange;
		};

		const VkImageMemoryBarrier imagePreCopyBarrier =
		{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,			// VkStructureType			sType;
			DE_NULL,										// const void*				pNext;
			VK_MEMORY_OUTPUT_SHADER_WRITE_BIT,				// VkMemoryOutputFlags		outputMask;
			VK_MEMORY_INPUT_TRANSFER_BIT,					// VkMemoryInputFlags		inputMask;
			VK_IMAGE_LAYOUT_GENERAL,						// VkImageLayout			oldLayout;
			VK_IMAGE_LAYOUT_TRANSFER_SOURCE_OPTIMAL,		// VkImageLayout			newLayout;
			VK_QUEUE_FAMILY_IGNORED,						// deUint32					srcQueueFamilyIndex;
			VK_QUEUE_FAMILY_IGNORED,						// deUint32					destQueueFamilyIndex;
			*image,											// VkImage					image;
			subresourceRange,								// VkImageSubresourceRange	subresourceRange;
		};

		const VkBufferMemoryBarrier outputBufferPostCopyBarrier = makeBufferMemoryBarrier(VK_MEMORY_OUTPUT_TRANSFER_BIT, VK_MEMORY_INPUT_HOST_READ_BIT, *outputBuffer, 0ull, bufferSizeBytes);

		const void* preComputeBarriers[] = { &inputBufferPostHostWriteBarrier, &imageLayoutBarrier };
		const void* preCopyBarriers[] = { &imagePreCopyBarrier };
		const void* postCopyBarriers[] = { &outputBufferPostCopyBarrier };

		const VkBufferImageCopy copyParams = makeBufferImageCopy(m_imageSize);
		const tcu::UVec2 workSize = m_imageSize / m_localSize;

		// Prepare the command buffer

		const Unique<VkCmdPool> cmdPool(makeCommandPool(vk, device, queueFamilyIndex));
		const Unique<VkCmdBuffer> cmdBuffer(makeCommandBuffer(vk, device, *cmdPool));

		// Start recording commands

		beginCommandBuffer(vk, *cmdBuffer);

		vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);
		vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);

		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, DE_FALSE, DE_LENGTH_OF_ARRAY(preComputeBarriers), preComputeBarriers);
		vk.cmdDispatch(*cmdBuffer, workSize.x(), workSize.y(), 1u);

		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, DE_FALSE, DE_LENGTH_OF_ARRAY(preCopyBarriers), preCopyBarriers);
		vk.cmdCopyImageToBuffer(*cmdBuffer, *image, VK_IMAGE_LAYOUT_TRANSFER_SOURCE_OPTIMAL, *outputBuffer, 1u, &copyParams);
		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, DE_FALSE, DE_LENGTH_OF_ARRAY(postCopyBarriers), postCopyBarriers);

		endCommandBuffer(vk, *cmdBuffer);

		// Wait for completion

		submitCommandsAndWait(vk, device, queue, *cmdBuffer);
	}

	// Validate the results

	const Allocation& outputBufferAllocation = outputBuffer.getAllocation();
	invalidateMappedMemoryRange(vk, device, outputBufferAllocation.getMemory(), outputBufferAllocation.getOffset(), bufferSizeBytes);

	const deUint32* bufferPtr = static_cast<deUint32*>(outputBufferAllocation.getHostPtr());
	const deUint32* refBufferPtr = static_cast<deUint32*>(inputBuffer.getAllocation().getHostPtr());

	for (deUint32 ndx = 0; ndx < imageArea; ++ndx)
	{
		const deUint32 res = *(bufferPtr + ndx);
		const deUint32 ref = *(refBufferPtr + ndx);

		if (res != ref)
		{
			std::ostringstream msg;
			msg << "Comparison failed for pixel " << ndx;
			return tcu::TestStatus::fail(msg.str());
		}
	}
	return tcu::TestStatus::pass("Compute succeeded");
}

class BufferToBufferInvertTest : public vkt::TestCase
{
public:
	void								initPrograms				(SourceCollections&	sourceCollections) const;
	TestInstance*						createInstance				(Context& context) const;

	static BufferToBufferInvertTest*	UBOToSSBOInvertCase			(tcu::TestContext&	testCtx,
																	 const std::string& name,
																	 const std::string& description,
																	 const deUint32		numValues,
																	 const tcu::UVec3&	localSize,
																	 const tcu::UVec3&	workSize);

	static BufferToBufferInvertTest*	CopyInvertSSBOCase			(tcu::TestContext&	testCtx,
																	 const std::string& name,
																	 const std::string& description,
																	 const deUint32		numValues,
																	 const tcu::UVec3&	localSize,
																	 const tcu::UVec3&	workSize);

private:
										BufferToBufferInvertTest	(tcu::TestContext&	testCtx,
																	 const std::string& name,
																	 const std::string& description,
																	 const deUint32		numValues,
																	 const tcu::UVec3&	localSize,
																	 const tcu::UVec3&	workSize,
																	 const BufferType	bufferType);

	const BufferType					m_bufferType;
	const deUint32						m_numValues;
	const tcu::UVec3					m_localSize;
	const tcu::UVec3					m_workSize;
};

class BufferToBufferInvertTestInstance : public vkt::TestInstance
{
public:
									BufferToBufferInvertTestInstance	(Context&			context,
																		 const deUint32		numValues,
																		 const tcu::UVec3&	localSize,
																		 const tcu::UVec3&	workSize,
																		 const BufferType	bufferType);

	tcu::TestStatus					iterate								(void);

private:
	const BufferType				m_bufferType;
	const deUint32					m_numValues;
	const tcu::UVec3				m_localSize;
	const tcu::UVec3				m_workSize;
};

BufferToBufferInvertTest::BufferToBufferInvertTest (tcu::TestContext&	testCtx,
													const std::string&	name,
													const std::string&	description,
													const deUint32		numValues,
													const tcu::UVec3&	localSize,
													const tcu::UVec3&	workSize,
													const BufferType	bufferType)
	: TestCase		(testCtx, name, description)
	, m_bufferType	(bufferType)
	, m_numValues	(numValues)
	, m_localSize	(localSize)
	, m_workSize	(workSize)
{
	DE_ASSERT(m_numValues % (multiplyComponents(m_workSize) * multiplyComponents(m_localSize)) == 0);
	DE_ASSERT(m_bufferType == BUFFER_TYPE_UNIFORM || m_bufferType == BUFFER_TYPE_SSBO);
}

BufferToBufferInvertTest* BufferToBufferInvertTest::UBOToSSBOInvertCase (tcu::TestContext&	testCtx,
																		 const std::string&	name,
																		 const std::string&	description,
																		 const deUint32		numValues,
																		 const tcu::UVec3&	localSize,
																		 const tcu::UVec3&	workSize)
{
	return new BufferToBufferInvertTest(testCtx, name, description, numValues, localSize, workSize, BUFFER_TYPE_UNIFORM);
}

BufferToBufferInvertTest* BufferToBufferInvertTest::CopyInvertSSBOCase (tcu::TestContext&	testCtx,
																		const std::string&	name,
																		const std::string&	description,
																		const deUint32		numValues,
																		const tcu::UVec3&	localSize,
																		const tcu::UVec3&	workSize)
{
	return new BufferToBufferInvertTest(testCtx, name, description, numValues, localSize, workSize, BUFFER_TYPE_SSBO);
}

void BufferToBufferInvertTest::initPrograms (SourceCollections& sourceCollections) const
{
	std::ostringstream src;
	if (m_bufferType == BUFFER_TYPE_UNIFORM)
	{
		src << "#version 310 es\n"
			<< "layout (local_size_x = " << m_localSize.x() << ", local_size_y = " << m_localSize.y() << ", local_size_z = " << m_localSize.z() << ") in;\n"
			<< "layout(binding = 0) readonly uniform Input {\n"
			<< "    uint values[" << m_numValues << "];\n"
			<< "} ub_in;\n"
			<< "layout(binding = 1) writeonly buffer Output {\n"
			<< "    uint values[" << m_numValues << "];\n"
			<< "} sb_out;\n"
			<< "void main (void) {\n"
			<< "    uvec3 size           = gl_NumWorkGroups * gl_WorkGroupSize;\n"
			<< "    uint numValuesPerInv = uint(ub_in.values.length()) / (size.x*size.y*size.z);\n"
			<< "    uint groupNdx        = size.x*size.y*gl_GlobalInvocationID.z + size.x*gl_GlobalInvocationID.y + gl_GlobalInvocationID.x;\n"
			<< "    uint offset          = numValuesPerInv*groupNdx;\n"
			<< "\n"
			<< "    for (uint ndx = 0u; ndx < numValuesPerInv; ndx++)\n"
			<< "        sb_out.values[offset + ndx] = ~ub_in.values[offset + ndx];\n"
			<< "}\n";
	}
	else if (m_bufferType == BUFFER_TYPE_SSBO)
	{
		src << "#version 310 es\n"
			<< "layout (local_size_x = " << m_localSize.x() << ", local_size_y = " << m_localSize.y() << ", local_size_z = " << m_localSize.z() << ") in;\n"
			<< "layout(binding = 0) readonly buffer Input {\n"
			<< "    uint values[" << m_numValues << "];\n"
			<< "} sb_in;\n"
			<< "layout (binding = 1) writeonly buffer Output {\n"
			<< "    uint values[" << m_numValues << "];\n"
			<< "} sb_out;\n"
			<< "void main (void) {\n"
			<< "    uvec3 size           = gl_NumWorkGroups * gl_WorkGroupSize;\n"
			<< "    uint numValuesPerInv = uint(sb_in.values.length()) / (size.x*size.y*size.z);\n"
			<< "    uint groupNdx        = size.x*size.y*gl_GlobalInvocationID.z + size.x*gl_GlobalInvocationID.y + gl_GlobalInvocationID.x;\n"
			<< "    uint offset          = numValuesPerInv*groupNdx;\n"
			<< "\n"
			<< "    for (uint ndx = 0u; ndx < numValuesPerInv; ndx++)\n"
			<< "        sb_out.values[offset + ndx] = ~sb_in.values[offset + ndx];\n"
			<< "}\n";
	}

	sourceCollections.glslSources.add("comp") << glu::ComputeSource(src.str());
}

TestInstance* BufferToBufferInvertTest::createInstance (Context& context) const
{
	return new BufferToBufferInvertTestInstance(context, m_numValues, m_localSize, m_workSize, m_bufferType);
}

BufferToBufferInvertTestInstance::BufferToBufferInvertTestInstance (Context&			context,
																	const deUint32		numValues,
																	const tcu::UVec3&	localSize,
																	const tcu::UVec3&	workSize,
																	const BufferType	bufferType)
	: TestInstance	(context)
	, m_bufferType	(bufferType)
	, m_numValues	(numValues)
	, m_localSize	(localSize)
	, m_workSize	(workSize)
{
}

tcu::TestStatus BufferToBufferInvertTestInstance::iterate (void)
{
	const VkDevice			device				= m_context.getDevice();
	const DeviceInterface&	vk					= m_context.getDeviceInterface();
	const VkQueue			queue				= m_context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= m_context.getDefaultAllocator();

	const Unique<VkShaderModule>	shaderModule(createShaderModule(vk, device, m_context.getBinaryCollection().get("comp"), 0));
	const Unique<VkShader>			shader(makeComputeShader(vk, device, *shaderModule));

	// Customize the test based on buffer type

	const VkBufferUsageFlags inputBufferUsageFlags		= (m_bufferType == BUFFER_TYPE_UNIFORM ? VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT : VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);
	const VkDescriptorType inputBufferDescriptorType	= (m_bufferType == BUFFER_TYPE_UNIFORM ? VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER : VK_DESCRIPTOR_TYPE_STORAGE_BUFFER);
	const deUint32 randomSeed							= (m_bufferType == BUFFER_TYPE_UNIFORM ? 0x111223f : 0x124fef);

	// Create an input buffer

	const VkDeviceSize bufferSizeBytes = sizeof(deUint32) * m_numValues;
	const Buffer inputBuffer(vk, device, allocator, bufferSizeBytes, inputBufferUsageFlags, MemoryRequirement::HostVisible);

	// Fill the input buffer with data
	{
		de::Random rnd(randomSeed);
		const Allocation& inputBufferAllocation = inputBuffer.getAllocation();
		deUint32* bufferPtr = static_cast<deUint32*>(inputBufferAllocation.getHostPtr());
		for (deUint32 i = 0; i < m_numValues; ++i)
			*bufferPtr++ = rnd.getUint32();

		flushMappedMemoryRange(vk, device, inputBufferAllocation.getMemory(), inputBufferAllocation.getOffset(), bufferSizeBytes);
	}

	// Create an output buffer

	const Buffer outputBuffer(vk, device, allocator, bufferSizeBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryRequirement::HostVisible);

	// Create descriptor set

	const Unique<VkDescriptorSetLayout> descriptorSetLayout(
		DescriptorSetLayoutBuilder()
		.addSingleBinding(inputBufferDescriptorType, VK_SHADER_STAGE_COMPUTE_BIT)
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(vk, device));

	const Unique<VkDescriptorPool> descriptorPool(
		DescriptorPoolBuilder()
		.addType(inputBufferDescriptorType)
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		.build(vk, device, VK_DESCRIPTOR_POOL_USAGE_DYNAMIC, 1u));

	const Unique<VkDescriptorSet> descriptorSet = allocDescriptorSet(vk, device, *descriptorPool, VK_DESCRIPTOR_SET_USAGE_ONE_SHOT, *descriptorSetLayout);

	const VkDescriptorInfo inputBufferDescriptorInfo = makeDescriptorInfoForBuffer(*inputBuffer, 0ull, bufferSizeBytes);
	const VkDescriptorInfo outputBufferDescriptorInfo = makeDescriptorInfoForBuffer(*outputBuffer, 0ull, bufferSizeBytes);
	DescriptorSetUpdateBuilder()
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), inputBufferDescriptorType, &inputBufferDescriptorInfo)
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &outputBufferDescriptorInfo)
		.update(vk, device);

	// Perform the computation

	const Unique<VkPipelineLayout> pipelineLayout(makePipelineLayout(vk, device, &descriptorSetLayout.get()));
	const Unique<VkPipeline> pipeline(makeComputePipeline(vk, device, *pipelineLayout, *shader));

	const VkBufferMemoryBarrier hostWriteBarrier = makeBufferMemoryBarrier(VK_MEMORY_OUTPUT_HOST_WRITE_BIT, VK_MEMORY_INPUT_SHADER_READ_BIT, *inputBuffer, 0ull, bufferSizeBytes);
	const void* preComputeBarriers[] = { &hostWriteBarrier };

	const VkBufferMemoryBarrier shaderWriteBarrier = makeBufferMemoryBarrier(VK_MEMORY_OUTPUT_SHADER_WRITE_BIT, VK_MEMORY_INPUT_HOST_READ_BIT, *outputBuffer, 0ull, bufferSizeBytes);
	const void* postComputeBarriers[] = { &shaderWriteBarrier };

	const Unique<VkCmdPool> cmdPool(makeCommandPool(vk, device, queueFamilyIndex));
	const Unique<VkCmdBuffer> cmdBuffer(makeCommandBuffer(vk, device, *cmdPool));

	// Start recording commands

	beginCommandBuffer(vk, *cmdBuffer);

	vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);
	vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);

	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, DE_FALSE, DE_LENGTH_OF_ARRAY(preComputeBarriers), preComputeBarriers);
	vk.cmdDispatch(*cmdBuffer, m_workSize.x(), m_workSize.y(), m_workSize.z());
	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, DE_FALSE, DE_LENGTH_OF_ARRAY(postComputeBarriers), postComputeBarriers);

	endCommandBuffer(vk, *cmdBuffer);

	// Wait for completion

	submitCommandsAndWait(vk, device, queue, *cmdBuffer);

	// Validate the results

	const Allocation& outputBufferAllocation = outputBuffer.getAllocation();
	invalidateMappedMemoryRange(vk, device, outputBufferAllocation.getMemory(), outputBufferAllocation.getOffset(), bufferSizeBytes);

	const deUint32* bufferPtr = static_cast<deUint32*>(outputBufferAllocation.getHostPtr());
	const deUint32* refBufferPtr = static_cast<deUint32*>(inputBuffer.getAllocation().getHostPtr());

	for (deUint32 ndx = 0; ndx < m_numValues; ++ndx)
	{
		const deUint32 res = bufferPtr[ndx];
		const deUint32 ref = ~refBufferPtr[ndx];

		if (res != ref)
		{
			std::ostringstream msg;
			msg << "Comparison failed for Output.values[" << ndx << "]";
			return tcu::TestStatus::fail(msg.str());
		}
	}
	return tcu::TestStatus::pass("Compute succeeded");
}

class InvertSSBOInPlaceTest : public vkt::TestCase
{
public:
						InvertSSBOInPlaceTest	(tcu::TestContext&	testCtx,
												 const std::string&	name,
												 const std::string&	description,
												 const deUint32		numValues,
												 const bool			sized,
												 const tcu::UVec3&	localSize,
												 const tcu::UVec3&	workSize);


	void				initPrograms			(SourceCollections& sourceCollections) const;
	TestInstance*		createInstance			(Context& context) const;

private:
	const deUint32		m_numValues;
	const bool			m_sized;
	const tcu::UVec3	m_localSize;
	const tcu::UVec3	m_workSize;
};

class InvertSSBOInPlaceTestInstance : public vkt::TestInstance
{
public:
									InvertSSBOInPlaceTestInstance	(Context&			context,
																	 const deUint32		numValues,
																	 const bool			sized,
																	 const tcu::UVec3&	localSize,
																	 const tcu::UVec3&	workSize);

	tcu::TestStatus					iterate							(void);

private:
	const deUint32					m_numValues;
	const bool						m_sized;
	const tcu::UVec3				m_localSize;
	const tcu::UVec3				m_workSize;
};

InvertSSBOInPlaceTest::InvertSSBOInPlaceTest (tcu::TestContext&		testCtx,
											  const std::string&	name,
											  const std::string&	description,
											  const deUint32		numValues,
											  const bool			sized,
											  const tcu::UVec3&		localSize,
											  const tcu::UVec3&		workSize)
	: TestCase		(testCtx, name, description)
	, m_numValues	(numValues)
	, m_sized		(sized)
	, m_localSize	(localSize)
	, m_workSize	(workSize)
{
	DE_ASSERT(m_numValues % (multiplyComponents(m_workSize) * multiplyComponents(m_localSize)) == 0);
}

void InvertSSBOInPlaceTest::initPrograms (SourceCollections& sourceCollections) const
{
	std::ostringstream src;
	src << "#version 310 es\n"
		<< "layout (local_size_x = " << m_localSize.x() << ", local_size_y = " << m_localSize.y() << ", local_size_z = " << m_localSize.z() << ") in;\n"
		<< "layout(binding = 0) buffer InOut {\n"
		<< "    uint values[" << (m_sized ? de::toString(m_numValues) : "") << "];\n"
		<< "} sb_inout;\n"
		<< "void main (void) {\n"
		<< "    uvec3 size           = gl_NumWorkGroups * gl_WorkGroupSize;\n"
		<< "    uint numValuesPerInv = uint(sb_inout.values.length()) / (size.x*size.y*size.z);\n"
		<< "    uint groupNdx        = size.x*size.y*gl_GlobalInvocationID.z + size.x*gl_GlobalInvocationID.y + gl_GlobalInvocationID.x;\n"
		<< "    uint offset          = numValuesPerInv*groupNdx;\n"
		<< "\n"
		<< "    for (uint ndx = 0u; ndx < numValuesPerInv; ndx++)\n"
		<< "        sb_inout.values[offset + ndx] = ~sb_inout.values[offset + ndx];\n"
		<< "}\n";

	sourceCollections.glslSources.add("comp") << glu::ComputeSource(src.str());
}

TestInstance* InvertSSBOInPlaceTest::createInstance (Context& context) const
{
	return new InvertSSBOInPlaceTestInstance(context, m_numValues, m_sized, m_localSize, m_workSize);
}

InvertSSBOInPlaceTestInstance::InvertSSBOInPlaceTestInstance (Context&			context,
															  const deUint32	numValues,
															  const bool		sized,
															  const tcu::UVec3&	localSize,
															  const tcu::UVec3&	workSize)
	: TestInstance	(context)
	, m_numValues	(numValues)
	, m_sized		(sized)
	, m_localSize	(localSize)
	, m_workSize	(workSize)
{
}

tcu::TestStatus InvertSSBOInPlaceTestInstance::iterate (void)
{
	const VkDevice			device				= m_context.getDevice();
	const DeviceInterface&	vk					= m_context.getDeviceInterface();
	const VkQueue			queue				= m_context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= m_context.getDefaultAllocator();

	const Unique<VkShaderModule>	shaderModule(createShaderModule(vk, device, m_context.getBinaryCollection().get("comp"), 0));
	const Unique<VkShader>			shader(makeComputeShader(vk, device, *shaderModule));

	// Create an input/output buffer

	const VkDeviceSize bufferSizeBytes = sizeof(deUint32) * m_numValues;
	const Buffer buffer(vk, device, allocator, bufferSizeBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryRequirement::HostVisible);

	// Fill the buffer with data

	typedef std::vector<deUint32> data_vector_t;
	data_vector_t inputData(m_numValues);

	{
		de::Random rnd(0x82ce7f);
		const Allocation& bufferAllocation = buffer.getAllocation();
		deUint32* bufferPtr = static_cast<deUint32*>(bufferAllocation.getHostPtr());
		for (deUint32 i = 0; i < m_numValues; ++i)
			inputData[i] = *bufferPtr++ = rnd.getUint32();

		flushMappedMemoryRange(vk, device, bufferAllocation.getMemory(), bufferAllocation.getOffset(), bufferSizeBytes);
	}

	// Create descriptor set

	const Unique<VkDescriptorSetLayout> descriptorSetLayout(
		DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(vk, device));

	const Unique<VkDescriptorPool> descriptorPool(
		DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		.build(vk, device, VK_DESCRIPTOR_POOL_USAGE_DYNAMIC, 1u));

	const Unique<VkDescriptorSet> descriptorSet = allocDescriptorSet(vk, device, *descriptorPool, VK_DESCRIPTOR_SET_USAGE_ONE_SHOT, *descriptorSetLayout);

	const VkDescriptorInfo bufferDescriptorInfo = makeDescriptorInfoForBuffer(*buffer, 0ull, bufferSizeBytes);
	DescriptorSetUpdateBuilder()
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &bufferDescriptorInfo)
		.update(vk, device);

	// Perform the computation

	const Unique<VkPipelineLayout> pipelineLayout(makePipelineLayout(vk, device, &descriptorSetLayout.get()));
	const Unique<VkPipeline> pipeline(makeComputePipeline(vk, device, *pipelineLayout, *shader));

	const VkBufferMemoryBarrier hostWriteBarrier = makeBufferMemoryBarrier(VK_MEMORY_OUTPUT_HOST_WRITE_BIT, VK_MEMORY_INPUT_SHADER_READ_BIT, *buffer, 0ull, bufferSizeBytes);
	const void* preComputeBarriers[] = { &hostWriteBarrier };

	const VkBufferMemoryBarrier shaderWriteBarrier = makeBufferMemoryBarrier(VK_MEMORY_OUTPUT_SHADER_WRITE_BIT, VK_MEMORY_INPUT_HOST_READ_BIT, *buffer, 0ull, bufferSizeBytes);
	const void* postComputeBarriers[] = { &shaderWriteBarrier };

	const Unique<VkCmdPool> cmdPool(makeCommandPool(vk, device, queueFamilyIndex));
	const Unique<VkCmdBuffer> cmdBuffer(makeCommandBuffer(vk, device, *cmdPool));

	// Start recording commands

	beginCommandBuffer(vk, *cmdBuffer);

	vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);
	vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);

	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, DE_FALSE, DE_LENGTH_OF_ARRAY(preComputeBarriers), preComputeBarriers);
	vk.cmdDispatch(*cmdBuffer, m_workSize.x(), m_workSize.y(), m_workSize.z());
	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, DE_FALSE, DE_LENGTH_OF_ARRAY(postComputeBarriers), postComputeBarriers);

	endCommandBuffer(vk, *cmdBuffer);

	// Wait for completion

	submitCommandsAndWait(vk, device, queue, *cmdBuffer);

	// Validate the results

	const Allocation& bufferAllocation = buffer.getAllocation();
	invalidateMappedMemoryRange(vk, device, bufferAllocation.getMemory(), bufferAllocation.getOffset(), bufferSizeBytes);

	const deUint32* bufferPtr = static_cast<deUint32*>(bufferAllocation.getHostPtr());

	for (deUint32 ndx = 0; ndx < m_numValues; ++ndx)
	{
		const deUint32 res = bufferPtr[ndx];
		const deUint32 ref = ~inputData[ndx];

		if (res != ref)
		{
			std::ostringstream msg;
			msg << "Comparison failed for InOut.values[" << ndx << "]";
			return tcu::TestStatus::fail(msg.str());
		}
	}
	return tcu::TestStatus::pass("Compute succeeded");
}

class WriteToMultipleSSBOTest : public vkt::TestCase
{
public:
						WriteToMultipleSSBOTest	(tcu::TestContext&	testCtx,
												 const std::string&	name,
												 const std::string&	description,
												 const deUint32		numValues,
												 const bool			sized,
												 const tcu::UVec3&	localSize,
												 const tcu::UVec3&	workSize);

	void				initPrograms			(SourceCollections& sourceCollections) const;
	TestInstance*		createInstance			(Context& context) const;

private:
	const deUint32		m_numValues;
	const bool			m_sized;
	const tcu::UVec3	m_localSize;
	const tcu::UVec3	m_workSize;
};

class WriteToMultipleSSBOTestInstance : public vkt::TestInstance
{
public:
									WriteToMultipleSSBOTestInstance	(Context&			context,
																	 const deUint32		numValues,
																	 const bool			sized,
																	 const tcu::UVec3&	localSize,
																	 const tcu::UVec3&	workSize);

	tcu::TestStatus					iterate							(void);

private:
	const deUint32					m_numValues;
	const bool						m_sized;
	const tcu::UVec3				m_localSize;
	const tcu::UVec3				m_workSize;
};

WriteToMultipleSSBOTest::WriteToMultipleSSBOTest (tcu::TestContext&		testCtx,
												  const std::string&	name,
												  const std::string&	description,
												  const deUint32		numValues,
												  const bool			sized,
												  const tcu::UVec3&		localSize,
												  const tcu::UVec3&		workSize)
	: TestCase		(testCtx, name, description)
	, m_numValues	(numValues)
	, m_sized		(sized)
	, m_localSize	(localSize)
	, m_workSize	(workSize)
{
	DE_ASSERT(m_numValues % (multiplyComponents(m_workSize) * multiplyComponents(m_localSize)) == 0);
}

void WriteToMultipleSSBOTest::initPrograms (SourceCollections& sourceCollections) const
{
	std::ostringstream src;
	src << "#version 310 es\n"
		<< "layout (local_size_x = " << m_localSize.x() << ", local_size_y = " << m_localSize.y() << ", local_size_z = " << m_localSize.z() << ") in;\n"
		<< "layout(binding = 0) writeonly buffer Out0 {\n"
		<< "    uint values[" << (m_sized ? de::toString(m_numValues) : "") << "];\n"
		<< "} sb_out0;\n"
		<< "layout(binding = 1) writeonly buffer Out1 {\n"
		<< "    uint values[" << (m_sized ? de::toString(m_numValues) : "") << "];\n"
		<< "} sb_out1;\n"
		<< "void main (void) {\n"
		<< "    uvec3 size      = gl_NumWorkGroups * gl_WorkGroupSize;\n"
		<< "    uint groupNdx   = size.x*size.y*gl_GlobalInvocationID.z + size.x*gl_GlobalInvocationID.y + gl_GlobalInvocationID.x;\n"
		<< "\n"
		<< "    {\n"
		<< "        uint numValuesPerInv = uint(sb_out0.values.length()) / (size.x*size.y*size.z);\n"
		<< "        uint offset          = numValuesPerInv*groupNdx;\n"
		<< "\n"
		<< "        for (uint ndx = 0u; ndx < numValuesPerInv; ndx++)\n"
		<< "            sb_out0.values[offset + ndx] = offset + ndx;\n"
		<< "    }\n"
		<< "    {\n"
		<< "        uint numValuesPerInv = uint(sb_out1.values.length()) / (size.x*size.y*size.z);\n"
		<< "        uint offset          = numValuesPerInv*groupNdx;\n"
		<< "\n"
		<< "        for (uint ndx = 0u; ndx < numValuesPerInv; ndx++)\n"
		<< "            sb_out1.values[offset + ndx] = uint(sb_out1.values.length()) - offset - ndx;\n"
		<< "    }\n"
		<< "}\n";

	sourceCollections.glslSources.add("comp") << glu::ComputeSource(src.str());
}

TestInstance* WriteToMultipleSSBOTest::createInstance (Context& context) const
{
	return new WriteToMultipleSSBOTestInstance(context, m_numValues, m_sized, m_localSize, m_workSize);
}

WriteToMultipleSSBOTestInstance::WriteToMultipleSSBOTestInstance (Context&			context,
																  const deUint32	numValues,
																  const bool		sized,
																  const tcu::UVec3&	localSize,
																  const tcu::UVec3&	workSize)
	: TestInstance	(context)
	, m_numValues	(numValues)
	, m_sized		(sized)
	, m_localSize	(localSize)
	, m_workSize	(workSize)
{
}

tcu::TestStatus WriteToMultipleSSBOTestInstance::iterate (void)
{
	const VkDevice			device				= m_context.getDevice();
	const DeviceInterface&	vk					= m_context.getDeviceInterface();
	const VkQueue			queue				= m_context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= m_context.getDefaultAllocator();

	const Unique<VkShaderModule>	shaderModule(createShaderModule(vk, device, m_context.getBinaryCollection().get("comp"), 0));
	const Unique<VkShader>			shader(makeComputeShader(vk, device, *shaderModule));

	// Create two output buffers

	const VkDeviceSize bufferSizeBytes = sizeof(deUint32) * m_numValues;
	const Buffer buffer0(vk, device, allocator, bufferSizeBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryRequirement::HostVisible);
	const Buffer buffer1(vk, device, allocator, bufferSizeBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryRequirement::HostVisible);

	// Create descriptor set

	const Unique<VkDescriptorSetLayout> descriptorSetLayout(
		DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(vk, device));

	const Unique<VkDescriptorPool> descriptorPool(
		DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2u)
		.build(vk, device, VK_DESCRIPTOR_POOL_USAGE_DYNAMIC, 1u));

	const Unique<VkDescriptorSet> descriptorSet = allocDescriptorSet(vk, device, *descriptorPool, VK_DESCRIPTOR_SET_USAGE_ONE_SHOT, *descriptorSetLayout);

	const VkDescriptorInfo buffer0DescriptorInfo = makeDescriptorInfoForBuffer(*buffer0, 0ull, bufferSizeBytes);
	const VkDescriptorInfo buffer1DescriptorInfo = makeDescriptorInfoForBuffer(*buffer1, 0ull, bufferSizeBytes);
	DescriptorSetUpdateBuilder()
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &buffer0DescriptorInfo)
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &buffer1DescriptorInfo)
		.update(vk, device);

	// Perform the computation

	const Unique<VkPipelineLayout> pipelineLayout(makePipelineLayout(vk, device, &descriptorSetLayout.get()));
	const Unique<VkPipeline> pipeline(makeComputePipeline(vk, device, *pipelineLayout, *shader));

	const VkBufferMemoryBarrier shaderWriteBarrier0 = makeBufferMemoryBarrier(VK_MEMORY_OUTPUT_SHADER_WRITE_BIT, VK_MEMORY_INPUT_HOST_READ_BIT, *buffer0, 0ull, bufferSizeBytes);
	const VkBufferMemoryBarrier shaderWriteBarrier1 = makeBufferMemoryBarrier(VK_MEMORY_OUTPUT_SHADER_WRITE_BIT, VK_MEMORY_INPUT_HOST_READ_BIT, *buffer1, 0ull, bufferSizeBytes);
	const void* postComputeBarriers[] = { &shaderWriteBarrier0, &shaderWriteBarrier1 };

	const Unique<VkCmdPool> cmdPool(makeCommandPool(vk, device, queueFamilyIndex));
	const Unique<VkCmdBuffer> cmdBuffer(makeCommandBuffer(vk, device, *cmdPool));

	// Start recording commands

	beginCommandBuffer(vk, *cmdBuffer);

	vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);
	vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);

	vk.cmdDispatch(*cmdBuffer, m_workSize.x(), m_workSize.y(), m_workSize.z());
	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, DE_FALSE, DE_LENGTH_OF_ARRAY(postComputeBarriers), postComputeBarriers);

	endCommandBuffer(vk, *cmdBuffer);

	// Wait for completion

	submitCommandsAndWait(vk, device, queue, *cmdBuffer);

	// Validate the results
	{
		const Allocation& buffer0Allocation = buffer0.getAllocation();
		invalidateMappedMemoryRange(vk, device, buffer0Allocation.getMemory(), buffer0Allocation.getOffset(), bufferSizeBytes);
		const deUint32* buffer0Ptr = static_cast<deUint32*>(buffer0Allocation.getHostPtr());

		for (deUint32 ndx = 0; ndx < m_numValues; ++ndx)
		{
			const deUint32 res = buffer0Ptr[ndx];
			const deUint32 ref = ndx;

			if (res != ref)
			{
				std::ostringstream msg;
				msg << "Comparison failed for Out0.values[" << ndx << "] res=" << res << " ref=" << ref;
				return tcu::TestStatus::fail(msg.str());
			}
		}
	}
	{
		const Allocation& buffer1Allocation = buffer1.getAllocation();
		invalidateMappedMemoryRange(vk, device, buffer1Allocation.getMemory(), buffer1Allocation.getOffset(), bufferSizeBytes);
		const deUint32* buffer1Ptr = static_cast<deUint32*>(buffer1Allocation.getHostPtr());

		for (deUint32 ndx = 0; ndx < m_numValues; ++ndx)
		{
			const deUint32 res = buffer1Ptr[ndx];
			const deUint32 ref = m_numValues - ndx;

			if (res != ref)
			{
				std::ostringstream msg;
				msg << "Comparison failed for Out1.values[" << ndx << "] res=" << res << " ref=" << ref;
				return tcu::TestStatus::fail(msg.str());
			}
		}
	}
	return tcu::TestStatus::pass("Compute succeeded");
}

class SSBOBarrierTest : public vkt::TestCase
{
public:
						SSBOBarrierTest		(tcu::TestContext&	testCtx,
											 const std::string&	name,
											 const std::string&	description,
											 const tcu::UVec3&	workSize);

	void				initPrograms		(SourceCollections& sourceCollections) const;
	TestInstance*		createInstance		(Context& context) const;

private:
	const tcu::UVec3	m_workSize;
};

class SSBOBarrierTestInstance : public vkt::TestInstance
{
public:
									SSBOBarrierTestInstance		(Context&			context,
																 const tcu::UVec3&	workSize);

	tcu::TestStatus					iterate						(void);

private:
	const tcu::UVec3				m_workSize;
};

SSBOBarrierTest::SSBOBarrierTest (tcu::TestContext&		testCtx,
								  const std::string&	name,
								  const std::string&	description,
								  const tcu::UVec3&		workSize)
	: TestCase		(testCtx, name, description)
	, m_workSize	(workSize)
{
}

void SSBOBarrierTest::initPrograms (SourceCollections& sourceCollections) const
{
	sourceCollections.glslSources.add("comp0") << glu::ComputeSource(
		"#version 310 es\n"
		"layout (local_size_x = 1) in;\n"
		"layout(binding = 2) readonly uniform Constants {\n"
		"    uint u_baseVal;\n"
		"};\n"
		"layout(binding = 1) writeonly buffer Output {\n"
		"    uint values[];\n"
		"};\n"
		"void main (void) {\n"
		"    uint offset = gl_NumWorkGroups.x*gl_NumWorkGroups.y*gl_WorkGroupID.z + gl_NumWorkGroups.x*gl_WorkGroupID.y + gl_WorkGroupID.x;\n"
		"    values[offset] = u_baseVal + offset;\n"
		"}\n");

	sourceCollections.glslSources.add("comp1") << glu::ComputeSource(
		"#version 310 es\n"
		"layout (local_size_x = 1) in;\n"
		"layout(binding = 1) readonly buffer Input {\n"
		"    uint values[];\n"
		"};\n"
		"layout(binding = 0) coherent buffer Output {\n"
		"    uint sum;\n"
		"};\n"
		"void main (void) {\n"
		"    uint offset = gl_NumWorkGroups.x*gl_NumWorkGroups.y*gl_WorkGroupID.z + gl_NumWorkGroups.x*gl_WorkGroupID.y + gl_WorkGroupID.x;\n"
		"    uint value  = values[offset];\n"
		"    atomicAdd(sum, value);\n"
		"}\n");
}

TestInstance* SSBOBarrierTest::createInstance (Context& context) const
{
	return new SSBOBarrierTestInstance(context, m_workSize);
}

SSBOBarrierTestInstance::SSBOBarrierTestInstance (Context& context, const tcu::UVec3& workSize)
	: TestInstance	(context)
	, m_workSize	(workSize)
{
}

tcu::TestStatus SSBOBarrierTestInstance::iterate (void)
{
	const VkDevice			device				= m_context.getDevice();
	const DeviceInterface&	vk					= m_context.getDeviceInterface();
	const VkQueue			queue				= m_context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= m_context.getDefaultAllocator();

	const Unique<VkShaderModule>	shaderModule0(createShaderModule(vk, device, m_context.getBinaryCollection().get("comp0"), 0));
	const Unique<VkShaderModule>	shaderModule1(createShaderModule(vk, device, m_context.getBinaryCollection().get("comp1"), 0));
	const Unique<VkShader>			shader0(makeComputeShader(vk, device, *shaderModule0));
	const Unique<VkShader>			shader1(makeComputeShader(vk, device, *shaderModule1));

	// Create a work buffer used by both shaders

	const int workGroupCount = multiplyComponents(m_workSize);
	const VkDeviceSize workBufferSizeBytes = sizeof(deUint32) * workGroupCount;
	const Buffer workBuffer(vk, device, allocator, workBufferSizeBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryRequirement::Any);

	// Create an output buffer

	const VkDeviceSize outputBufferSizeBytes = sizeof(deUint32);
	const Buffer outputBuffer(vk, device, allocator, outputBufferSizeBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryRequirement::HostVisible);

	// Create a uniform buffer (to pass uniform constants)

	const VkDeviceSize uniformBufferSizeBytes = sizeof(deUint32);
	const Buffer uniformBuffer(vk, device, allocator, uniformBufferSizeBytes, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, MemoryRequirement::HostVisible);

	// Set the constants in the uniform buffer

	const deUint32	baseValue = 127;
	{
		const Allocation& uniformBufferAllocation = uniformBuffer.getAllocation();
		deUint32* uniformBufferPtr = static_cast<deUint32*>(uniformBufferAllocation.getHostPtr());
		uniformBufferPtr[0] = baseValue;

		flushMappedMemoryRange(vk, device, uniformBufferAllocation.getMemory(), uniformBufferAllocation.getOffset(), uniformBufferSizeBytes);
	}

	// Create descriptor set

	const Unique<VkDescriptorSetLayout> descriptorSetLayout(
		DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.addSingleBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(vk, device));

	const Unique<VkDescriptorPool> descriptorPool(
		DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2u)
		.addType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
		.build(vk, device, VK_DESCRIPTOR_POOL_USAGE_DYNAMIC, 1u));

	const Unique<VkDescriptorSet> descriptorSet = allocDescriptorSet(vk, device, *descriptorPool, VK_DESCRIPTOR_SET_USAGE_ONE_SHOT, *descriptorSetLayout);

	const VkDescriptorInfo workBufferDescriptorInfo = makeDescriptorInfoForBuffer(*workBuffer, 0ull, workBufferSizeBytes);
	const VkDescriptorInfo outputBufferDescriptorInfo = makeDescriptorInfoForBuffer(*outputBuffer, 0ull, outputBufferSizeBytes);
	const VkDescriptorInfo uniformBufferDescriptorInfo = makeDescriptorInfoForBuffer(*uniformBuffer, 0ull, uniformBufferSizeBytes);
	DescriptorSetUpdateBuilder()
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &outputBufferDescriptorInfo)
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &workBufferDescriptorInfo)
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(2u), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &uniformBufferDescriptorInfo)
		.update(vk, device);

	// Perform the computation

	const Unique<VkPipelineLayout> pipelineLayout(makePipelineLayout(vk, device, &descriptorSetLayout.get()));
	const Unique<VkPipeline> pipeline0(makeComputePipeline(vk, device, *pipelineLayout, *shader0));
	const Unique<VkPipeline> pipeline1(makeComputePipeline(vk, device, *pipelineLayout, *shader1));

	const VkBufferMemoryBarrier writeUniformConstantsBarrier = makeBufferMemoryBarrier(VK_MEMORY_OUTPUT_HOST_WRITE_BIT, VK_MEMORY_INPUT_UNIFORM_READ_BIT, *uniformBuffer, 0ull, uniformBufferSizeBytes);
	const void* barriersBeforeCompute[] = { &writeUniformConstantsBarrier };

	const VkBufferMemoryBarrier betweenShadersBarrier = makeBufferMemoryBarrier(VK_MEMORY_OUTPUT_SHADER_WRITE_BIT, VK_MEMORY_INPUT_SHADER_READ_BIT, *workBuffer, 0ull, workBufferSizeBytes);
	const void* barriersAfterFirstShader[] = { &betweenShadersBarrier };

	const VkBufferMemoryBarrier afterComputeBarrier = makeBufferMemoryBarrier(VK_MEMORY_OUTPUT_SHADER_WRITE_BIT, VK_MEMORY_INPUT_HOST_READ_BIT, *outputBuffer, 0ull, outputBufferSizeBytes);
	const void* barriersAfterCompute[] = { &afterComputeBarrier };

	const Unique<VkCmdPool> cmdPool(makeCommandPool(vk, device, queueFamilyIndex));
	const Unique<VkCmdBuffer> cmdBuffer(makeCommandBuffer(vk, device, *cmdPool));

	// Start recording commands

	beginCommandBuffer(vk, *cmdBuffer);

	vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline0);
	vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);

	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, DE_FALSE, DE_LENGTH_OF_ARRAY(barriersBeforeCompute), barriersBeforeCompute);

	vk.cmdDispatch(*cmdBuffer, m_workSize.x(), m_workSize.y(), m_workSize.z());
	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, DE_FALSE, DE_LENGTH_OF_ARRAY(barriersAfterFirstShader), barriersAfterFirstShader);

	// Switch to the second shader program
	vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline1);

	vk.cmdDispatch(*cmdBuffer, m_workSize.x(), m_workSize.y(), m_workSize.z());
	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, DE_FALSE, DE_LENGTH_OF_ARRAY(barriersAfterCompute), barriersAfterCompute);

	endCommandBuffer(vk, *cmdBuffer);

	// Wait for completion

	submitCommandsAndWait(vk, device, queue, *cmdBuffer);

	// Validate the results

	const Allocation& outputBufferAllocation = outputBuffer.getAllocation();
	invalidateMappedMemoryRange(vk, device, outputBufferAllocation.getMemory(), outputBufferAllocation.getOffset(), outputBufferSizeBytes);

	const deUint32* bufferPtr = static_cast<deUint32*>(outputBufferAllocation.getHostPtr());
	const deUint32	res = *bufferPtr;
	deUint32		ref = 0;

	for (int ndx = 0; ndx < workGroupCount; ++ndx)
		ref += baseValue + ndx;

	if (res != ref)
	{
		std::ostringstream msg;
		msg << "ERROR: comparison failed, expected " << ref << ", got " << res;
		return tcu::TestStatus::fail(msg.str());
	}
	return tcu::TestStatus::pass("Compute succeeded");
}

class ImageAtomicOpTest : public vkt::TestCase
{
public:
						ImageAtomicOpTest		(tcu::TestContext&	testCtx,
												 const std::string& name,
												 const std::string& description,
												 const deUint32		localSize,
												 const tcu::UVec2&	imageSize);

	void				initPrograms			(SourceCollections& sourceCollections) const;
	TestInstance*		createInstance			(Context& context) const;

private:
	const deUint32		m_localSize;
	const tcu::UVec2	m_imageSize;
};

class ImageAtomicOpTestInstance : public vkt::TestInstance
{
public:
									ImageAtomicOpTestInstance		(Context& context,
																	 const deUint32 localSize,
																	 const tcu::UVec2& imageSize);

	tcu::TestStatus					iterate							(void);

private:
	const deUint32					m_localSize;
	const tcu::UVec2				m_imageSize;
};

ImageAtomicOpTest::ImageAtomicOpTest (tcu::TestContext&		testCtx,
									  const std::string&	name,
									  const std::string&	description,
									  const deUint32		localSize,
									  const tcu::UVec2&		imageSize)
	: TestCase		(testCtx, name, description)
	, m_localSize	(localSize)
	, m_imageSize	(imageSize)
{
}

void ImageAtomicOpTest::initPrograms (SourceCollections& sourceCollections) const
{
	std::ostringstream src;
	src << "#version 310 es\n"
		<< "#extension GL_OES_shader_image_atomic : require\n"
		<< "layout (local_size_x = " << m_localSize << ") in;\n"
		<< "layout(binding = 1, r32ui) coherent uniform highp uimage2D u_dstImg;\n"
		<< "layout(binding = 0) readonly buffer Input {\n"
		<< "    uint values[" << (multiplyComponents(m_imageSize) * m_localSize) << "];\n"
		<< "} sb_in;\n\n"
		<< "void main (void) {\n"
		<< "    uint stride = gl_NumWorkGroups.x*gl_WorkGroupSize.x;\n"
		<< "    uint value  = sb_in.values[gl_GlobalInvocationID.y*stride + gl_GlobalInvocationID.x];\n"
		<< "\n"
		<< "    if (gl_LocalInvocationIndex == 0u)\n"
		<< "        imageStore(u_dstImg, ivec2(gl_WorkGroupID.xy), uvec4(0));\n"
		<< "    memoryBarrierImage();\n"
		<< "    barrier();\n"
		<< "    imageAtomicAdd(u_dstImg, ivec2(gl_WorkGroupID.xy), value);\n"
		<< "}\n";

	sourceCollections.glslSources.add("comp") << glu::ComputeSource(src.str());
}

TestInstance* ImageAtomicOpTest::createInstance (Context& context) const
{
	return new ImageAtomicOpTestInstance(context, m_localSize, m_imageSize);
}

ImageAtomicOpTestInstance::ImageAtomicOpTestInstance (Context& context, const deUint32 localSize, const tcu::UVec2& imageSize)
	: TestInstance	(context)
	, m_localSize	(localSize)
	, m_imageSize	(imageSize)
{
}

tcu::TestStatus ImageAtomicOpTestInstance::iterate (void)
{
	const VkDevice			device				= m_context.getDevice();
	const DeviceInterface&	vk					= m_context.getDeviceInterface();
	const VkQueue			queue				= m_context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= m_context.getDefaultAllocator();

	const Unique<VkShaderModule>	shaderModule(createShaderModule(vk, device, m_context.getBinaryCollection().get("comp"), 0));
	const Unique<VkShader>			shader(makeComputeShader(vk, device, *shaderModule));

	// Create an image

	const VkImageCreateInfo imageParams =
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,									//	VkStructureType		sType;
		DE_NULL,																//	const void*			pNext;
		VK_IMAGE_TYPE_2D,														//	VkImageType			imageType;
		VK_FORMAT_R32_UINT,														//	VkFormat			format;
		{ m_imageSize.x(), m_imageSize.y(), 1 },								//	VkExtent3D			extent;
		1u,																		//	deUint32			mipLevels;
		1u,																		//	deUint32			arraySize;
		1u,																		//	deUint32			samples;
		VK_IMAGE_TILING_OPTIMAL,												//	VkImageTiling		tiling;
		VK_IMAGE_USAGE_TRANSFER_SOURCE_BIT | VK_IMAGE_USAGE_STORAGE_BIT,		//	VkImageUsageFlags	usage;
		0u,																		//	VkImageCreateFlags	flags;
		VK_SHARING_MODE_EXCLUSIVE,												//	VkSharingMode		sharingMode;
		1u,																		//	deUint32			queueFamilyCount;
		&queueFamilyIndex,													//	const deUint32*		pQueueFamilyIndices;
		VK_IMAGE_LAYOUT_UNDEFINED,												//	VkImageLayout		initialLayout;
	};

	const Image image(vk, device, allocator, imageParams, MemoryRequirement::Any);

	// Input buffer

	const deUint32 numInputValues = multiplyComponents(m_imageSize) * m_localSize;
	const VkDeviceSize inputBufferSizeBytes = sizeof(deUint32) * numInputValues;

	const Buffer inputBuffer(vk, device, allocator, inputBufferSizeBytes, VK_BUFFER_USAGE_TRANSFER_SOURCE_BIT, MemoryRequirement::HostVisible);

	// Populate the input buffer with test data
	{
		de::Random rnd(0x77238ac2);
		const Allocation& inputBufferAllocation = inputBuffer.getAllocation();
		deUint32* bufferPtr = static_cast<deUint32*>(inputBufferAllocation.getHostPtr());
		for (deUint32 i = 0; i < numInputValues; ++i)
			*bufferPtr++ = rnd.getUint32();

		flushMappedMemoryRange(vk, device, inputBufferAllocation.getMemory(), inputBufferAllocation.getOffset(), inputBufferSizeBytes);
	}

	// Create a buffer to store shader output (copied from image data)

	const deUint32 imageArea = multiplyComponents(m_imageSize);
	const VkDeviceSize outputBufferSizeBytes = sizeof(deUint32) * imageArea;
	const Buffer outputBuffer(vk, device, allocator, outputBufferSizeBytes, VK_BUFFER_USAGE_TRANSFER_DESTINATION_BIT, MemoryRequirement::HostVisible);

	// Create descriptor set

	const Unique<VkDescriptorSetLayout> descriptorSetLayout(
		DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(vk, device));

	const Unique<VkDescriptorPool> descriptorPool(
		DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		.build(vk, device, VK_DESCRIPTOR_POOL_USAGE_DYNAMIC, 1u));

	const Unique<VkDescriptorSet> descriptorSet = allocDescriptorSet(vk, device, *descriptorPool, VK_DESCRIPTOR_SET_USAGE_ONE_SHOT, *descriptorSetLayout);

	// Set the bindings

	const VkImageSubresourceRange subresourceRange = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u);
	const VkImageViewCreateInfo imageViewParams =
	{
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,		// VkStructureType				sType;
		DE_NULL,										// const void*					pNext;
		*image,											// VkImage						image;
		VK_IMAGE_VIEW_TYPE_2D,							// VkImageViewType				viewType;
		VK_FORMAT_R32_UINT,								// VkFormat						format;
		makeChannelMappingRGBA(),						// VkChannelMapping				channels;
		subresourceRange,								// VkImageSubresourceRange		subresourceRange;
		0u,												// VkImageViewCreateFlags		flags;
	};
	const Unique<VkImageView> imageView(createImageView(vk, device, &imageViewParams));

	const VkDescriptorInfo imageDescriptorInfo = makeDescriptorInfoForImageView(*imageView, VK_IMAGE_LAYOUT_GENERAL);
	const VkDescriptorInfo bufferDescriptorInfo = makeDescriptorInfoForBuffer(*inputBuffer, 0ull, inputBufferSizeBytes);

	DescriptorSetUpdateBuilder()
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &bufferDescriptorInfo)
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1u), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &imageDescriptorInfo)
		.update(vk, device);

	// Perform the computation
	{
		const Unique<VkPipelineLayout> pipelineLayout(makePipelineLayout(vk, device, &descriptorSetLayout.get()));
		const Unique<VkPipeline> pipeline(makeComputePipeline(vk, device, *pipelineLayout, *shader));

		const VkBufferMemoryBarrier inputBufferPostHostWriteBarrier = makeBufferMemoryBarrier(VK_MEMORY_OUTPUT_HOST_WRITE_BIT, VK_MEMORY_INPUT_SHADER_READ_BIT, *inputBuffer, 0ull, inputBufferSizeBytes);

		const VkImageMemoryBarrier imagePreCopyBarrier =
		{
			VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,			// VkStructureType			sType;
			DE_NULL,										// const void*				pNext;
			VK_MEMORY_OUTPUT_SHADER_WRITE_BIT,				// VkMemoryOutputFlags		outputMask;
			VK_MEMORY_INPUT_TRANSFER_BIT,					// VkMemoryInputFlags		inputMask;
			VK_IMAGE_LAYOUT_GENERAL,						// VkImageLayout			oldLayout;
			VK_IMAGE_LAYOUT_TRANSFER_SOURCE_OPTIMAL,		// VkImageLayout			newLayout;
			VK_QUEUE_FAMILY_IGNORED,						// deUint32					srcQueueFamilyIndex;
			VK_QUEUE_FAMILY_IGNORED,						// deUint32					destQueueFamilyIndex;
			*image,											// VkImage					image;
			subresourceRange,								// VkImageSubresourceRange	subresourceRange;
		};

		const VkBufferMemoryBarrier outputBufferPostCopyBarrier = makeBufferMemoryBarrier(VK_MEMORY_OUTPUT_TRANSFER_BIT, VK_MEMORY_INPUT_HOST_READ_BIT, *outputBuffer, 0ull, outputBufferSizeBytes);

		const void* preComputeBarriers[] = { &inputBufferPostHostWriteBarrier };
		const void* preCopyBarriers[] = { &imagePreCopyBarrier };
		const void* postCopyBarriers[] = { &outputBufferPostCopyBarrier };

		const VkBufferImageCopy copyParams = makeBufferImageCopy(m_imageSize);

		// Prepare the command buffer

		const Unique<VkCmdPool> cmdPool(makeCommandPool(vk, device, queueFamilyIndex));
		const Unique<VkCmdBuffer> cmdBuffer(makeCommandBuffer(vk, device, *cmdPool));

		// Start recording commands

		beginCommandBuffer(vk, *cmdBuffer);

		vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);
		vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);

		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, DE_FALSE, DE_LENGTH_OF_ARRAY(preComputeBarriers), preComputeBarriers);
		vk.cmdDispatch(*cmdBuffer, m_imageSize.x(), m_imageSize.y(), 1u);

		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, DE_FALSE, DE_LENGTH_OF_ARRAY(preCopyBarriers), preCopyBarriers);
		vk.cmdCopyImageToBuffer(*cmdBuffer, *image, VK_IMAGE_LAYOUT_TRANSFER_SOURCE_OPTIMAL, *outputBuffer, 1u, &copyParams);
		vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_HOST_BIT, DE_FALSE, DE_LENGTH_OF_ARRAY(postCopyBarriers), postCopyBarriers);

		endCommandBuffer(vk, *cmdBuffer);

		// Wait for completion

		submitCommandsAndWait(vk, device, queue, *cmdBuffer);
	}

	// Validate the results

	const Allocation& outputBufferAllocation = outputBuffer.getAllocation();
	invalidateMappedMemoryRange(vk, device, outputBufferAllocation.getMemory(), outputBufferAllocation.getOffset(), outputBufferSizeBytes);

	const deUint32* bufferPtr = static_cast<deUint32*>(outputBufferAllocation.getHostPtr());
	const deUint32* refBufferPtr = static_cast<deUint32*>(inputBuffer.getAllocation().getHostPtr());

	for (deUint32 pixelNdx = 0; pixelNdx < imageArea; ++pixelNdx)
	{
		const deUint32	res = bufferPtr[pixelNdx];
		deUint32		ref = 0;

		for (deUint32 offs = 0; offs < m_localSize; ++offs)
			ref += refBufferPtr[pixelNdx * m_localSize + offs];

		if (res != ref)
		{
			std::ostringstream msg;
			msg << "Comparison failed for pixel " << pixelNdx;
			return tcu::TestStatus::fail(msg.str());
		}
	}
	return tcu::TestStatus::pass("Compute succeeded");
}

class ImageBarrierTest : public vkt::TestCase
{
public:
						ImageBarrierTest	(tcu::TestContext&	testCtx,
											const std::string&	name,
											const std::string&	description,
											const tcu::UVec2&	imageSize);

	void				initPrograms		(SourceCollections& sourceCollections) const;
	TestInstance*		createInstance		(Context& context) const;

private:
	const tcu::UVec2	m_imageSize;
};

class ImageBarrierTestInstance : public vkt::TestInstance
{
public:
									ImageBarrierTestInstance	(Context&			context,
																 const tcu::UVec2&	imageSize);

	tcu::TestStatus					iterate						(void);

private:
	const tcu::UVec2				m_imageSize;
};

ImageBarrierTest::ImageBarrierTest (tcu::TestContext&	testCtx,
									const std::string&	name,
									const std::string&	description,
									const tcu::UVec2&	imageSize)
	: TestCase		(testCtx, name, description)
	, m_imageSize	(imageSize)
{
}

void ImageBarrierTest::initPrograms (SourceCollections& sourceCollections) const
{
	sourceCollections.glslSources.add("comp0") << glu::ComputeSource(
		"#version 310 es\n"
		"layout (local_size_x = 1) in;\n"
		"layout(binding = 2) readonly uniform Constants {\n"
		"    uint u_baseVal;\n"
		"};\n"
		"layout(binding = 1, r32ui) writeonly uniform highp uimage2D u_img;\n"
		"void main (void) {\n"
		"    uint offset = gl_NumWorkGroups.x*gl_NumWorkGroups.y*gl_WorkGroupID.z + gl_NumWorkGroups.x*gl_WorkGroupID.y + gl_WorkGroupID.x;\n"
		"    imageStore(u_img, ivec2(gl_WorkGroupID.xy), uvec4(offset + u_baseVal, 0, 0, 0));\n"
		"}\n");

	sourceCollections.glslSources.add("comp1") << glu::ComputeSource(
		"#version 310 es\n"
		"layout (local_size_x = 1) in;\n"
		"layout(binding = 1, r32ui) readonly uniform highp uimage2D u_img;\n"
		"layout(binding = 0) coherent buffer Output {\n"
		"    uint sum;\n"
		"};\n"
		"void main (void) {\n"
		"    uint value = imageLoad(u_img, ivec2(gl_WorkGroupID.xy)).x;\n"
		"    atomicAdd(sum, value);\n"
		"}\n");
}

TestInstance* ImageBarrierTest::createInstance (Context& context) const
{
	return new ImageBarrierTestInstance(context, m_imageSize);
}

ImageBarrierTestInstance::ImageBarrierTestInstance (Context& context, const tcu::UVec2& imageSize)
	: TestInstance	(context)
	, m_imageSize	(imageSize)
{
}

tcu::TestStatus ImageBarrierTestInstance::iterate (void)
{
	const VkDevice			device				= m_context.getDevice();
	const DeviceInterface&	vk					= m_context.getDeviceInterface();
	const VkQueue			queue				= m_context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= m_context.getUniversalQueueFamilyIndex();
	Allocator&				allocator			= m_context.getDefaultAllocator();

	const Unique<VkShaderModule>	shaderModule0(createShaderModule(vk, device, m_context.getBinaryCollection().get("comp0"), 0));
	const Unique<VkShaderModule>	shaderModule1(createShaderModule(vk, device, m_context.getBinaryCollection().get("comp1"), 0));
	const Unique<VkShader>			shader0(makeComputeShader(vk, device, *shaderModule0));
	const Unique<VkShader>			shader1(makeComputeShader(vk, device, *shaderModule1));

	// Create an image used by both shaders

	const VkImageCreateInfo imageParams =
	{
		VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,		//	VkStructureType		sType;
		DE_NULL,									//	const void*			pNext;
		VK_IMAGE_TYPE_2D,							//	VkImageType			imageType;
		VK_FORMAT_R32_UINT,							//	VkFormat			format;
		{ m_imageSize.x(), m_imageSize.y(), 1 },	//	VkExtent3D			extent;
		1u,											//	deUint32			mipLevels;
		1u,											//	deUint32			arraySize;
		1u,											//	deUint32			samples;
		VK_IMAGE_TILING_OPTIMAL,					//	VkImageTiling		tiling;
		VK_IMAGE_USAGE_STORAGE_BIT,					//	VkImageUsageFlags	usage;
		0u,											//	VkImageCreateFlags	flags;
		VK_SHARING_MODE_EXCLUSIVE,					//	VkSharingMode		sharingMode;
		1u,											//	deUint32			queueFamilyCount;
		&queueFamilyIndex,						//	const deUint32*		pQueueFamilyIndices;
		VK_IMAGE_LAYOUT_UNDEFINED,					//	VkImageLayout		initialLayout;
	};

	const Image image(vk, device, allocator, imageParams, MemoryRequirement::Any);

	// Create an output buffer

	const VkDeviceSize outputBufferSizeBytes = sizeof(deUint32);
	const Buffer outputBuffer(vk, device, allocator, outputBufferSizeBytes, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, MemoryRequirement::HostVisible);

	// Create a uniform buffer (to pass uniform constants)

	const VkDeviceSize uniformBufferSizeBytes = sizeof(deUint32);
	const Buffer uniformBuffer(vk, device, allocator, uniformBufferSizeBytes, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, MemoryRequirement::HostVisible);

	// Set the constants in the uniform buffer

	const deUint32	baseValue = 127;
	{
		const Allocation& uniformBufferAllocation = uniformBuffer.getAllocation();
		deUint32* uniformBufferPtr = static_cast<deUint32*>(uniformBufferAllocation.getHostPtr());
		uniformBufferPtr[0] = baseValue;

		flushMappedMemoryRange(vk, device, uniformBufferAllocation.getMemory(), uniformBufferAllocation.getOffset(), uniformBufferSizeBytes);
	}

	// Create descriptor set

	const Unique<VkDescriptorSetLayout> descriptorSetLayout(
		DescriptorSetLayoutBuilder()
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.addSingleBinding(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_SHADER_STAGE_COMPUTE_BIT)
		.addSingleBinding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_COMPUTE_BIT)
		.build(vk, device));

	const Unique<VkDescriptorPool> descriptorPool(
		DescriptorPoolBuilder()
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		.addType(VK_DESCRIPTOR_TYPE_STORAGE_IMAGE)
		.addType(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
		.build(vk, device, VK_DESCRIPTOR_POOL_USAGE_DYNAMIC, 1u));

	const Unique<VkDescriptorSet> descriptorSet = allocDescriptorSet(vk, device, *descriptorPool, VK_DESCRIPTOR_SET_USAGE_ONE_SHOT, *descriptorSetLayout);

	const VkImageSubresourceRange subresourceRange = makeImageSubresourceRange(VK_IMAGE_ASPECT_COLOR_BIT, 0u, 1u, 0u, 1u);
	const VkImageViewCreateInfo imageViewParams =
	{
		VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,		// VkStructureType				sType;
		DE_NULL,										// const void*					pNext;
		*image,											// VkImage						image;
		VK_IMAGE_VIEW_TYPE_2D,							// VkImageViewType				viewType;
		VK_FORMAT_R32_UINT,								// VkFormat						format;
		makeChannelMappingRGBA(),						// VkChannelMapping				channels;
		subresourceRange,								// VkImageSubresourceRange		subresourceRange;
		0u,												// VkImageViewCreateFlags		flags;
	};
	const Unique<VkImageView> imageView(createImageView(vk, device, &imageViewParams));

	const VkDescriptorInfo imageDescriptorInfo = makeDescriptorInfoForImageView(*imageView, VK_IMAGE_LAYOUT_GENERAL);
	const VkDescriptorInfo outputBufferDescriptorInfo = makeDescriptorInfoForBuffer(*outputBuffer, 0ull, outputBufferSizeBytes);
	const VkDescriptorInfo uniformBufferDescriptorInfo = makeDescriptorInfoForBuffer(*uniformBuffer, 0ull, uniformBufferSizeBytes);
	DescriptorSetUpdateBuilder()
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(0u), VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, &outputBufferDescriptorInfo)
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(1u), VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, &imageDescriptorInfo)
		.writeSingle(*descriptorSet, DescriptorSetUpdateBuilder::Location::binding(2u), VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, &uniformBufferDescriptorInfo)
		.update(vk, device);

	// Perform the computation

	const Unique<VkPipelineLayout> pipelineLayout(makePipelineLayout(vk, device, &descriptorSetLayout.get()));
	const Unique<VkPipeline> pipeline0(makeComputePipeline(vk, device, *pipelineLayout, *shader0));
	const Unique<VkPipeline> pipeline1(makeComputePipeline(vk, device, *pipelineLayout, *shader1));

	const VkBufferMemoryBarrier writeUniformConstantsBarrier = makeBufferMemoryBarrier(VK_MEMORY_OUTPUT_HOST_WRITE_BIT, VK_MEMORY_INPUT_UNIFORM_READ_BIT, *uniformBuffer, 0ull, uniformBufferSizeBytes);

	const VkImageMemoryBarrier imageLayoutBarrier =
	{
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,			// VkStructureType			sType;
		DE_NULL,										// const void*				pNext;
		0u,												// VkMemoryOutputFlags		outputMask;
		0u,												// VkMemoryInputFlags		inputMask;
		VK_IMAGE_LAYOUT_UNDEFINED,						// VkImageLayout			oldLayout;
		VK_IMAGE_LAYOUT_GENERAL,						// VkImageLayout			newLayout;
		VK_QUEUE_FAMILY_IGNORED,						// deUint32					srcQueueFamilyIndex;
		VK_QUEUE_FAMILY_IGNORED,						// deUint32					destQueueFamilyIndex;
		*image,											// VkImage					image;
		subresourceRange,								// VkImageSubresourceRange	subresourceRange;
	};

	const void* barriersBeforeCompute[] = { &writeUniformConstantsBarrier, &imageLayoutBarrier };

	const VkImageMemoryBarrier imageBarrierBetweenShaders =
	{
		VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,			// VkStructureType			sType;
		DE_NULL,										// const void*				pNext;
		VK_MEMORY_OUTPUT_SHADER_WRITE_BIT,				// VkMemoryOutputFlags		outputMask;
		VK_MEMORY_INPUT_SHADER_READ_BIT,				// VkMemoryInputFlags		inputMask;
		VK_IMAGE_LAYOUT_GENERAL,						// VkImageLayout			oldLayout;
		VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,		// VkImageLayout			newLayout;
		VK_QUEUE_FAMILY_IGNORED,						// deUint32					srcQueueFamilyIndex;
		VK_QUEUE_FAMILY_IGNORED,						// deUint32					destQueueFamilyIndex;
		*image,											// VkImage					image;
		subresourceRange,								// VkImageSubresourceRange	subresourceRange;
	};
	const void* barriersAfterFirstShader[] = { &imageBarrierBetweenShaders };

	const VkBufferMemoryBarrier afterComputeBarrier = makeBufferMemoryBarrier(VK_MEMORY_OUTPUT_SHADER_WRITE_BIT, VK_MEMORY_INPUT_HOST_READ_BIT, *outputBuffer, 0ull, outputBufferSizeBytes);
	const void* barriersAfterCompute[] = { &afterComputeBarrier };

	const Unique<VkCmdPool> cmdPool(makeCommandPool(vk, device, queueFamilyIndex));
	const Unique<VkCmdBuffer> cmdBuffer(makeCommandBuffer(vk, device, *cmdPool));

	// Start recording commands

	beginCommandBuffer(vk, *cmdBuffer);

	vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline0);
	vk.cmdBindDescriptorSets(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipelineLayout, 0u, 1u, &descriptorSet.get(), 0u, DE_NULL);

	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, DE_FALSE, DE_LENGTH_OF_ARRAY(barriersBeforeCompute), barriersBeforeCompute);

	vk.cmdDispatch(*cmdBuffer, m_imageSize.x(), m_imageSize.y(), 1u);
	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, DE_FALSE, DE_LENGTH_OF_ARRAY(barriersAfterFirstShader), barriersAfterFirstShader);

	// Switch to the second shader program
	vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline1);

	vk.cmdDispatch(*cmdBuffer, m_imageSize.x(), m_imageSize.y(), 1u);
	vk.cmdPipelineBarrier(*cmdBuffer, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_HOST_BIT, DE_FALSE, DE_LENGTH_OF_ARRAY(barriersAfterCompute), barriersAfterCompute);

	endCommandBuffer(vk, *cmdBuffer);

	// Wait for completion

	submitCommandsAndWait(vk, device, queue, *cmdBuffer);

	// Validate the results

	const Allocation& outputBufferAllocation = outputBuffer.getAllocation();
	invalidateMappedMemoryRange(vk, device, outputBufferAllocation.getMemory(), outputBufferAllocation.getOffset(), outputBufferSizeBytes);

	const int		numValues = multiplyComponents(m_imageSize);
	const deUint32* bufferPtr = static_cast<deUint32*>(outputBufferAllocation.getHostPtr());
	const deUint32	res = *bufferPtr;
	deUint32		ref = 0;

	for (int ndx = 0; ndx < numValues; ++ndx)
		ref += baseValue + ndx;

	if (res != ref)
	{
		std::ostringstream msg;
		msg << "ERROR: comparison failed, expected " << ref << ", got " << res;
		return tcu::TestStatus::fail(msg.str());
	}
	return tcu::TestStatus::pass("Compute succeeded");
}

namespace EmptyShaderTest
{

void createProgram (SourceCollections& dst)
{
	dst.glslSources.add("comp") << glu::ComputeSource(
		"#version 310 es\n"
		"layout (local_size_x = 1) in;\n"
		"void main (void) {}\n"
	);
}

tcu::TestStatus createTest (Context& context)
{
	const VkDevice			device				= context.getDevice();
	const DeviceInterface&	vk					= context.getDeviceInterface();
	const VkQueue			queue				= context.getUniversalQueue();
	const deUint32			queueFamilyIndex	= context.getUniversalQueueFamilyIndex();

	const Unique<VkShaderModule> shaderModule(createShaderModule(vk, device,  context.getBinaryCollection().get("comp"), 0));
	const Unique<VkShader> shader(makeComputeShader(vk, device, *shaderModule));

	const Unique<VkPipelineLayout> pipelineLayout(makePipelineLayout(vk, device, DE_NULL));
	const Unique<VkPipeline> pipeline(makeComputePipeline(vk, device, *pipelineLayout, *shader));

	const Unique<VkCmdPool> cmdPool(makeCommandPool(vk, device, queueFamilyIndex));
	const Unique<VkCmdBuffer> cmdBuffer(makeCommandBuffer(vk, device, *cmdPool));

	// Start recording commands

	beginCommandBuffer(vk, *cmdBuffer);

	vk.cmdBindPipeline(*cmdBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, *pipeline);

	const tcu::UVec3 workGroups(1, 1, 1);
	vk.cmdDispatch(*cmdBuffer, workGroups.x(), workGroups.y(), workGroups.z());

	endCommandBuffer(vk, *cmdBuffer);

	submitCommandsAndWait(vk, device, queue, *cmdBuffer);

	return tcu::TestStatus::pass("Compute succeeded");
}

} // EmptyShaderTest ns
} // anonymous

tcu::TestCaseGroup* createBasicComputeShaderTests (tcu::TestContext& testCtx)
{
	de::MovePtr<tcu::TestCaseGroup> basicComputeTests(new tcu::TestCaseGroup(testCtx, "basic", "Basic compute tests"));

	addFunctionCaseWithPrograms(basicComputeTests.get(), "empty_shader", "Shader that does nothing", EmptyShaderTest::createProgram, EmptyShaderTest::createTest);

	basicComputeTests->addChild(BufferToBufferInvertTest::UBOToSSBOInvertCase(testCtx,	"ubo_to_ssbo_single_invocation",	"Copy from UBO to SSBO, inverting bits",	256,	tcu::UVec3(1,1,1),	tcu::UVec3(1,1,1)));
	basicComputeTests->addChild(BufferToBufferInvertTest::UBOToSSBOInvertCase(testCtx,	"ubo_to_ssbo_single_group",			"Copy from UBO to SSBO, inverting bits",	1024,	tcu::UVec3(2,1,4),	tcu::UVec3(1,1,1)));
	basicComputeTests->addChild(BufferToBufferInvertTest::UBOToSSBOInvertCase(testCtx,	"ubo_to_ssbo_multiple_invocations",	"Copy from UBO to SSBO, inverting bits",	1024,	tcu::UVec3(1,1,1),	tcu::UVec3(2,4,1)));
	basicComputeTests->addChild(BufferToBufferInvertTest::UBOToSSBOInvertCase(testCtx,	"ubo_to_ssbo_multiple_groups",		"Copy from UBO to SSBO, inverting bits",	1024,	tcu::UVec3(1,4,2),	tcu::UVec3(2,2,4)));

	basicComputeTests->addChild(BufferToBufferInvertTest::CopyInvertSSBOCase(testCtx,	"copy_ssbo_single_invocation",		"Copy between SSBOs, inverting bits",	256,	tcu::UVec3(1,1,1),	tcu::UVec3(1,1,1)));
	basicComputeTests->addChild(BufferToBufferInvertTest::CopyInvertSSBOCase(testCtx,	"copy_ssbo_multiple_invocations",	"Copy between SSBOs, inverting bits",	1024,	tcu::UVec3(1,1,1),	tcu::UVec3(2,4,1)));
	basicComputeTests->addChild(BufferToBufferInvertTest::CopyInvertSSBOCase(testCtx,	"copy_ssbo_multiple_groups",		"Copy between SSBOs, inverting bits",	1024,	tcu::UVec3(1,4,2),	tcu::UVec3(2,2,4)));

	basicComputeTests->addChild(new InvertSSBOInPlaceTest(testCtx,	"ssbo_rw_single_invocation",			"Read and write same SSBO",		256,	true,	tcu::UVec3(1,1,1),	tcu::UVec3(1,1,1)));
	basicComputeTests->addChild(new InvertSSBOInPlaceTest(testCtx,	"ssbo_rw_multiple_groups",				"Read and write same SSBO",		1024,	true,	tcu::UVec3(1,4,2),	tcu::UVec3(2,2,4)));
	basicComputeTests->addChild(new InvertSSBOInPlaceTest(testCtx,	"ssbo_unsized_arr_single_invocation",	"Read and write same SSBO",		256,	false,	tcu::UVec3(1,1,1),	tcu::UVec3(1,1,1)));
	basicComputeTests->addChild(new InvertSSBOInPlaceTest(testCtx,	"ssbo_unsized_arr_multiple_groups",		"Read and write same SSBO",		1024,	false,	tcu::UVec3(1,4,2),	tcu::UVec3(2,2,4)));

	basicComputeTests->addChild(new WriteToMultipleSSBOTest(testCtx,	"write_multiple_arr_single_invocation",			"Write to multiple SSBOs",	256,	true,	tcu::UVec3(1,1,1),	tcu::UVec3(1,1,1)));
	basicComputeTests->addChild(new WriteToMultipleSSBOTest(testCtx,	"write_multiple_arr_multiple_groups",			"Write to multiple SSBOs",	1024,	true,	tcu::UVec3(1,4,2),	tcu::UVec3(2,2,4)));
	basicComputeTests->addChild(new WriteToMultipleSSBOTest(testCtx,	"write_multiple_unsized_arr_single_invocation",	"Write to multiple SSBOs",	256,	false,	tcu::UVec3(1,1,1),	tcu::UVec3(1,1,1)));
	basicComputeTests->addChild(new WriteToMultipleSSBOTest(testCtx,	"write_multiple_unsized_arr_multiple_groups",	"Write to multiple SSBOs",	1024,	false,	tcu::UVec3(1,4,2),	tcu::UVec3(2,2,4)));

	basicComputeTests->addChild(new SSBOLocalBarrierTest(testCtx,	"ssbo_local_barrier_single_invocation",	"SSBO local barrier usage",	tcu::UVec3(1,1,1),	tcu::UVec3(1,1,1)));
	basicComputeTests->addChild(new SSBOLocalBarrierTest(testCtx,	"ssbo_local_barrier_single_group",		"SSBO local barrier usage",	tcu::UVec3(3,2,5),	tcu::UVec3(1,1,1)));
	basicComputeTests->addChild(new SSBOLocalBarrierTest(testCtx,	"ssbo_local_barrier_multiple_groups",	"SSBO local barrier usage",	tcu::UVec3(3,4,1),	tcu::UVec3(2,7,3)));

	basicComputeTests->addChild(new SSBOBarrierTest(testCtx,	"ssbo_cmd_barrier_single",		"SSBO memory barrier usage",	tcu::UVec3(1,1,1)));
	basicComputeTests->addChild(new SSBOBarrierTest(testCtx,	"ssbo_cmd_barrier_multiple",	"SSBO memory barrier usage",	tcu::UVec3(11,5,7)));

	basicComputeTests->addChild(new SharedVarTest(testCtx,	"shared_var_single_invocation",		"Basic shared variable usage",	tcu::UVec3(1,1,1),	tcu::UVec3(1,1,1)));
	basicComputeTests->addChild(new SharedVarTest(testCtx,	"shared_var_single_group",			"Basic shared variable usage",	tcu::UVec3(3,2,5),	tcu::UVec3(1,1,1)));
	basicComputeTests->addChild(new SharedVarTest(testCtx,	"shared_var_multiple_invocations",	"Basic shared variable usage",	tcu::UVec3(1,1,1),	tcu::UVec3(2,5,4)));
	basicComputeTests->addChild(new SharedVarTest(testCtx,	"shared_var_multiple_groups",		"Basic shared variable usage",	tcu::UVec3(3,4,1),	tcu::UVec3(2,7,3)));

	basicComputeTests->addChild(new SharedVarAtomicOpTest(testCtx,	"shared_atomic_op_single_invocation",		"Atomic operation with shared var",		tcu::UVec3(1,1,1),	tcu::UVec3(1,1,1)));
	basicComputeTests->addChild(new SharedVarAtomicOpTest(testCtx,	"shared_atomic_op_single_group",			"Atomic operation with shared var",		tcu::UVec3(3,2,5),	tcu::UVec3(1,1,1)));
	basicComputeTests->addChild(new SharedVarAtomicOpTest(testCtx,	"shared_atomic_op_multiple_invocations",	"Atomic operation with shared var",		tcu::UVec3(1,1,1),	tcu::UVec3(2,5,4)));
	basicComputeTests->addChild(new SharedVarAtomicOpTest(testCtx,	"shared_atomic_op_multiple_groups",			"Atomic operation with shared var",		tcu::UVec3(3,4,1),	tcu::UVec3(2,7,3)));

	basicComputeTests->addChild(new CopyImageToSSBOTest(testCtx,	"copy_image_to_ssbo_small",	"Image to SSBO copy",	tcu::UVec2(1,1),	tcu::UVec2(64,64)));
	basicComputeTests->addChild(new CopyImageToSSBOTest(testCtx,	"copy_image_to_ssbo_large",	"Image to SSBO copy",	tcu::UVec2(2,4),	tcu::UVec2(512,512)));

	basicComputeTests->addChild(new CopySSBOToImageTest(testCtx,	"copy_ssbo_to_image_small",	"SSBO to image copy",	tcu::UVec2(1, 1),	tcu::UVec2(64, 64)));
	basicComputeTests->addChild(new CopySSBOToImageTest(testCtx,	"copy_ssbo_to_image_large",	"SSBO to image copy",	tcu::UVec2(2, 4),	tcu::UVec2(512, 512)));

	basicComputeTests->addChild(new ImageAtomicOpTest(testCtx,	"image_atomic_op_local_size_1",	"Atomic operation with image",	1,	tcu::UVec2(64,64)));
	basicComputeTests->addChild(new ImageAtomicOpTest(testCtx,	"image_atomic_op_local_size_8",	"Atomic operation with image",	8,	tcu::UVec2(64,64)));

	basicComputeTests->addChild(new ImageBarrierTest(testCtx,	"image_barrier_single",		"Image barrier",	tcu::UVec2(1,1)));
	basicComputeTests->addChild(new ImageBarrierTest(testCtx,	"image_barrier_multiple",	"Image barrier",	tcu::UVec2(64,64)));

	return basicComputeTests.release();
}

} // compute
} // vkt
