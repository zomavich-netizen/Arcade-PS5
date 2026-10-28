// SPDX-License-Identifier: GPL-3.0-or-later
#include "gfx/vulkan_processor.hpp"
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan.h>
#include "gfx/shaders/arcade_effects_spv.hpp"
#include <algorithm>
#include <cstring>
#include <cstdlib>
#include <stdexcept>
#include <vector>
#include "gfx/font_metrics.hpp"
#include "gfx/png_image.hpp"
#ifdef ARCADE_PS5
extern "C" PFN_vkVoidFunction radv_GetInstanceProcAddr(VkInstance,const char*);
#else
extern "C" PFN_vkVoidFunction vkGetInstanceProcAddr(VkInstance,const char*);
#endif
namespace arcade::native {
struct VulkanProcessor::State {
 VkInstance instance=VK_NULL_HANDLE;VkPhysicalDevice physical=VK_NULL_HANDLE;VkDevice device=VK_NULL_HANDLE;VkQueue queue=VK_NULL_HANDLE;
 VkDescriptorSetLayout descriptor_layout=VK_NULL_HANDLE;VkPipelineLayout pipeline_layout=VK_NULL_HANDLE;VkPipeline pipeline=VK_NULL_HANDLE;VkDescriptorPool descriptor_pool=VK_NULL_HANDLE;VkDescriptorSet descriptor=VK_NULL_HANDLE;
 VkCommandPool command_pool=VK_NULL_HANDLE;VkCommandBuffer command=VK_NULL_HANDLE;VkFence fence=VK_NULL_HANDLE;VkPhysicalDeviceMemoryProperties memory{};
 std::vector<unsigned char> hud_font;
 VkBuffer buffers[2]{};VkDeviceMemory allocations[2]{};void* mapped[2]{};VkDeviceSize sizes[2]{};std::vector<std::uint32_t> pixels;
 VkSurfaceKHR surface=VK_NULL_HANDLE;VkSwapchainKHR swapchain=VK_NULL_HANDLE;std::vector<VkImage> images;std::vector<bool> presented;
 VkSemaphore acquired=VK_NULL_HANDLE;std::vector<VkSemaphore> rendered;unsigned target_width=0,target_height=0,queue_family=0;
 PFN_vkDestroyInstance DestroyInstance=nullptr;
 PFN_vkEnumeratePhysicalDevices EnumeratePhysicalDevices=nullptr;
 PFN_vkGetPhysicalDeviceQueueFamilyProperties GetPhysicalDeviceQueueFamilyProperties=nullptr;
 PFN_vkGetPhysicalDeviceMemoryProperties GetPhysicalDeviceMemoryProperties=nullptr;
 PFN_vkCreateDevice CreateDevice=nullptr;
 PFN_vkGetDeviceProcAddr GetDeviceProcAddr=nullptr;
 PFN_vkDestroyDevice DestroyDevice=nullptr;
 PFN_vkGetDeviceQueue GetDeviceQueue=nullptr;
 PFN_vkCreateBuffer CreateBuffer=nullptr;
 PFN_vkDestroyBuffer DestroyBuffer=nullptr;
 PFN_vkGetBufferMemoryRequirements GetBufferMemoryRequirements=nullptr;
 PFN_vkAllocateMemory AllocateMemory=nullptr;
 PFN_vkFreeMemory FreeMemory=nullptr;
 PFN_vkBindBufferMemory BindBufferMemory=nullptr;
 PFN_vkMapMemory MapMemory=nullptr;
 PFN_vkUnmapMemory UnmapMemory=nullptr;
 PFN_vkCreateDescriptorSetLayout CreateDescriptorSetLayout=nullptr;
 PFN_vkDestroyDescriptorSetLayout DestroyDescriptorSetLayout=nullptr;
 PFN_vkCreatePipelineLayout CreatePipelineLayout=nullptr;
 PFN_vkDestroyPipelineLayout DestroyPipelineLayout=nullptr;
 PFN_vkCreateShaderModule CreateShaderModule=nullptr;
 PFN_vkDestroyShaderModule DestroyShaderModule=nullptr;
 PFN_vkCreateComputePipelines CreateComputePipelines=nullptr;
 PFN_vkDestroyPipeline DestroyPipeline=nullptr;
 PFN_vkCreateDescriptorPool CreateDescriptorPool=nullptr;
 PFN_vkDestroyDescriptorPool DestroyDescriptorPool=nullptr;
 PFN_vkAllocateDescriptorSets AllocateDescriptorSets=nullptr;
 PFN_vkUpdateDescriptorSets UpdateDescriptorSets=nullptr;
 PFN_vkCreateCommandPool CreateCommandPool=nullptr;
 PFN_vkDestroyCommandPool DestroyCommandPool=nullptr;
 PFN_vkAllocateCommandBuffers AllocateCommandBuffers=nullptr;
 PFN_vkResetCommandPool ResetCommandPool=nullptr;
 PFN_vkBeginCommandBuffer BeginCommandBuffer=nullptr;
 PFN_vkEndCommandBuffer EndCommandBuffer=nullptr;
 PFN_vkCmdPipelineBarrier CmdPipelineBarrier=nullptr;
 PFN_vkCmdBindPipeline CmdBindPipeline=nullptr;
 PFN_vkCmdBindDescriptorSets CmdBindDescriptorSets=nullptr;
 PFN_vkCmdPushConstants CmdPushConstants=nullptr;
 PFN_vkCmdDispatch CmdDispatch=nullptr;
 PFN_vkCreateFence CreateFence=nullptr;
 PFN_vkDestroyFence DestroyFence=nullptr;
 PFN_vkResetFences ResetFences=nullptr;
 PFN_vkQueueSubmit QueueSubmit=nullptr;
 PFN_vkWaitForFences WaitForFences=nullptr;
 PFN_vkDeviceWaitIdle DeviceWaitIdle=nullptr;
 PFN_vkGetPhysicalDeviceDisplayPropertiesKHR GetPhysicalDeviceDisplayPropertiesKHR=nullptr;
 PFN_vkGetDisplayModePropertiesKHR GetDisplayModePropertiesKHR=nullptr;
 PFN_vkCreateDisplayPlaneSurfaceKHR CreateDisplayPlaneSurfaceKHR=nullptr;
 PFN_vkGetPhysicalDeviceSurfaceSupportKHR GetPhysicalDeviceSurfaceSupportKHR=nullptr;
 PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR GetPhysicalDeviceSurfaceCapabilitiesKHR=nullptr;
 PFN_vkDestroySurfaceKHR DestroySurfaceKHR=nullptr;
 PFN_vkCreateSwapchainKHR CreateSwapchainKHR=nullptr;
 PFN_vkDestroySwapchainKHR DestroySwapchainKHR=nullptr;
 PFN_vkGetSwapchainImagesKHR GetSwapchainImagesKHR=nullptr;
 PFN_vkAcquireNextImageKHR AcquireNextImageKHR=nullptr;
 PFN_vkQueuePresentKHR QueuePresentKHR=nullptr;
 PFN_vkCreateSemaphore CreateSemaphore=nullptr;
 PFN_vkDestroySemaphore DestroySemaphore=nullptr;
 PFN_vkCmdCopyBufferToImage CmdCopyBufferToImage=nullptr;
 void release_buffers(){for(int i=0;i<2;++i){if(mapped[i])UnmapMemory(device,allocations[i]);if(buffers[i])DestroyBuffer(device,buffers[i],nullptr);if(allocations[i])FreeMemory(device,allocations[i],nullptr);mapped[i]=nullptr;buffers[i]=VK_NULL_HANDLE;allocations[i]=VK_NULL_HANDLE;sizes[i]=0;}}
 void release_display(){if(device&&DeviceWaitIdle)DeviceWaitIdle(device);if(acquired)DestroySemaphore(device,acquired,nullptr);for(auto semaphore:rendered)if(semaphore)DestroySemaphore(device,semaphore,nullptr);rendered.clear();acquired=VK_NULL_HANDLE;if(swapchain)DestroySwapchainKHR(device,swapchain,nullptr);swapchain=VK_NULL_HANDLE;if(surface)DestroySurfaceKHR(instance,surface,nullptr);surface=VK_NULL_HANDLE;images.clear();presented.clear();target_width=target_height=0;}
 ~State(){release_display();if(device){if(DeviceWaitIdle)DeviceWaitIdle(device);release_buffers();if(fence)DestroyFence(device,fence,nullptr);if(command_pool)DestroyCommandPool(device,command_pool,nullptr);if(descriptor_pool)DestroyDescriptorPool(device,descriptor_pool,nullptr);if(pipeline)DestroyPipeline(device,pipeline,nullptr);if(pipeline_layout)DestroyPipelineLayout(device,pipeline_layout,nullptr);if(descriptor_layout)DestroyDescriptorSetLayout(device,descriptor_layout,nullptr);if(DestroyDevice)DestroyDevice(device,nullptr);}if(instance&&DestroyInstance)DestroyInstance(instance,nullptr);}
};
namespace {
void check(VkResult result,const char* operation){if(result!=VK_SUCCESS)throw std::runtime_error(std::string("Vulkan ")+operation+" failed ("+std::to_string(result)+")");}
}
VulkanProcessor::VulkanProcessor()=default;VulkanProcessor::~VulkanProcessor()=default;
void VulkanProcessor::close(){state_.reset();}
bool VulkanProcessor::open(std::string& error,const std::string& assets){
 close();auto state=std::make_unique<State>();auto& s=*state;
 if(!assets.empty()){auto font=load_png_image(assets+"/native-font-1.png");if(font&&font.width==1024&&font.height==480)s.hud_font=std::move(font.pixels);}
 try{
#ifdef ARCADE_PS5
  auto get=radv_GetInstanceProcAddr;
#else
  auto get=vkGetInstanceProcAddr;
#endif
  auto create=reinterpret_cast<PFN_vkCreateInstance>(get(VK_NULL_HANDLE,"vkCreateInstance"));if(!create)throw std::runtime_error("Vulkan instance entry point is unavailable");
  VkApplicationInfo app{};app.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO;app.pApplicationName="Arcade Gallery";app.apiVersion=VK_API_VERSION_1_1;VkInstanceCreateInfo instance{};instance.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;instance.pApplicationInfo=&app;
#ifdef ARCADE_PS5
 const char* instance_extensions[]={VK_KHR_SURFACE_EXTENSION_NAME,VK_KHR_DISPLAY_EXTENSION_NAME};instance.enabledExtensionCount=2;instance.ppEnabledExtensionNames=instance_extensions;
#endif
 check(create(&instance,nullptr,&s.instance),"create instance");
  s.DestroyInstance=reinterpret_cast<PFN_vkDestroyInstance>(get(s.instance,"vkDestroyInstance"));if(!s.DestroyInstance)throw std::runtime_error("Missing Vulkan DestroyInstance");
  s.EnumeratePhysicalDevices=reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(get(s.instance,"vkEnumeratePhysicalDevices"));if(!s.EnumeratePhysicalDevices)throw std::runtime_error("Missing Vulkan EnumeratePhysicalDevices");
  s.GetPhysicalDeviceQueueFamilyProperties=reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(get(s.instance,"vkGetPhysicalDeviceQueueFamilyProperties"));if(!s.GetPhysicalDeviceQueueFamilyProperties)throw std::runtime_error("Missing Vulkan GetPhysicalDeviceQueueFamilyProperties");
  s.GetPhysicalDeviceMemoryProperties=reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties>(get(s.instance,"vkGetPhysicalDeviceMemoryProperties"));if(!s.GetPhysicalDeviceMemoryProperties)throw std::runtime_error("Missing Vulkan GetPhysicalDeviceMemoryProperties");
  s.CreateDevice=reinterpret_cast<PFN_vkCreateDevice>(get(s.instance,"vkCreateDevice"));if(!s.CreateDevice)throw std::runtime_error("Missing Vulkan CreateDevice");
  s.GetDeviceProcAddr=reinterpret_cast<PFN_vkGetDeviceProcAddr>(get(s.instance,"vkGetDeviceProcAddr"));if(!s.GetDeviceProcAddr)throw std::runtime_error("Missing Vulkan GetDeviceProcAddr");
  uint32_t count=0;check(s.EnumeratePhysicalDevices(s.instance,&count,nullptr),"enumerate physical devices");if(!count)throw std::runtime_error("No Vulkan device available");std::vector<VkPhysicalDevice> devices(count);check(s.EnumeratePhysicalDevices(s.instance,&count,devices.data()),"get physical devices");s.physical=devices.front();
  s.GetPhysicalDeviceQueueFamilyProperties(s.physical,&count,nullptr);std::vector<VkQueueFamilyProperties> families(count);s.GetPhysicalDeviceQueueFamilyProperties(s.physical,&count,families.data());uint32_t family=count;for(uint32_t i=0;i<count;++i)if(families[i].queueFlags&VK_QUEUE_COMPUTE_BIT){family=i;break;}if(family==count)throw std::runtime_error("No Vulkan compute queue available");
  float priority=1;VkDeviceQueueCreateInfo queue{};queue.sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;queue.queueFamilyIndex=family;queue.queueCount=1;queue.pQueuePriorities=&priority;VkDeviceCreateInfo device{};device.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;device.queueCreateInfoCount=1;device.pQueueCreateInfos=&queue;
#ifdef ARCADE_PS5
 const char* extensions[]={VK_KHR_SWAPCHAIN_EXTENSION_NAME};device.enabledExtensionCount=1;device.ppEnabledExtensionNames=extensions;
#endif
 check(s.CreateDevice(s.physical,&device,nullptr,&s.device),"create device");
  s.DestroyDevice=reinterpret_cast<PFN_vkDestroyDevice>(s.GetDeviceProcAddr(s.device,"vkDestroyDevice"));if(!s.DestroyDevice)throw std::runtime_error("Missing Vulkan DestroyDevice");
  s.GetDeviceQueue=reinterpret_cast<PFN_vkGetDeviceQueue>(s.GetDeviceProcAddr(s.device,"vkGetDeviceQueue"));if(!s.GetDeviceQueue)throw std::runtime_error("Missing Vulkan GetDeviceQueue");
  s.CreateBuffer=reinterpret_cast<PFN_vkCreateBuffer>(s.GetDeviceProcAddr(s.device,"vkCreateBuffer"));if(!s.CreateBuffer)throw std::runtime_error("Missing Vulkan CreateBuffer");
  s.DestroyBuffer=reinterpret_cast<PFN_vkDestroyBuffer>(s.GetDeviceProcAddr(s.device,"vkDestroyBuffer"));if(!s.DestroyBuffer)throw std::runtime_error("Missing Vulkan DestroyBuffer");
  s.GetBufferMemoryRequirements=reinterpret_cast<PFN_vkGetBufferMemoryRequirements>(s.GetDeviceProcAddr(s.device,"vkGetBufferMemoryRequirements"));if(!s.GetBufferMemoryRequirements)throw std::runtime_error("Missing Vulkan GetBufferMemoryRequirements");
  s.AllocateMemory=reinterpret_cast<PFN_vkAllocateMemory>(s.GetDeviceProcAddr(s.device,"vkAllocateMemory"));if(!s.AllocateMemory)throw std::runtime_error("Missing Vulkan AllocateMemory");
  s.FreeMemory=reinterpret_cast<PFN_vkFreeMemory>(s.GetDeviceProcAddr(s.device,"vkFreeMemory"));if(!s.FreeMemory)throw std::runtime_error("Missing Vulkan FreeMemory");
  s.BindBufferMemory=reinterpret_cast<PFN_vkBindBufferMemory>(s.GetDeviceProcAddr(s.device,"vkBindBufferMemory"));if(!s.BindBufferMemory)throw std::runtime_error("Missing Vulkan BindBufferMemory");
  s.MapMemory=reinterpret_cast<PFN_vkMapMemory>(s.GetDeviceProcAddr(s.device,"vkMapMemory"));if(!s.MapMemory)throw std::runtime_error("Missing Vulkan MapMemory");
  s.UnmapMemory=reinterpret_cast<PFN_vkUnmapMemory>(s.GetDeviceProcAddr(s.device,"vkUnmapMemory"));if(!s.UnmapMemory)throw std::runtime_error("Missing Vulkan UnmapMemory");
  s.CreateDescriptorSetLayout=reinterpret_cast<PFN_vkCreateDescriptorSetLayout>(s.GetDeviceProcAddr(s.device,"vkCreateDescriptorSetLayout"));if(!s.CreateDescriptorSetLayout)throw std::runtime_error("Missing Vulkan CreateDescriptorSetLayout");
  s.DestroyDescriptorSetLayout=reinterpret_cast<PFN_vkDestroyDescriptorSetLayout>(s.GetDeviceProcAddr(s.device,"vkDestroyDescriptorSetLayout"));if(!s.DestroyDescriptorSetLayout)throw std::runtime_error("Missing Vulkan DestroyDescriptorSetLayout");
  s.CreatePipelineLayout=reinterpret_cast<PFN_vkCreatePipelineLayout>(s.GetDeviceProcAddr(s.device,"vkCreatePipelineLayout"));if(!s.CreatePipelineLayout)throw std::runtime_error("Missing Vulkan CreatePipelineLayout");
  s.DestroyPipelineLayout=reinterpret_cast<PFN_vkDestroyPipelineLayout>(s.GetDeviceProcAddr(s.device,"vkDestroyPipelineLayout"));if(!s.DestroyPipelineLayout)throw std::runtime_error("Missing Vulkan DestroyPipelineLayout");
  s.CreateShaderModule=reinterpret_cast<PFN_vkCreateShaderModule>(s.GetDeviceProcAddr(s.device,"vkCreateShaderModule"));if(!s.CreateShaderModule)throw std::runtime_error("Missing Vulkan CreateShaderModule");
  s.DestroyShaderModule=reinterpret_cast<PFN_vkDestroyShaderModule>(s.GetDeviceProcAddr(s.device,"vkDestroyShaderModule"));if(!s.DestroyShaderModule)throw std::runtime_error("Missing Vulkan DestroyShaderModule");
  s.CreateComputePipelines=reinterpret_cast<PFN_vkCreateComputePipelines>(s.GetDeviceProcAddr(s.device,"vkCreateComputePipelines"));if(!s.CreateComputePipelines)throw std::runtime_error("Missing Vulkan CreateComputePipelines");
  s.DestroyPipeline=reinterpret_cast<PFN_vkDestroyPipeline>(s.GetDeviceProcAddr(s.device,"vkDestroyPipeline"));if(!s.DestroyPipeline)throw std::runtime_error("Missing Vulkan DestroyPipeline");
  s.CreateDescriptorPool=reinterpret_cast<PFN_vkCreateDescriptorPool>(s.GetDeviceProcAddr(s.device,"vkCreateDescriptorPool"));if(!s.CreateDescriptorPool)throw std::runtime_error("Missing Vulkan CreateDescriptorPool");
  s.DestroyDescriptorPool=reinterpret_cast<PFN_vkDestroyDescriptorPool>(s.GetDeviceProcAddr(s.device,"vkDestroyDescriptorPool"));if(!s.DestroyDescriptorPool)throw std::runtime_error("Missing Vulkan DestroyDescriptorPool");
  s.AllocateDescriptorSets=reinterpret_cast<PFN_vkAllocateDescriptorSets>(s.GetDeviceProcAddr(s.device,"vkAllocateDescriptorSets"));if(!s.AllocateDescriptorSets)throw std::runtime_error("Missing Vulkan AllocateDescriptorSets");
  s.UpdateDescriptorSets=reinterpret_cast<PFN_vkUpdateDescriptorSets>(s.GetDeviceProcAddr(s.device,"vkUpdateDescriptorSets"));if(!s.UpdateDescriptorSets)throw std::runtime_error("Missing Vulkan UpdateDescriptorSets");
  s.CreateCommandPool=reinterpret_cast<PFN_vkCreateCommandPool>(s.GetDeviceProcAddr(s.device,"vkCreateCommandPool"));if(!s.CreateCommandPool)throw std::runtime_error("Missing Vulkan CreateCommandPool");
  s.DestroyCommandPool=reinterpret_cast<PFN_vkDestroyCommandPool>(s.GetDeviceProcAddr(s.device,"vkDestroyCommandPool"));if(!s.DestroyCommandPool)throw std::runtime_error("Missing Vulkan DestroyCommandPool");
  s.AllocateCommandBuffers=reinterpret_cast<PFN_vkAllocateCommandBuffers>(s.GetDeviceProcAddr(s.device,"vkAllocateCommandBuffers"));if(!s.AllocateCommandBuffers)throw std::runtime_error("Missing Vulkan AllocateCommandBuffers");
  s.ResetCommandPool=reinterpret_cast<PFN_vkResetCommandPool>(s.GetDeviceProcAddr(s.device,"vkResetCommandPool"));if(!s.ResetCommandPool)throw std::runtime_error("Missing Vulkan ResetCommandPool");
  s.BeginCommandBuffer=reinterpret_cast<PFN_vkBeginCommandBuffer>(s.GetDeviceProcAddr(s.device,"vkBeginCommandBuffer"));if(!s.BeginCommandBuffer)throw std::runtime_error("Missing Vulkan BeginCommandBuffer");
  s.EndCommandBuffer=reinterpret_cast<PFN_vkEndCommandBuffer>(s.GetDeviceProcAddr(s.device,"vkEndCommandBuffer"));if(!s.EndCommandBuffer)throw std::runtime_error("Missing Vulkan EndCommandBuffer");
  s.CmdPipelineBarrier=reinterpret_cast<PFN_vkCmdPipelineBarrier>(s.GetDeviceProcAddr(s.device,"vkCmdPipelineBarrier"));if(!s.CmdPipelineBarrier)throw std::runtime_error("Missing Vulkan CmdPipelineBarrier");
  s.CmdBindPipeline=reinterpret_cast<PFN_vkCmdBindPipeline>(s.GetDeviceProcAddr(s.device,"vkCmdBindPipeline"));if(!s.CmdBindPipeline)throw std::runtime_error("Missing Vulkan CmdBindPipeline");
  s.CmdBindDescriptorSets=reinterpret_cast<PFN_vkCmdBindDescriptorSets>(s.GetDeviceProcAddr(s.device,"vkCmdBindDescriptorSets"));if(!s.CmdBindDescriptorSets)throw std::runtime_error("Missing Vulkan CmdBindDescriptorSets");
  s.CmdPushConstants=reinterpret_cast<PFN_vkCmdPushConstants>(s.GetDeviceProcAddr(s.device,"vkCmdPushConstants"));if(!s.CmdPushConstants)throw std::runtime_error("Missing Vulkan CmdPushConstants");
  s.CmdDispatch=reinterpret_cast<PFN_vkCmdDispatch>(s.GetDeviceProcAddr(s.device,"vkCmdDispatch"));if(!s.CmdDispatch)throw std::runtime_error("Missing Vulkan CmdDispatch");
  s.CreateFence=reinterpret_cast<PFN_vkCreateFence>(s.GetDeviceProcAddr(s.device,"vkCreateFence"));if(!s.CreateFence)throw std::runtime_error("Missing Vulkan CreateFence");
  s.DestroyFence=reinterpret_cast<PFN_vkDestroyFence>(s.GetDeviceProcAddr(s.device,"vkDestroyFence"));if(!s.DestroyFence)throw std::runtime_error("Missing Vulkan DestroyFence");
  s.ResetFences=reinterpret_cast<PFN_vkResetFences>(s.GetDeviceProcAddr(s.device,"vkResetFences"));if(!s.ResetFences)throw std::runtime_error("Missing Vulkan ResetFences");
  s.QueueSubmit=reinterpret_cast<PFN_vkQueueSubmit>(s.GetDeviceProcAddr(s.device,"vkQueueSubmit"));if(!s.QueueSubmit)throw std::runtime_error("Missing Vulkan QueueSubmit");
  s.WaitForFences=reinterpret_cast<PFN_vkWaitForFences>(s.GetDeviceProcAddr(s.device,"vkWaitForFences"));if(!s.WaitForFences)throw std::runtime_error("Missing Vulkan WaitForFences");
  s.DeviceWaitIdle=reinterpret_cast<PFN_vkDeviceWaitIdle>(s.GetDeviceProcAddr(s.device,"vkDeviceWaitIdle"));if(!s.DeviceWaitIdle)throw std::runtime_error("Missing Vulkan DeviceWaitIdle");
  s.queue_family=family;s.GetDeviceQueue(s.device,family,0,&s.queue);s.GetPhysicalDeviceMemoryProperties(s.physical,&s.memory);
  VkDescriptorSetLayoutBinding bindings[2]{};for(unsigned i=0;i<2;++i){bindings[i].binding=i;bindings[i].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;bindings[i].descriptorCount=1;bindings[i].stageFlags=VK_SHADER_STAGE_COMPUTE_BIT;}
  VkDescriptorSetLayoutCreateInfo layout{};layout.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;layout.bindingCount=2;layout.pBindings=bindings;check(s.CreateDescriptorSetLayout(s.device,&layout,nullptr,&s.descriptor_layout),"descriptor layout");
  VkPushConstantRange push{VK_SHADER_STAGE_COMPUTE_BIT,0,32};VkPipelineLayoutCreateInfo pl{};pl.sType=VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;pl.setLayoutCount=1;pl.pSetLayouts=&s.descriptor_layout;pl.pushConstantRangeCount=1;pl.pPushConstantRanges=&push;check(s.CreatePipelineLayout(s.device,&pl,nullptr,&s.pipeline_layout),"pipeline layout");
  VkShaderModule module=VK_NULL_HANDLE;VkShaderModuleCreateInfo shader{};shader.sType=VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;shader.codeSize=sizeof(arcade_effects_spv);shader.pCode=arcade_effects_spv;check(s.CreateShaderModule(s.device,&shader,nullptr,&module),"shader module");
  VkComputePipelineCreateInfo cp{};cp.sType=VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;cp.layout=s.pipeline_layout;cp.stage.sType=VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;cp.stage.stage=VK_SHADER_STAGE_COMPUTE_BIT;cp.stage.module=module;cp.stage.pName="main";auto result=s.CreateComputePipelines(s.device,VK_NULL_HANDLE,1,&cp,nullptr,&s.pipeline);s.DestroyShaderModule(s.device,module,nullptr);check(result,"compute pipeline");
  VkDescriptorPoolSize pool_size{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,2};VkDescriptorPoolCreateInfo pool{};pool.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;pool.maxSets=1;pool.poolSizeCount=1;pool.pPoolSizes=&pool_size;check(s.CreateDescriptorPool(s.device,&pool,nullptr,&s.descriptor_pool),"descriptor pool");VkDescriptorSetAllocateInfo allocate{};allocate.sType=VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;allocate.descriptorPool=s.descriptor_pool;allocate.descriptorSetCount=1;allocate.pSetLayouts=&s.descriptor_layout;check(s.AllocateDescriptorSets(s.device,&allocate,&s.descriptor),"descriptor set");
  VkCommandPoolCreateInfo commands{};commands.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;commands.queueFamilyIndex=family;commands.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;check(s.CreateCommandPool(s.device,&commands,nullptr,&s.command_pool),"command pool");VkCommandBufferAllocateInfo ca{};ca.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;ca.commandPool=s.command_pool;ca.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;ca.commandBufferCount=1;check(s.AllocateCommandBuffers(s.device,&ca,&s.command),"command buffer");VkFenceCreateInfo fence{};fence.sType=VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;check(s.CreateFence(s.device,&fence,nullptr,&s.fence),"fence");
  state_=std::move(state);return true;
 }catch(const std::exception& e){error=e.what();return false;}
}
bool VulkanProcessor::process(const Frame& frame,const Settings& prefs,Frame& output,std::string& error){
 if(!state_){error="Vulkan processor is not initialized";return false;}auto& s=*state_;
 try{
  unsigned width=s.target_width?s.target_width:frame.width,height=s.target_height?s.target_height:frame.height;std::size_t count=std::size_t(frame.width)*frame.height,output_count=std::size_t(width)*height;
  if(!frame.ready())throw std::runtime_error("MAME has not produced a video frame yet");
  if(frame.width>8192||frame.height>8192)throw std::runtime_error("MAME frame "+std::to_string(frame.width)+"x"+std::to_string(frame.height)+" exceeds the 8192x8192 input limit");
  if(width>3840||height>2160||output_count>3840u*2160u)throw std::runtime_error("Vulkan output "+std::to_string(width)+"x"+std::to_string(height)+" exceeds the 3840x2160 display limit");
  VkDeviceSize desired[2]={count*4,output_count*4};
  if(s.sizes[0]!=desired[0]||s.sizes[1]!=desired[1]){
   s.release_buffers();
   for(int i=0;i<2;++i){VkBufferCreateInfo buffer{};buffer.sType=VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;buffer.size=desired[i];buffer.usage=VK_BUFFER_USAGE_STORAGE_BUFFER_BIT|VK_BUFFER_USAGE_TRANSFER_SRC_BIT;buffer.sharingMode=VK_SHARING_MODE_EXCLUSIVE;check(s.CreateBuffer(s.device,&buffer,nullptr,&s.buffers[i]),"create image buffer");VkMemoryRequirements req{};s.GetBufferMemoryRequirements(s.device,s.buffers[i],&req);uint32_t type=s.memory.memoryTypeCount;for(uint32_t j=0;j<s.memory.memoryTypeCount;++j)if((req.memoryTypeBits&(1u<<j))&&(s.memory.memoryTypes[j].propertyFlags&(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT))==(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT|VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)){type=j;break;}if(type==s.memory.memoryTypeCount)throw std::runtime_error("No coherent Vulkan image memory available");VkMemoryAllocateInfo allocation{};allocation.sType=VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;allocation.allocationSize=req.size;allocation.memoryTypeIndex=type;check(s.AllocateMemory(s.device,&allocation,nullptr,&s.allocations[i]),"allocate image memory");check(s.BindBufferMemory(s.device,s.buffers[i],s.allocations[i],0),"bind image memory");check(s.MapMemory(s.device,s.allocations[i],0,desired[i],0,&s.mapped[i]),"map image memory");s.sizes[i]=desired[i];}
   VkDescriptorBufferInfo info[2]{};VkWriteDescriptorSet write[2]{};for(unsigned i=0;i<2;++i){info[i]={s.buffers[i],0,s.sizes[i]};write[i].sType=VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;write[i].dstSet=s.descriptor;write[i].dstBinding=i;write[i].descriptorCount=1;write[i].descriptorType=VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;write[i].pBufferInfo=&info[i];}s.UpdateDescriptorSets(s.device,2,write,0,nullptr);
  }
  std::memcpy(s.mapped[0],frame.pixels,desired[0]);check(s.ResetCommandPool(s.device,s.command_pool,0),"reset command pool");VkCommandBufferBeginInfo begin{};begin.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;check(s.BeginCommandBuffer(s.command,&begin),"begin commands");
  VkMemoryBarrier barrier{};barrier.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER;barrier.srcAccessMask=VK_ACCESS_HOST_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_SHADER_READ_BIT;s.CmdPipelineBarrier(s.command,VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,0,1,&barrier,0,nullptr,0,nullptr);
  s.CmdBindPipeline(s.command,VK_PIPELINE_BIND_POINT_COMPUTE,s.pipeline);s.CmdBindDescriptorSets(s.command,VK_PIPELINE_BIND_POINT_COMPUTE,s.pipeline_layout,0,1,&s.descriptor,0,nullptr);struct {uint32_t width,height,out_width,out_height;float aspect;uint32_t effect,flags,reserved;} options{frame.width,frame.height,width,height,frame.aspect,static_cast<unsigned>(std::clamp(prefs.post_processing,0,5)),(prefs.smooth?256u:0u)|(static_cast<unsigned>(prefs.rotation)<<10)|(static_cast<unsigned>(prefs.scaling)<<12),0};s.CmdPushConstants(s.command,s.pipeline_layout,VK_SHADER_STAGE_COMPUTE_BIT,0,32,&options);s.CmdDispatch(s.command,(width+7)/8,(height+7)/8,1);
  barrier.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT;barrier.dstAccessMask=VK_ACCESS_HOST_READ_BIT;s.CmdPipelineBarrier(s.command,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,VK_PIPELINE_STAGE_HOST_BIT,0,1,&barrier,0,nullptr,0,nullptr);check(s.EndCommandBuffer(s.command),"end commands");check(s.ResetFences(s.device,1,&s.fence),"reset fence");VkSubmitInfo submit{};submit.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;submit.commandBufferCount=1;submit.pCommandBuffers=&s.command;check(s.QueueSubmit(s.queue,1,&submit,s.fence),"submit processing");check(s.WaitForFences(s.device,1,&s.fence,VK_TRUE,UINT64_MAX),"wait for processing");
  if(s.swapchain){output={static_cast<const uint32_t*>(s.mapped[1]),width,height,float(width)/height};}else{s.pixels.resize(output_count);std::memcpy(s.pixels.data(),s.mapped[1],desired[1]);output={s.pixels.data(),width,height,frame.aspect};}return true;
 }catch(const std::exception& e){error=e.what();return false;}
}
bool VulkanProcessor::display_open()const{return state_&&state_->swapchain;}
void VulkanProcessor::close_display(){if(state_)state_->release_display();}
bool VulkanProcessor::open_display(const Settings& prefs,std::string& error){
 if(!state_){error="Vulkan is not initialized";return false;}auto& s=*state_;s.release_display();
#ifdef ARCADE_PS5
 setenv("EDEN_VIDEOOUT_120HZ",prefs.refresh?"1":"0",1);
#endif
 try{
#ifdef ARCADE_PS5
 auto get=radv_GetInstanceProcAddr;
#else
 auto get=vkGetInstanceProcAddr;
#endif
 s.GetPhysicalDeviceDisplayPropertiesKHR=reinterpret_cast<PFN_vkGetPhysicalDeviceDisplayPropertiesKHR>(get(s.instance,"vkGetPhysicalDeviceDisplayPropertiesKHR"));if(!s.GetPhysicalDeviceDisplayPropertiesKHR)throw std::runtime_error("Missing Vulkan GetPhysicalDeviceDisplayPropertiesKHR");
 s.GetDisplayModePropertiesKHR=reinterpret_cast<PFN_vkGetDisplayModePropertiesKHR>(get(s.instance,"vkGetDisplayModePropertiesKHR"));if(!s.GetDisplayModePropertiesKHR)throw std::runtime_error("Missing Vulkan GetDisplayModePropertiesKHR");
 s.CreateDisplayPlaneSurfaceKHR=reinterpret_cast<PFN_vkCreateDisplayPlaneSurfaceKHR>(get(s.instance,"vkCreateDisplayPlaneSurfaceKHR"));if(!s.CreateDisplayPlaneSurfaceKHR)throw std::runtime_error("Missing Vulkan CreateDisplayPlaneSurfaceKHR");
 s.GetPhysicalDeviceSurfaceSupportKHR=reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceSupportKHR>(get(s.instance,"vkGetPhysicalDeviceSurfaceSupportKHR"));if(!s.GetPhysicalDeviceSurfaceSupportKHR)throw std::runtime_error("Missing Vulkan GetPhysicalDeviceSurfaceSupportKHR");
 s.GetPhysicalDeviceSurfaceCapabilitiesKHR=reinterpret_cast<PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR>(get(s.instance,"vkGetPhysicalDeviceSurfaceCapabilitiesKHR"));if(!s.GetPhysicalDeviceSurfaceCapabilitiesKHR)throw std::runtime_error("Missing Vulkan GetPhysicalDeviceSurfaceCapabilitiesKHR");
 s.DestroySurfaceKHR=reinterpret_cast<PFN_vkDestroySurfaceKHR>(get(s.instance,"vkDestroySurfaceKHR"));if(!s.DestroySurfaceKHR)throw std::runtime_error("Missing Vulkan DestroySurfaceKHR");
 s.CreateSwapchainKHR=reinterpret_cast<PFN_vkCreateSwapchainKHR>(s.GetDeviceProcAddr(s.device,"vkCreateSwapchainKHR"));if(!s.CreateSwapchainKHR)throw std::runtime_error("Missing Vulkan CreateSwapchainKHR");
 s.DestroySwapchainKHR=reinterpret_cast<PFN_vkDestroySwapchainKHR>(s.GetDeviceProcAddr(s.device,"vkDestroySwapchainKHR"));if(!s.DestroySwapchainKHR)throw std::runtime_error("Missing Vulkan DestroySwapchainKHR");
 s.GetSwapchainImagesKHR=reinterpret_cast<PFN_vkGetSwapchainImagesKHR>(s.GetDeviceProcAddr(s.device,"vkGetSwapchainImagesKHR"));if(!s.GetSwapchainImagesKHR)throw std::runtime_error("Missing Vulkan GetSwapchainImagesKHR");
 s.AcquireNextImageKHR=reinterpret_cast<PFN_vkAcquireNextImageKHR>(s.GetDeviceProcAddr(s.device,"vkAcquireNextImageKHR"));if(!s.AcquireNextImageKHR)throw std::runtime_error("Missing Vulkan AcquireNextImageKHR");
 s.QueuePresentKHR=reinterpret_cast<PFN_vkQueuePresentKHR>(s.GetDeviceProcAddr(s.device,"vkQueuePresentKHR"));if(!s.QueuePresentKHR)throw std::runtime_error("Missing Vulkan QueuePresentKHR");
 s.CreateSemaphore=reinterpret_cast<PFN_vkCreateSemaphore>(s.GetDeviceProcAddr(s.device,"vkCreateSemaphore"));if(!s.CreateSemaphore)throw std::runtime_error("Missing Vulkan CreateSemaphore");
 s.DestroySemaphore=reinterpret_cast<PFN_vkDestroySemaphore>(s.GetDeviceProcAddr(s.device,"vkDestroySemaphore"));if(!s.DestroySemaphore)throw std::runtime_error("Missing Vulkan DestroySemaphore");
 s.CmdCopyBufferToImage=reinterpret_cast<PFN_vkCmdCopyBufferToImage>(s.GetDeviceProcAddr(s.device,"vkCmdCopyBufferToImage"));if(!s.CmdCopyBufferToImage)throw std::runtime_error("Missing Vulkan CmdCopyBufferToImage");

 uint32_t count=0;check(s.GetPhysicalDeviceDisplayPropertiesKHR(s.physical,&count,nullptr),"display count");if(!count)throw std::runtime_error("No native Vulkan display");std::vector<VkDisplayPropertiesKHR> displays(count);check(s.GetPhysicalDeviceDisplayPropertiesKHR(s.physical,&count,displays.data()),"display properties");
 check(s.GetDisplayModePropertiesKHR(s.physical,displays.front().display,&count,nullptr),"display mode count");std::vector<VkDisplayModePropertiesKHR> modes(count);check(s.GetDisplayModePropertiesKHR(s.physical,displays.front().display,&count,modes.data()),"display modes");if(modes.empty())throw std::runtime_error("No native Vulkan display mode");
 const unsigned widths[]={1920,2560,3840},heights[]={1080,1440,2160};unsigned selected=0;for(unsigned i=0;i<modes.size();++i){auto p=modes[i].parameters;if(p.visibleRegion.width==widths[prefs.output]&&p.visibleRegion.height==heights[prefs.output]){selected=i;if(prefs.refresh?(p.refreshRate>90000):(p.refreshRate<90000))break;}}
 auto mode=modes[selected];VkDisplaySurfaceCreateInfoKHR surface{};surface.sType=VK_STRUCTURE_TYPE_DISPLAY_SURFACE_CREATE_INFO_KHR;surface.displayMode=mode.displayMode;surface.transform=VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;surface.alphaMode=VK_DISPLAY_PLANE_ALPHA_OPAQUE_BIT_KHR;surface.imageExtent=mode.parameters.visibleRegion;check(s.CreateDisplayPlaneSurfaceKHR(s.instance,&surface,nullptr,&s.surface),"create native display surface");
 VkBool32 supported=false;check(s.GetPhysicalDeviceSurfaceSupportKHR(s.physical,s.queue_family,s.surface,&supported),"queue display support");if(!supported)throw std::runtime_error("Vulkan queue cannot present to native display");VkSurfaceCapabilitiesKHR caps{};check(s.GetPhysicalDeviceSurfaceCapabilitiesKHR(s.physical,s.surface,&caps),"display capabilities");if(!(caps.supportedUsageFlags&VK_IMAGE_USAGE_TRANSFER_DST_BIT))throw std::runtime_error("Native Vulkan display does not accept transferred game frames");
 VkExtent2D extent=caps.currentExtent;if(extent.width==UINT32_MAX)extent=mode.parameters.visibleRegion;
 VkSwapchainCreateInfoKHR swap{};swap.sType=VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;swap.surface=s.surface;swap.minImageCount=std::max(3u,caps.minImageCount);if(caps.maxImageCount)swap.minImageCount=std::min(swap.minImageCount,caps.maxImageCount);swap.imageFormat=VK_FORMAT_B8G8R8A8_UNORM;swap.imageColorSpace=VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;swap.imageExtent=extent;swap.imageArrayLayers=1;swap.imageUsage=VK_IMAGE_USAGE_TRANSFER_DST_BIT;swap.imageSharingMode=VK_SHARING_MODE_EXCLUSIVE;swap.preTransform=VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;swap.compositeAlpha=VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;swap.presentMode=VK_PRESENT_MODE_FIFO_KHR;swap.clipped=VK_TRUE;check(s.CreateSwapchainKHR(s.device,&swap,nullptr,&s.swapchain),"create native swapchain");
 check(s.GetSwapchainImagesKHR(s.device,s.swapchain,&count,nullptr),"swapchain image count");s.images.resize(count);s.presented.assign(count,false);check(s.GetSwapchainImagesKHR(s.device,s.swapchain,&count,s.images.data()),"swapchain images");VkSemaphoreCreateInfo sem{};sem.sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;check(s.CreateSemaphore(s.device,&sem,nullptr,&s.acquired),"acquire semaphore");s.rendered.resize(count,VK_NULL_HANDLE);for(auto& semaphore:s.rendered)check(s.CreateSemaphore(s.device,&sem,nullptr,&semaphore),"render semaphore");s.target_width=extent.width;s.target_height=extent.height;return true;
 }catch(const std::exception& e){error=e.what();s.release_display();return false;}
}
void VulkanProcessor::overlay(const std::string& text,const Settings& prefs){
 if(!state_||!state_->mapped[1]||state_->hud_font.empty()||text.empty())return;auto& s=*state_;auto* pixels=static_cast<uint32_t*>(s.mapped[1]);int w=s.target_width,h=s.target_height;if(!w||!h)return;
 float scale=w/1672.f,size=(20+prefs.hud_size*4)*scale;float width=0;for(unsigned char c:text)width+=glyph_widths[1][(c>=32&&c<128?c:'?')-32]*size/48;
 int margin=int(32*scale),padding=int(16*scale),boxw=std::min(w-margin*2,int(width)+padding*2),boxh=int(size)+padding*2;int x=prefs.hud_corner%2?w-margin-boxw:margin,y=prefs.hud_corner>=2?h-margin-boxh:margin;
 auto blend=[&](int px,int py,unsigned color,unsigned alpha){if(px<0||px>=w||py<0||py>=h)return;auto& dst=pixels[py*w+px];unsigned red=(((dst>>16)&255)*(255-alpha)+((color>>16)&255)*alpha+127)/255;unsigned green=(((dst>>8)&255)*(255-alpha)+((color>>8)&255)*alpha+127)/255;unsigned blue=((dst&255)*(255-alpha)+(color&255)*alpha+127)/255;dst=0xff000000|(red<<16)|(green<<8)|blue;};
 for(int py=y;py<y+boxh;++py)for(int px=x;px<x+boxw;++px)blend(px,py,0x10151e,235);
 unsigned ink=prefs.hud_colors?accent_color(prefs.accent):0xffffff;float cursor=x+padding;int top=y+padding-int(11*size/48);
 for(unsigned char c:text){int glyph=(c>=32&&c<128?c:'?')-32;int gw=std::max(1,int(64*size/48)),gh=std::max(1,int(80*size/48));for(int py=0;py<gh;++py)for(int px=0;px<gw;++px){int dx=int(cursor)+px;if(dx>=x+boxw-padding)continue;int sx=(glyph%16)*64+std::min(63,px*64/gw),sy=(glyph/16)*80+std::min(79,py*80/gh);unsigned alpha=s.hud_font[(sy*1024+sx)*4+3];if(alpha)blend(dx,top+py,ink,alpha);}cursor+=glyph_widths[1][glyph]*size/48;}
}
bool VulkanProcessor::present(std::string& error){
 if(!state_||!state_->swapchain){error="Native Vulkan display is not open";return false;}auto& s=*state_;
 try{
 uint32_t index=0;auto result=s.AcquireNextImageKHR(s.device,s.swapchain,UINT64_MAX,s.acquired,VK_NULL_HANDLE,&index);if(result!=VK_SUBOPTIMAL_KHR)check(result,"acquire native image");
 check(s.ResetCommandPool(s.device,s.command_pool,0),"reset presentation commands");VkCommandBufferBeginInfo begin{};begin.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;begin.flags=VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;check(s.BeginCommandBuffer(s.command,&begin),"begin presentation");
 VkMemoryBarrier memory{};memory.sType=VK_STRUCTURE_TYPE_MEMORY_BARRIER;memory.srcAccessMask=VK_ACCESS_SHADER_WRITE_BIT|VK_ACCESS_HOST_WRITE_BIT;memory.dstAccessMask=VK_ACCESS_TRANSFER_READ_BIT;s.CmdPipelineBarrier(s.command,VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT|VK_PIPELINE_STAGE_HOST_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,1,&memory,0,nullptr,0,nullptr);
 VkImageMemoryBarrier image{};image.sType=VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;image.srcAccessMask=0;image.dstAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;image.oldLayout=s.presented[index]?VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:VK_IMAGE_LAYOUT_UNDEFINED;image.newLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;image.srcQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;image.dstQueueFamilyIndex=VK_QUEUE_FAMILY_IGNORED;image.image=s.images[index];image.subresourceRange={VK_IMAGE_ASPECT_COLOR_BIT,0,1,0,1};s.CmdPipelineBarrier(s.command,VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,VK_PIPELINE_STAGE_TRANSFER_BIT,0,0,nullptr,0,nullptr,1,&image);
 VkBufferImageCopy region{};region.imageSubresource={VK_IMAGE_ASPECT_COLOR_BIT,0,0,1};region.imageExtent={s.target_width,s.target_height,1};s.CmdCopyBufferToImage(s.command,s.buffers[1],s.images[index],VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,1,&region);
 image.srcAccessMask=VK_ACCESS_TRANSFER_WRITE_BIT;image.dstAccessMask=0;image.oldLayout=VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;image.newLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;s.CmdPipelineBarrier(s.command,VK_PIPELINE_STAGE_TRANSFER_BIT,VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,0,0,nullptr,0,nullptr,1,&image);check(s.EndCommandBuffer(s.command),"end presentation");
 VkPipelineStageFlags wait=VK_PIPELINE_STAGE_TRANSFER_BIT;VkSubmitInfo submit{};submit.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;submit.waitSemaphoreCount=1;submit.pWaitSemaphores=&s.acquired;submit.pWaitDstStageMask=&wait;submit.commandBufferCount=1;submit.pCommandBuffers=&s.command;submit.signalSemaphoreCount=1;submit.pSignalSemaphores=&s.rendered[index];check(s.ResetFences(s.device,1,&s.fence),"reset presentation fence");check(s.QueueSubmit(s.queue,1,&submit,s.fence),"submit presentation");VkPresentInfoKHR present{};present.sType=VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;present.waitSemaphoreCount=1;present.pWaitSemaphores=&s.rendered[index];present.swapchainCount=1;present.pSwapchains=&s.swapchain;present.pImageIndices=&index;result=s.QueuePresentKHR(s.queue,&present);if(result!=VK_SUBOPTIMAL_KHR)check(result,"native presentation");check(s.WaitForFences(s.device,1,&s.fence,VK_TRUE,UINT64_MAX),"wait presentation fence");s.presented[index]=true;return true;
 }catch(const std::exception& e){error=e.what();return false;}
}

}
