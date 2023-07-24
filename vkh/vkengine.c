/*
 * Copyright (c) 2018-2021 Jean-Philippe Bruyère <jp_bruyere@hotmail.com>
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

#include "vkh/vkh.h"
#include "vkh/vkengine.h"
#include "vkh/vkh_phyinfo.h"
#include "vkh/vkh_presenter.h"
#include "vkh/vkh_image.h"
#include "vkh/vkh_device.h"

/// will merge vkengine & vk next
#include <GLFW/glfw3.h>

//#include "vkg/vkvg.h"

VkBool32 debugUtilsMessengerCallback (
	VkDebugUtilsMessageSeverityFlagBitsEXT			 messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT					 messageTypes,
	const VkDebugUtilsMessengerCallbackDataEXT*		 pCallbackData,
	void*											 pUserData) {

	switch (messageSeverity) {
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
		printf (KYEL);
		break;
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
		printf (KRED);
		break;
	default:
	case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
		printf (KGRN);
		break;
	}
	switch (messageTypes) {
	case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT:
		printf ("GEN: ");
		break;
	case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT:
		printf ("VAL: ");
		break;
	case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT:
		printf ("PRF: ");
		break;
	}

	printf (KNRM);
	printf ("%s\n", pCallbackData->pMessage);


	fflush(stdout);
	return VK_FALSE;
}

VkInstance vkengine_get_inst (VkEngine e) {
	return e->inst;
}

VkhPhyInfo* vkengine_get_phyinfos (VkEngine e, uint32_t* count, VkSurfaceKHR surface) {
	VK_CHECK_RESULT(vkEnumeratePhysicalDevices (e->inst, count, NULL));
	VkPhysicalDevice* phyDevices = (VkPhysicalDevice*)malloc((*count) * sizeof(VkPhysicalDevice));
	VK_CHECK_RESULT(vkEnumeratePhysicalDevices (e->inst, count, phyDevices));
	VkhPhyInfo* infos = (VkhPhyInfo*)malloc((*count) * sizeof(VkhPhyInfo));

	for (uint32_t i=0; i<(*count); i++)
		infos[i] = vkh_phyinfo_create (phyDevices[i], surface);

	free (phyDevices);
	return infos;
}

void vkengine_free_phyinfos (uint32_t count, VkhPhyInfo* infos) {
	for (uint32_t i=0; i<count; i++)
		vkh_phyinfo_drop (infos[i]);
	free (infos);
}
/**
 * @brief Add a Debug utils messenger to this  VkEngine. It will be destroyed on VkEngine end.
 * @param VKH application pointer containing vkInstance.
 * @param Message type flags
 * @param Message severity flags.
 * @param optional message callback, if null a default one which print to stdout is configured.
 */
void vkengine_enable_debug_messenger (VkEngine e,
	VkDebugUtilsMessageTypeFlagsEXT typeFlags,
	VkDebugUtilsMessageSeverityFlagsEXT severityFlags,
	PFN_vkDebugUtilsMessengerCallbackEXT callback){

	VkDebugUtilsMessengerCreateInfoEXT info = { .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
												.pNext = VK_NULL_HANDLE,
												.flags = 0,
												.messageSeverity = severityFlags,
												.messageType = typeFlags,
												.pUserData = NULL };
	if (callback == NULL)
		info.pfnUserCallback = (PFN_vkDebugUtilsMessengerCallbackEXT)debugUtilsMessengerCallback;
	else
		info.pfnUserCallback = callback;

	PFN_vkCreateDebugUtilsMessengerEXT	CreateDebugUtilsMessenger = (PFN_vkCreateDebugUtilsMessengerEXT)
			vkGetInstanceProcAddr(e->inst, "vkCreateDebugUtilsMessengerEXT");

	CreateDebugUtilsMessenger(e->inst, &info, VK_NULL_HANDLE, &e->debugMessenger);
}

