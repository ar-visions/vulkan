/*
 * Copyright (c) 2018-2022 Jean-Philippe Bruyère <jp_bruyere@hotmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the
 * Software, and to permit persons to whom the Software is furnished to do so, subject
 * to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include <vkh/vkh_device.h>
#include <vkh/vkh_phyinfo.h>
#include <vkh/vkh_app.h>
#include "string.h"


static PFN_vkSetDebugUtilsObjectNameEXT		SetDebugUtilsObjectNameEXT;
static PFN_vkQueueBeginDebugUtilsLabelEXT	QueueBeginDebugUtilsLabelEXT;
static PFN_vkQueueEndDebugUtilsLabelEXT		QueueEndDebugUtilsLabelEXT;
static PFN_vkCmdBeginDebugUtilsLabelEXT		CmdBeginDebugUtilsLabelEXT;
static PFN_vkCmdEndDebugUtilsLabelEXT		CmdEndDebugUtilsLabelEXT;
static PFN_vkCmdInsertDebugUtilsLabelEXT	CmdInsertDebugUtilsLabelEXT;


#define _CHECK_INST_EXT(ext)\
if (vkh_instance_extension_supported(#ext)) {	\
	if (pExtensions)							\
	   pExtensions[*pExtCount] = #ext;			\
	(*pExtCount)++;								\
}
void vkh_get_required_instance_extensions (const char** pExtensions, uint32_t* pExtCount) {
	*pExtCount = 0;

	vkh_instance_extensions_check_init ();

#if defined(DEBUG) && defined (VKVG_DBG_UTILS)
	_CHECK_INST_EXT(VK_EXT_debug_utils)
#endif
	_CHECK_INST_EXT(VK_KHR_get_physical_device_properties2)

	vkh_instance_extensions_check_release();
}

bool _get_dev_extension_is_supported (VkExtensionProperties* pExtensionProperties, uint32_t extensionCount, const char* name) {
	for (uint32_t i=0; i<extensionCount; i++) {
		if (strcmp(name, pExtensionProperties[i].extensionName)==0)
			return true;
	}
	return false;
}
#define _CHECK_DEV_EXT(ext) {					\
	if (_get_dev_extension_is_supported(pExtensionProperties, extensionCount, #ext)){\
		if (pExtensions)							\
			pExtensions[*pExtCount] = #ext;			\
		(*pExtCount)++;								\
	}\
}

bool vkh_get_required_device_extensions (VkPhysicalDevice phy, const char** pExtensions, uint32_t* pExtCount) {
	VkExtensionProperties* pExtensionProperties;
	uint32_t extensionCount;

	*pExtCount = 0;

	VK_CHECK_RESULT(vkEnumerateDeviceExtensionProperties(phy, NULL, &extensionCount, NULL));
	pExtensionProperties = (VkExtensionProperties*)malloc(extensionCount * sizeof(VkExtensionProperties));
	VK_CHECK_RESULT(vkEnumerateDeviceExtensionProperties(phy, NULL, &extensionCount, pExtensionProperties));

	//https://vulkan.lunarg.com/doc/view/1.2.162.0/mac/1.2-extensions/vkspec.html#VK_KHR_portability_subset
	_CHECK_DEV_EXT(VK_KHR_portability_subset);
	VkPhysicalDeviceFeatures2 phyFeat2 = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2};

	vkGetPhysicalDeviceFeatures2(phy, &phyFeat2);
	return true;
}

//enabledFeature12 is guaranteed to be the first in pNext chain
const void* vkh_get_device_requirements (VkPhysicalDevice phy, VkPhysicalDeviceFeatures* pEnabledFeatures) {

	VkPhysicalDeviceFeatures supported;
	vkGetPhysicalDeviceFeatures(phy, &supported);

	pEnabledFeatures->fillModeNonSolid	= supported.fillModeNonSolid;  // VK_TRUE;
	pEnabledFeatures->sampleRateShading	= supported.sampleRateShading; // VK_TRUE;
	pEnabledFeatures->logicOp			= supported.logicOp; 		   // VK_TRUE;

	void* pNext = NULL;

#ifdef VK_VERSION_1_2
	static VkPhysicalDeviceVulkan12Features enabledFeatures12 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES
	};
	enabledFeatures12.pNext = pNext;
	pNext = &enabledFeatures12;
#endif

	return pNext;
}


VkhDevice vkh_device_create (VkEngine e, VkhPhyInfo phyInfo, VkDeviceCreateInfo* pDevice_info){
	VkDevice dev;
	VK_CHECK_RESULT(vkCreateDevice (phyInfo->phy, pDevice_info, NULL, &dev));
	VkhDevice vkhd = vkh_device_import(e->inst, phyInfo->phy, dev);
	vkhd->e = e;
	vkhd->phyinfo = vkh_phyinfo_grab(phyInfo);
	return vkhd;
}
VkhDevice vkh_device_import (VkInstance inst, VkPhysicalDevice phy, VkDevice vkDev) {
	VkhDevice dev = (vkh_device_t*)calloc(1,sizeof(vkh_device_t));
	dev->dev = vkDev;
	dev->phy = phy;
	dev->instance = inst;

	vkGetPhysicalDeviceMemoryProperties (phy, &dev->phyMemProps);
#ifdef VKH_USE_VMA
	VmaAllocatorCreateInfo allocatorInfo = {
		.physicalDevice = phy,
		.device = vkDev
	};
	vmaCreateAllocator(&allocatorInfo, &dev->allocator);
#else
#endif

	return dev;
}
VkDevice vkh_device_get_vkdev (VkhDevice dev) {
	return dev->dev;
}
VkPhysicalDevice vkh_device_get_phy (VkhDevice dev) {
	return dev->phy;
}
VkEngine vkh_device_get_engine (VkhDevice dev) {
	return dev->e;
}
/**
 * @brief get instance proc addresses for debug utils (name, label,...)
 * @param vkh device
 */