#define TRY_LOAD_DEVICE_EXT(ext) {								\
if (vkh_phyinfo_try_get_extension_properties(pi, #ext, NULL))	\
	enabledExts[enabledExtsCount++] = #ext;						\
}

static void glfw_error_callback(int error, const char *description) {
	fprintf(stderr, "vkengine: GLFW error %d: %s\n", error, description);
}

VkSampleCountFlagBits getMaxUsableSampleCount(VkSampleCountFlags counts)
{
	if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
	if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
	if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }
	return VK_SAMPLE_COUNT_1_BIT;
}

void vkengine_dump_Infos (VkEngine e){
	printf("max samples = %d\n", getMaxUsableSampleCount(e->gpu_props.limits.framebufferColorSampleCounts));
	printf("max tex2d size = %d\n", e->gpu_props.limits.maxImageDimension2D);
	printf("max tex array layers = %d\n", e->gpu_props.limits.maxImageArrayLayers);
	printf("max mem alloc count = %d\n", e->gpu_props.limits.maxMemoryAllocationCount);

	for (uint32_t i = 0; i < e->memory_properties.memoryHeapCount; i++) {
		printf("Mem Heap %d\n", i);
		printf("\tflags= %d\n", e->memory_properties.memoryHeaps[i].flags);
		printf("\tsize = %lu Mo\n", (unsigned long)e->memory_properties.memoryHeaps[i].size/ (uint32_t)(1024*1024));
	}
	for (uint32_t i = 0; i < e->memory_properties.memoryTypeCount; i++) {
		printf("Mem type %d\n", i);
		printf("\theap %d: ", e->memory_properties.memoryTypes[i].heapIndex);
		if (e->memory_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			printf("VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT|");
		if (e->memory_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
			printf("VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|");
		if (e->memory_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
			printf("VK_MEMORY_PROPERTY_HOST_COHERENT_BIT|");
		if (e->memory_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_CACHED_BIT)
			printf("VK_MEMORY_PROPERTY_HOST_CACHED_BIT|");
		if (e->memory_properties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT)
			printf("VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT|");
		printf("\n");
	}
}

void vkengine_dump_available_layers () {
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, NULL);

	VkLayerProperties* availableLayers = (VkLayerProperties*)malloc(layerCount*sizeof(VkLayerProperties));
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers);

	printf("Available Layers:\n");
	printf("-----------------\n");
	for (uint32_t i=0; i<layerCount; i++) {
		 printf ("\t - %s\n", availableLayers[i].layerName);
	}
	printf("-----------------\n\n");
	free (availableLayers);
}

bool vkengine_try_get_phyinfo (VkhPhyInfo* phys, uint32_t phyCount, VkPhysicalDeviceType gpuType, VkhPhyInfo* phy) {
	for (uint32_t i=0; i<phyCount; i++){
		if (phys[i]->properties.deviceType == gpuType) {
			 *phy = phys[i];
			 return true;
		}
	}
	return false;
}

bool instance_extension_supported (VkExtensionProperties* instanceExtProps, uint32_t extCount, const char* instanceName) {
	for (uint32_t i=0; i<extCount; i++) {
		if (!strcmp(instanceExtProps[i].extensionName, instanceName))
			return true;
	}
	return false;
}

static VkEngine singleton;

VkEngine vkengine_create (
	uint32_t version_major, uint32_t version_minor, const char* app_name,
	VkPhysicalDeviceType preferedGPU, VkPresentModeKHR presentMode, uint32_t width, uint32_t height, int dpi_index)
{
	if (singleton)
		return singleton;
	
	glfwSetErrorCallback(glfw_error_callback);

	if (!glfwInit()) {
		perror ("glfwInit failed");
		exit(-1);
	}

	if (!glfwVulkanSupported()) {
		perror ("glfwVulkanSupported return false.");
		exit(-1);
	}

	uint32_t phyCount = 0;
	uint32_t glfwReqExtsCount = 0;
	const char** gflwExts = glfwGetRequiredInstanceExtensions (&glfwReqExtsCount);

	vkh_get_required_instance_extensions (enabledExts, &enabledExtsCount);

	for (uint32_t i=0;i<glfwReqExtsCount;i++)
		enabledExts[i+enabledExtsCount] = gflwExts[i];

	enabledExtsCount += glfwReqExtsCount;

	int count;
	GLFWmonitor** monitors = glfwGetMonitors(&count);

	e = (VkEngine)calloc(1, sizeof(vk_engine_t));
	e->refs = 1;
	/// control over the dpi selection is looked up by the user
	/// we can offer that form of index of monitors in another api
	if (count > dpi_index) {
		glfwGetMonitorContentScale(monitors[dpi_index], &e->dpi_scale_x, &e->dpi_scale_y);
	} else {
		e->dpi_scale_x = 1.0f;
		e->dpi_scale_y = 1.0f;
	}

	/// instance vk with ion (we will bring this api into that abstract next phase)
    ion::Vulkan vk { version_major, version_minor }; /// singleton; if constructed prior with a version, that remains set
    vk->init();

	e->infos = vk->app_info;
	e->debugMessenger = VK_NULL_HANDLE;
	e->inst = vk->inst();

#if defined(DEBUG) && defined (VKVG_DBG_UTILS)
	vkengine_enable_debug_messenger(e
							   , VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
							   //| VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
							   //| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
							   , VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
							   | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
							   //| VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT
							   //| VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
							   , NULL);
#endif

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE,  GLFW_TRUE);
	glfwWindowHint(GLFW_FLOATING,   GLFW_FALSE);
	glfwWindowHint(GLFW_DECORATED,  GLFW_TRUE);

	e->window = glfwCreateWindow ((int)width, (int)height, "Window Title", NULL, NULL);
	glfwShowWindow(e->window);

	VkSurfaceKHR surf;
	VK_CHECK_RESULT (glfwCreateWindowSurface(e->inst, e->window, NULL, &surf))

	VkhPhyInfo* phys = vkengine_get_phyinfos (e, &phyCount, surf);

	VkhPhyInfo pi = 0;
	if (!vkengine_try_get_phyinfo(phys, phyCount, preferedGPU, &pi)
	&&  !vkengine_try_get_phyinfo(phys, phyCount, VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU, &pi)
	&&  !vkengine_try_get_phyinfo(phys, phyCount, VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU, &pi))
		pi = phys[0];
	assert(pi && "No vulkan physical device found.");

	e->memory_properties = pi->memProps;
	e->gpu_props = pi->properties;

	uint32_t qCount = 0;
	float qPriorities[] = {0.0};

	VkDeviceQueueCreateInfo pQueueInfos[] = { {0},{0},{0} };
	if (vkh_phyinfo_create_presentable_queues	(pi, 1, qPriorities, &pQueueInfos[qCount]))
		qCount++;
	/*if (vkh_phyinfo_create_compute_queues		(pi, 1, qPriorities, &pQueueInfos[qCount]))
		qCount++;
	if (vkh_phyinfo_create_transfer_queues		(pi, 1, qPriorities, &pQueueInfos[qCount]))
		qCount++;*/

	enabledExtsCount=0;

	if (!vkh_get_required_device_extensions (pi->phy, enabledExts, &enabledExtsCount)) {
		perror ("vkh_get_required_device_extensions failed, enable log for details.\n");
		exit(-1);
	}
	TRY_LOAD_DEVICE_EXT (VK_KHR_swapchain)

	const void* pNext = vkh_get_device_requirements (pi->phy, &pi->supportedFeatures);

	VkDeviceCreateInfo device_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = qCount,
		.pQueueCreateInfos = (VkDeviceQueueCreateInfo*)&pQueueInfos,
		.enabledExtensionCount = enabledExtsCount,
		.ppEnabledExtensionNames = enabledExts,
		.pEnabledFeatures = &pi->supportedFeatures,
		.pNext = pNext};

	e->dev = vkh_device_create(e, pi, &device_info);

	e->renderer = vkh_presenter_create
			(e->dev, (uint32_t) pi->pQueue, surf, width * 2, height * 2, VK_FORMAT_B8G8R8A8_UNORM, presentMode);

	vkengine_free_phyinfos (phyCount, phys);
	singleton = e;
	return e;
}

VkEngine vkengine_grab (VkEngine e) {
	if (e)
		e->refs++;
	return e;
}

void vkengine_drop (VkEngine e) {
	if (e && --e->refs == 0) {
		//vkDeviceWaitIdle(e->dev->dev);
		VkSurfaceKHR surf = e->renderer->surface;
		vkh_presenter_destroy(e->renderer);
		vkDestroySurfaceKHR(e->inst, surf, NULL);
		vkh_device_destroy(e->dev);
		glfwDestroyWindow(e->window);
		glfwTerminate();
		///
		if (e->debugMessenger != VK_NULL_HANDLE) {
			PFN_vkDestroyDebugUtilsMessengerEXT	 DestroyDebugUtilsMessenger = (PFN_vkDestroyDebugUtilsMessengerEXT)
					vkGetInstanceProcAddr(e->inst, "vkDestroyDebugUtilsMessengerEXT");
			DestroyDebugUtilsMessenger (e->inst, e->debugMessenger, VK_NULL_HANDLE);
		}
		vkDestroyInstance (e->inst, NULL);
		free(e);
	}
}

/// i think vkengine should support multiple windows
void vkengine_close (VkEngine e) {
	glfwSetWindowShouldClose(e->window, GLFW_TRUE);
}

void vkengine_blitter_run (VkEngine e, VkImage img, uint32_t width, uint32_t height) {
	VkhPresenter p = e->renderer;
	vkh_presenter_build_blit_cmd (p, img, width, height);

	while (!vkengine_should_close (e)) {
		glfwPollEvents();
		if (!vkh_presenter_draw (p))
			vkh_presenter_build_blit_cmd (p, img, width, height);
	}
}