void vkh_device_init_debug_utils (VkhDevice dev) {
	SetDebugUtilsObjectNameEXT		= (PFN_vkSetDebugUtilsObjectNameEXT)	vkGetInstanceProcAddr(dev->instance, "vkSetDebugUtilsObjectNameEXT");
	QueueBeginDebugUtilsLabelEXT	= (PFN_vkQueueBeginDebugUtilsLabelEXT)	vkGetInstanceProcAddr(dev->instance, "vkQueueBeginDebugUtilsLabelEXT");
	QueueEndDebugUtilsLabelEXT		= (PFN_vkQueueEndDebugUtilsLabelEXT)	vkGetInstanceProcAddr(dev->instance, "vkQueueEndDebugUtilsLabelEXT");
	CmdBeginDebugUtilsLabelEXT		= (PFN_vkCmdBeginDebugUtilsLabelEXT)	vkGetInstanceProcAddr(dev->instance, "vkCmdBeginDebugUtilsLabelEXT");
	CmdEndDebugUtilsLabelEXT		= (PFN_vkCmdEndDebugUtilsLabelEXT)		vkGetInstanceProcAddr(dev->instance, "vkCmdEndDebugUtilsLabelEXT");
	CmdInsertDebugUtilsLabelEXT		= (PFN_vkCmdInsertDebugUtilsLabelEXT)	vkGetInstanceProcAddr(dev->instance, "vkCmdInsertDebugUtilsLabelEXT");
}
VkSampler vkh_device_create_sampler (VkhDevice dev, VkFilter magFilter, VkFilter minFilter,
							   VkSamplerMipmapMode mipmapMode, VkSamplerAddressMode addressMode){
	VkSampler sampler = VK_NULL_HANDLE;
	VkSamplerCreateInfo samplerCreateInfo = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
											  .maxAnisotropy= 1.0,
											  .addressModeU = addressMode,
											  .addressModeV = addressMode,
											  .addressModeW = addressMode,
											  .magFilter	= magFilter,
											  .minFilter	= minFilter,
											  .mipmapMode	= mipmapMode};
	VK_CHECK_RESULT(vkCreateSampler(dev->dev, &samplerCreateInfo, NULL, &sampler));
	return sampler;
}
void vkh_device_destroy_sampler (VkhDevice dev, VkSampler sampler) {
	vkDestroySampler (dev->dev, sampler, NULL);
}
void vkh_device_destroy (VkhDevice dev) {
#ifdef VKH_USE_VMA
	vmaDestroyAllocator (dev->allocator);
#else
#endif
	vkDestroyDevice (dev->dev, NULL);
	free (dev);
}

void vkh_device_set_object_name (VkhDevice dev, VkObjectType objectType, uint64_t handle, const char* name){
	const VkDebugUtilsObjectNameInfoEXT info = {
		.sType		 = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
		.pNext		 = 0,
		.objectType	 = objectType,
		.objectHandle= handle,
		.pObjectName = name
	};
	SetDebugUtilsObjectNameEXT (dev->dev, &info);
}
void vkh_cmd_label_start (VkCommandBuffer cmd, const char* name, const float color[4]) {
	const VkDebugUtilsLabelEXT info = {
		.sType		= VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
		.pNext		= 0,
		.pLabelName= name
	};
	memcpy ((void*)info.color, (void*)color, 4 * sizeof(float));
	CmdBeginDebugUtilsLabelEXT (cmd, &info);
}
void vkh_cmd_label_insert (VkCommandBuffer cmd, const char* name, const float color[4]) {
	const VkDebugUtilsLabelEXT info = {
		.sType		= VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
		.pNext		= 0,
		.pLabelName= name
	};
	memcpy ((void*)info.color, (void*)color, 4 * sizeof(float));
	CmdInsertDebugUtilsLabelEXT (cmd, &info);
}
void vkh_cmd_label_end (VkCommandBuffer cmd) {
	CmdEndDebugUtilsLabelEXT (cmd);
}