bool vkengine_should_close(VkEngine e) {
	return glfwWindowShouldClose (e->window);
}

void vkengine_set_title(VkEngine e, const char* title) {
	glfwSetWindowTitle(e->window, title);
}

VkInstance vkengine_get_instance(VkEngine e) {
	return e->dev->instance;
}

VkDevice vkengine_get_device(VkEngine e) {
	return e->dev->dev;
}

VkPhysicalDevice vkengine_get_physical_device(VkEngine e) {
	return e->dev->phy;
}

VkQueue vkengine_get_queue(VkEngine e) {
	return e->renderer->queue;
}

uint32_t vkengine_get_queue_fam_idx(VkEngine e) {
	return e->renderer->qFam;
}

void vkengine_wait_idle (VkEngine e) {
	vkDeviceWaitIdle(e->dev->dev);
}

void vkengine_set_key_callback (VkEngine e, GLFWkeyfun key_callback){
	glfwSetKeyCallback (e->window, key_callback);
}

void vkengine_set_mouse_but_callback (VkEngine e, GLFWmousebuttonfun onMouseBut){
	glfwSetMouseButtonCallback(e->window, onMouseBut);
}

void vkengine_set_cursor_pos_callback (VkEngine e, GLFWcursorposfun onMouseMove){
	glfwSetCursorPosCallback(e->window, onMouseMove);
}

void vkengine_set_scroll_callback (VkEngine e, GLFWscrollfun onScroll){
	glfwSetScrollCallback(e->window, onScroll);
}

void vkengine_set_char_callback (VkEngine e, GLFWcharfun onChar){
	glfwSetCharCallback(e->window, onChar);
}
