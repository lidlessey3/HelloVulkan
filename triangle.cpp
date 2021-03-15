#include "triangle.h"
#include <GLFW/glfw3.h>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <map>
#include <math.h>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.h>

void HelloTriangleApplication::run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
}

void HelloTriangleApplication::initWindow() {
    framebufferResized = false;    // starting value of this variable
    // Inizializzo GLFW
    glfwInit();
    // Devo dirli di non aprire cose con opengl perchè userò vulkan
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    // Disabilito il rezing delle finestre perchè posso
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); removed since I should now be able to handle it

    // Inizializzo la variabile window con una dimensione di 800*600 in modalità finestra (il primo nullptr) e non ho capito che fa il secondo nullprt
    window = glfwCreateWindow(800, 600, "Vulkan Triangle hopefully", nullptr, nullptr);

    glfwSetWindowUserPointer(window, this);                               // I attach this instance to the window
    glfwSetFramebufferSizeCallback(window, framebufferResizeCallBack);    // and I set the callback function
}

void HelloTriangleApplication::framebufferResizeCallBack(GLFWwindow *window, int width, int height) {
    // std::cout << "Called the callback!" << std::endl;
    auto app                = reinterpret_cast<HelloTriangleApplication *>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
}

void HelloTriangleApplication::initVulkan() {
    createInstance();
    // Skipped adding debug functionality cause who cares :)
    // also without them its a lot faster ^w^
    createSurface();                // creates the surface I'll use to draw stuff
    pickGraphicsCard();             // I select the right graphic card
    createLogicalDevice();          // I create the logical device
    createSwapChain();              // I create the swapchain, finally
    createImageViews();             // I create the image views even though I have no idea the fuck are they for
    createRenderPass();             // I add this before closing the pipeline
    createDescriptorSetLayout();    // sets the bindign for the shader
    createGraphicsPipeline();       // I create my death :)
    createFrameBuffer();            // I create the frame buffer
    createCommandPool();            // allocate the command pool to be able to create command buffers for later
    createVertexBuffer();           // create vertex buffer
    createIndexBuffer();            // creates the index buffer
    createUniformBuffers();         // creates the uniform buffers
    createDescriptorPool();         // and the relative descriptor pool
    createDescriptorSets();         // now I can create the sets
    createCommandBuffers();         // allocate command buffers
    createSemaphores();             // create the semaphores
    currentFrame = 0;               // set the starting value
};

void HelloTriangleApplication::recreateSwapChain() {
    // handles minimization
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);    // get the dims
    while (width == 0 || height == 0) {                 // if they are 0the window is minimized, so I wait
        glfwWaitEvents();
        glfwGetFramebufferSize(window, &width, &height);
    }

    vkDeviceWaitIdle(device);

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFrameBuffer();
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();
    createCommandBuffers();
}

void HelloTriangleApplication::mainLoop() {
    // beginTime = new std::chrono::steady_clock::time_point(std::chrono::steady_clock::now());    // I start the clock
    while (!glfwWindowShouldClose(this->window)) {    // fintanto che non chiudo la finestra
        // updatePos();                                                                            // it goes here
        glfwPollEvents();    // ottiene eventi
        drawFrame();         // disegno un frame
    }

    vkDeviceWaitIdle(device);    // wait for the device to finish whatever it is doing
}

void HelloTriangleApplication::cleanup() {
    cleanupSwapChain();    // cleans up swapchian

    vkDestroyDescriptorSetLayout(device, descriptorLayout, nullptr);    // destroying the set layout

    vkDestroyBuffer(device, vertexBuffer, nullptr);       // I destroy the vertex buffer
    vkDestroyBuffer(device, indexBuffer, nullptr);        // I destroy the index buffer
    vkFreeMemory(device, vertexBufferMemory, nullptr);    // I free the memory after I destroyed the buffer NOTE: it is mandatory it be done in this order
    vkFreeMemory(device, indexBufferMemory, nullptr);     // I free the memory of the index buffer

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, imageAvailableSemaphore[i], nullptr);    // deleting the semaphores
        vkDestroySemaphore(device, renderFinishedSemaphore[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);    // and the fences
    }

    vkDestroyCommandPool(device, commandPool, nullptr);            // selfexplanatory, not as much as I thought as it also destroys the command buffers
    vkDestroyCommandPool(device, transferCommandPool, nullptr);    // I also destroy this comand pool

    vkDestroyDevice(device, nullptr);    // distruggo il device virtual che ho creato

    vkDestroySurfaceKHR(instance, surface, nullptr);    // distruggo òa superficie

    vkDestroyInstance(instance, nullptr);    // distruggo l'handle di Vulkan

    glfwDestroyWindow(window);    // Distruggo la finestra

    glfwTerminate();    // termino l'istanza grafica
}

void HelloTriangleApplication::cleanupSwapChain() {
    for (auto framebuffer : swapChainFrameBuffer)    // destroy all the frame buffers
        vkDestroyFramebuffer(device, framebuffer, nullptr);

    vkFreeCommandBuffers(device, commandPool, static_cast<uint64_t>(commandBuffers.size()), commandBuffers.data());

    vkDestroyPipeline(device, graphicsPipeline, nullptr);    // destroy the pipeline

    vkDestroyPipelineLayout(device, pipelineLayout, nullptr);    // I destroy the pipeline layout
    vkDestroyRenderPass(device, renderPass, nullptr);            // destroy the render pass

    for (auto imageView : swapChainImageViews) {    // I destroy all of the image views
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroyDescriptorPool(device, descriptorPool, nullptr);    // destroy the descriptor pool

    vkDestroySwapchainKHR(device, swapChain, nullptr);    // destroy the swap chain

    for (int i = 0; i < swapChainImages.size(); i++) {
        vkDestroyBuffer(device, uniformBuffers[i], nullptr);       // destroy the uniform buffer
        vkFreeMemory(device, uniformBuffersMemory[i], nullptr);    // free the memory
    }
}

void HelloTriangleApplication::createInstance() {
    VkApplicationInfo appInfo {};                                       // creo una struct VkApplicationInfo che conterra dati OPZIONALI per vulkan
                                                                        // li fornisco perchè vulkan potrebbe essere utile per ottimizzare qualcosa
    appInfo.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;    // Dichiaro il tipo di questa struct
    appInfo.pApplicationName   = "Vulkan Triangle hopefully";
    appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
    appInfo.pEngineName        = "No Engine";
    appInfo.engineVersion      = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion         = VK_API_VERSION_1_0;
    appInfo.pNext              = nullptr;

    // Creo la struct InstanceCreateInfo, questa è necessaria per ottenere un handle valido
    VkInstanceCreateInfo instanceInfo {};
    instanceInfo.sType            = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;

    uint32_t glfwExtensionCount = 0;                                            // numero di estensioni richieste
    const char **glfwExtensions;                                                // quali estensioni sono necessarie
    glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);    // compilo questi due campi

    instanceInfo.enabledExtensionCount   = glfwExtensionCount;
    instanceInfo.ppEnabledExtensionNames = glfwExtensions;
    instanceInfo.enabledLayerCount       = 0;    // numero di layer abilitati, ancora non so che sono però, rip

    if (vkCreateInstance(&instanceInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("Unable to create Vulkan instance. REEEEEEEEEEEEEE!!!");
    }
}

void HelloTriangleApplication::pickGraphicsCard() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);    // Check the number of avaliable GPUs
    if (deviceCount == 0) {
        throw std::runtime_error("No valid GPU. Cry :(");
    } else {
        std::cout << "Devices found = " << deviceCount << std::endl;
    }
    // now I allocate the vector to contain all found devices
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());    // store all avaliable GPUs inside the vector
    std::multimap<int, VkPhysicalDevice> candidates;
    // int i = 0;
    for (const auto &device : devices) {    // I select the first good enough card
        // if(i != 0 && i != 2) {
        int score = rateDevice(device);
        candidates.insert(std::make_pair(score, device));
        //}
        // i++;
    }

    if (candidates.rbegin()->first > 0) {
        physicalDevice = candidates.rbegin()->second;
    }

    if (physicalDevice == VK_NULL_HANDLE)
        throw std::runtime_error("GPUs were found, but none was good enough.");

    getQueueFamilies();
}

int HelloTriangleApplication::rateDevice(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    // obtain properties and features
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    // check for swap chain compatibility it is required

    // Require that it has a geometry shader or it does not have a valid graphic family queue and also that it has swapchain support
    if (!deviceFeatures.geometryShader || !getQueueFamilies(device).isComplete() || !checkDeviceExtentionSupport(device))
        return 0;

    bool swapChainOK                         = false;
    SwapChainSupportDetails swapChainSupport = this->querySwapChainSupport(device);
    swapChainOK                              = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
    if (!swapChainOK)
        return 0;

    int score = 0;    // The score associated with this device

    score += deviceProperties.limits.maxImageDimension2D;      // max texture
    score += deviceProperties.limits.maxImageDimensionCube;    // don't know, but seams good :)

    // bonus if it is a discrete GPU
    if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
        score += 1000;
        std::cout << "there is at least one device which is a discrete GPU!!" << std::endl;
    }

    return score;
}

bool HelloTriangleApplication::checkDeviceExtentionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());    // Gets all the available extensions from the device

    std::set<std::string> requiredExtensions(deviceRequiredExtensions.begin(), deviceRequiredExtensions.end());

    for (const auto &extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

void HelloTriangleApplication::getQueueFamilies() {
    QueueFamilyIndices indices = this->getQueueFamilies(this->physicalDevice);
    if (indices.graphicsFamily.has_value())
        this->indices.graphicsFamily = indices.graphicsFamily;
    if (indices.presentFamily.has_value())
        this->indices.presentFamily = indices.presentFamily;
    if (indices.transferFamily.has_value())
        this->indices.transferFamily = indices.presentFamily;
}

HelloTriangleApplication::QueueFamilyIndices HelloTriangleApplication::getQueueFamilies(VkPhysicalDevice physicalDevice) {
    QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);    // Get the number of avaliable queues
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());    // obtain all tha avaliable queues in my vector

    int i = 0;
    for (const auto &queue : queueFamilies) {
        if (queue.queueFlags & VK_QUEUE_GRAPHICS_BIT)    // check for transfer bit
            indices.graphicsFamily = i;
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);    // check for present support
        if (presentSupport)
            indices.presentFamily = i;
        if (queue.queueFlags & VK_QUEUE_TRANSFER_BIT && !(queue.queueFlags & VK_QUEUE_GRAPHICS_BIT))    // check for  the transfer bit, and not graphic bit
            indices.transferFamily = i;
        i++;
    }
    return indices;
}

void HelloTriangleApplication::createLogicalDevice() {
    // this is the code to allow for the creation of multiple queues.

    std::vector<VkDeviceQueueCreateInfo> QueueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value(),
                                               indices.transferFamily.value() };    // this is to select only the unique values among the twos
                                                                                    // now there are three of them
    float queuePriority = 1.0f;                                                     // dummy value

    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo {};
        queueCreateInfo.sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount       = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        QueueCreateInfos.push_back(queueCreateInfo);    // add the newly create struct to the vector
    }

    VkPhysicalDeviceFeatures deviceFeatures {};    // the features I'm gonna use, in this case none, so I live it all to 0
    VkDeviceCreateInfo createInfo {};              // the info about creation of this device
    createInfo.sType                = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos    = QueueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(QueueCreateInfos.size());

    createInfo.pEnabledFeatures = &deviceFeatures;

    // I enter the required extensions
    createInfo.enabledExtensionCount   = static_cast<uint32_t>(deviceRequiredExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceRequiredExtensions.data();

    // deprecated, must be set to 0
    createInfo.enabledLayerCount = 0;

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
        throw std::runtime_error("Unable to create virtual device");

    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);    // I get the handle for the queue that I need
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);      // and the handle for the othe queue
    vkGetDeviceQueue(device, indices.transferFamily.value(), 0, &transferQueue);    // and the trasfer one
}

void HelloTriangleApplication::createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
        throw std::runtime_error("Unable to create a surface");
}

HelloTriangleApplication::SwapChainSupportDetails HelloTriangleApplication::querySwapChainSupport(VkPhysicalDevice device) {
    SwapChainSupportDetails details {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &(details.capabilities));    // I query for the capabilities of the given device

    // I query for the remaining two details in the standard way
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if (formatCount != 0) {
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }

    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    if (presentModeCount != 0) {
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }
    return details;
}

VkSurfaceFormatKHR HelloTriangleApplication::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
    for (const auto &availableFormat : availableFormats) {    // I search for an SRGB format, since it it the best
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            return availableFormat;
    }

    return availableFormats[0];    // if I don't get what I want I pick the first, too smart I know
}

VkPresentModeKHR HelloTriangleApplication::chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
    for (const auto &presentMode : availablePresentModes) {    // I look for triplebuffering, cause it allows to show most recent immage avaliable when the screen refreshes
        if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)        // without tearing
            return presentMode;
    }
    return VK_PRESENT_MODE_FIFO_KHR;    // if that fails I go for normal v-sync, since it is garanteed to exist ^w^
}

VkExtent2D HelloTriangleApplication::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
    if (capabilities.currentExtent.width != UINT32_MAX)    // if the width is set to UINT32_MAX then the window manager will do the rest
        return capabilities.currentExtent;
    else {    // if that is not the case I have to match the resolution to that of the window by hand
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        VkExtent2D actualExtent = { static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
        actualExtent.width      = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
        actualExtent.height     = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));
        return actualExtent;
    }
}

void HelloTriangleApplication::createSwapChain() {
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);    // get all the information

    // Select the best options
    VkSurfaceFormatKHR surfaceFormat    = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR surfacePresentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D surfaceExtent            = chooseSwapExtent(swapChainSupport.capabilities);

    // length of this chain (set it to the minimum necessary for it to function)
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;    // the plus one is recommended as so we don't have to wait for the driver
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
        imageCount = swapChainSupport.capabilities.maxImageCount;    // making sure not to excide the maximum allowed (0 means no maximum)

    // declaring the struct to compile in order to create the swap chain
    VkSwapchainCreateInfoKHR createInfo {};
    createInfo.sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface          = surface;
    createInfo.minImageCount    = imageCount;
    createInfo.imageFormat      = surfaceFormat.format;
    createInfo.imageColorSpace  = surfaceFormat.colorSpace;
    createInfo.imageExtent      = surfaceExtent;
    createInfo.imageArrayLayers = 1;                                      // number of layers in the image, usually it is always 1 unless you do some strange shit
    createInfo.imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;    // render directly to them, no post-processing

    // I get the indicies
    // maybe not necessary to modify here since it has nothing to do with the third queue
    uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };    // in caso devo venire a piangere qua

    if (queueFamilyIndices[0] != queueFamilyIndices[1]) {    // check if they are the same or not and act accordingly
        createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;    // now changed to 3
        createInfo.pQueueFamilyIndices   = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;    // better for performance, would require more work if there was more than one queue
        createInfo.queueFamilyIndexCount = 0;                            // optimal
        createInfo.pQueueFamilyIndices   = nullptr;                      // optimal
    }
    createInfo.preTransform   = swapChainSupport.capabilities.currentTransform;    // I do not want any transformation to be done
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;                 // Ignore the alpha channel as I do not want the window to be transparent

    createInfo.presentMode = surfacePresentMode;
    createInfo.clipped     = VK_TRUE;    // I do not care about obscured pixel (i.e. there is another window in front of them)

    createInfo.oldSwapchain = VK_NULL_HANDLE;    // set the previous chain to null (in case I resize the window)

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS)
        throw std::runtime_error("Unable to create swapchain. PD");

    // thing to retrive the handle of the Vk images
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    // I store the format and the extent in class variables

    this->swapChainExtent = surfaceExtent;
    this->swapChainFormat = surfaceFormat.format;
}

void HelloTriangleApplication::createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());

    // I loop over the images
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo {};
        createInfo.sType    = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        createInfo.image    = swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;    // I select how the image should be taken, in this case 2D texture.
        createInfo.format   = swapChainFormat;          // the format used to create the swap chain

        // mapping of the color channels in this case I leave them all to the default
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        // what is the raison d'etrè of this image?
        // color targets with no mipmapping and no multiple layers, in this case
        createInfo.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel   = 0;
        createInfo.subresourceRange.levelCount     = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount     = 1;
        // NOTE: in case of stereoscopic 3D image (VR) I would create two image views, one per eye

        if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS)
            throw std::runtime_error("Unable to create images views! I DO NOT WANT TO GO ON LIVING!!");
    }
}

void HelloTriangleApplication::createGraphicsPipeline() {    // NOTE: I fear the shit that I am reading rn, A LOT!!!
    auto vertShaderCode   = readFile("../shaders/vert.spv");
    auto fragShaderCode   = readFile("../shaders/frag.spv");
    auto vertShaderModule = createShaderModule(vertShaderCode);
    auto fragShaderModule = createShaderModule(fragShaderCode);

    // data to insert the shader into the pipeline
    VkPipelineShaderStageCreateInfo vertShaderStageInfo {};
    vertShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage  = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName  = "main";    // name of the starting function to invoke

    // copypaste for the frag shader
    VkPipelineShaderStageCreateInfo fragShaderStageInfo {};
    fragShaderStageInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName  = "main";    // name of the starting function to invoke

    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    // now configuring the fixed stages of the pipeline

    VkPipelineVertexInputStateCreateInfo vertexInputInfo {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    // the following is to indicate the inputs of the vertex
    auto bindingDescriptions   = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount   = 1;
    vertexInputInfo.pVertexBindingDescriptions      = &bindingDescriptions;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions    = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly {};
    inputAssembly.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;    // Every 3 vertex represent a triangle
    // inputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;    // I reuse the second and third verticies with the forth to create a tringle
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport {};    // it specifies the region of the frame buffer the ouput will be rendered to
    // in this case, as in the vast majority of case it will be as big as the window we are drawing to is
    viewport.x      = 0.0f;
    viewport.y      = 0.0f;
    viewport.width  = swapChainExtent.width;
    viewport.height = swapChainExtent.height;
    // the range of values for the frame buffer, in this case I set them to the default values
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 0.0f;

    // the scissor specify which area of the frame buffer is to be displayed, in this case one as big as the viewport
    VkRect2D scissor {};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState {};    // now I comnine the 2 previous structs into a single element
    viewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.scissorCount  = 1;
    viewportState.pScissors     = &scissor;
    viewportState.viewportCount = 1;
    viewportState.pViewports    = &viewport;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer {};
    rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthBiasClamp          = VK_FALSE;                   // to do this a particular GPU feature needs to be enabled, it was not
    rasterizer.rasterizerDiscardEnable = VK_FALSE;                   // if set to true nothing gets outputed from the rasterizer
    rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;       // fill polygons with fragments
    rasterizer.lineWidth               = 1.0f;                       // the number of fragments per line, a bigger number requires to enable a GPU feature
    rasterizer.cullMode                = VK_CULL_MODE_BACK_BIT;      // type of cull, what a cull is though is unkown
    rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;    // the order of verticies to consider a face front facing or not
    // rasterizer.frontFace       = VK_FRONT_FACE_COUNTER_CLOCKWISE;   // I change this because I do a Y - flip and so the order of the verticies gets reversed
    rasterizer.depthBiasEnable = VK_FALSE;    // used to alter the depth values

    // multisapling, aka one of the ways to perform anti-aliasing, it is disabled though as it needs a GPU feature not enable at the moment
    VkPipelineMultisampleStateCreateInfo multisampling {};
    multisampling.sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable   = VK_FALSE;
    multisampling.rasterizationSamples  = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading      = 1.0f;        // Optional
    multisampling.pSampleMask           = nullptr;     // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE;    // Optional
    multisampling.alphaToOneEnable      = VK_FALSE;    // Optional

    // blending it with the color already inside the frame buffer, in this case mixing the old and the new.
    VkPipelineColorBlendAttachmentState colorBlendAttachment {};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable    = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending {};    // it requires these two structures
    colorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable   = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments    = &colorBlendAttachment;

    // pipeline layout declares some paramenters that can be modified at drwing time, but in this case we will create and empty struct
    VkPipelineLayoutCreateInfo pipelineLayoutInfo {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    // the rest of the struct is left with its starting values of 0s as we said we will not use this right now

    // modifications in order to add the descriptor set layout
    pipelineLayoutInfo.setLayoutCount = 1;                    // how many we are gonna have
    pipelineLayoutInfo.pSetLayouts    = &descriptorLayout;    // array of descriptor

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS)
        throw std::runtime_error("Failed to create pipeline layout.");

    VkGraphicsPipelineCreateInfo pipelineInfo {};
    pipelineInfo.sType      = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages    = shaderStages;    // pointer to the stagers loaded intomemory

    // loading every other struct
    pipelineInfo.pVertexInputState   = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState      = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState   = &multisampling;
    pipelineInfo.pDepthStencilState  = nullptr;    // Optional
    pipelineInfo.pColorBlendState    = &colorBlending;
    pipelineInfo.pDynamicState       = nullptr;    // Optional

    // loading the layout
    pipelineInfo.layout = pipelineLayout;
    // the render pass
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass    = 0;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(device, vertShaderModule, nullptr);
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
}

VkShaderModule HelloTriangleApplication::createShaderModule(const std::vector<char> &code) {
    // Create the struct and configure it. the following is pretty self explanatory
    VkShaderModuleCreateInfo createInfo {};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode    = reinterpret_cast<const uint32_t *>(code.data());
    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
        throw std::runtime_error("Unable to create shader module");
    return shaderModule;
}

void HelloTriangleApplication::createRenderPass() {
    // we will only need one attachment in this case
    VkAttachmentDescription colorAttachment {};
    colorAttachment.format  = swapChainFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;    // no multisampling so one sample
    // what to do with the data attachment before rendering and after rendering
    colorAttachment.loadOp  = VK_ATTACHMENT_LOAD_OP_CLEAR;     // clear the values to a constant at the beginning
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;    // rendered contents will be stored to memory since we want to see them on screen

    colorAttachment.stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;    // I don't care about these two as we don't use them :)
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;          // I don0t know what I get at the begginning
    colorAttachment.finalLayout   = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;    // will be presented on the swap chain

    // a single render pass can have multiple subpass
    VkAttachmentReference colorAttachmentRef {};
    colorAttachmentRef.attachment = 0;                                           // the index in the array of attachment descriptions and the location = 0 in the shaders
    colorAttachmentRef.layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;    // this gives us the best performance possible

    // now defing the subpasses
    VkSubpassDescription subpass {};
    subpass.pipelineBindPoint    = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments    = &colorAttachmentRef;

    // struct to crete the render pass
    VkRenderPassCreateInfo renderPassInfo {};
    renderPassInfo.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments    = &colorAttachment;
    renderPassInfo.subpassCount    = 1;
    renderPassInfo.pSubpasses      = &subpass;

    // configure subpasses so as to work correctly cause they are bitches
    // we make them wait until I take the image
    VkSubpassDependency dependacy {};
    dependacy.srcSubpass    = VK_SUBPASS_EXTERNAL;                              // previous pass, the system one
    dependacy.dstSubpass    = 0;                                                // the index of my pass
    dependacy.srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;    // what to wait for
    dependacy.srcAccessMask = 0;                                                // in which stage it occurs
    dependacy.dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;    // prevent transition from happening until allowed
    dependacy.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

    // append to the render pass
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies   = &dependacy;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS)
        throw std::runtime_error("Unable to create render pass");
}

void HelloTriangleApplication::createFrameBuffer() {
    swapChainFrameBuffer.resize(swapChainImageViews.size());    // I resize it to make it match the size of the swapchain imageviews

    for (int i = 0; i < swapChainFrameBuffer.size(); i++) {    // for every element in swapchain image view
        VkImageView attachments[] = { swapChainImageViews[i] };

        // create the info struct
        VkFramebufferCreateInfo framebufferInfo {};
        framebufferInfo.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass      = renderPass;
        framebufferInfo.layers          = 1;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments    = attachments;
        framebufferInfo.width           = swapChainExtent.width;
        framebufferInfo.height          = swapChainExtent.height;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFrameBuffer[i]) != VK_SUCCESS)
            throw std::runtime_error("Unable to create the framebuffer");
    }
}

void HelloTriangleApplication::createCommandPool() {
    QueueFamilyIndices queueIndicies = getQueueFamilies(physicalDevice);

    VkCommandPoolCreateInfo poolInfo {};
    poolInfo.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.queueFamilyIndex = queueIndicies.graphicsFamily.value();
    poolInfo.flags            = 0;    // I have no flags to set

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
        throw std::runtime_error("Unable to allocate command pool.");

    poolInfo.queueFamilyIndex = queueIndicies.transferFamily.value();    // for the transfer queue

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &transferCommandPool) != VK_SUCCESS)
        throw std::runtime_error("Unable to allocate transfer pool.");
}

void HelloTriangleApplication::createCommandBuffers() {
    commandBuffers.resize(swapChainFrameBuffer.size());

    // The struct to allocate them
    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool        = commandPool;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;    // can be submitted to a queue for execution
    allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
        throw std::runtime_error("Unable to allocate command buffers");

    // now we have to begin recording commands
    for (int i = 0; i < commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        // the rest of the struct is left empty

        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS)
            throw std::runtime_error("Error starting command buffers.");

        // begin a renderpass here to drawblack I guess
        VkRenderPassBeginInfo beginRPInfo {};
        beginRPInfo.sType       = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        beginRPInfo.renderPass  = renderPass;
        beginRPInfo.framebuffer = swapChainFrameBuffer[i];

        // area specifications
        beginRPInfo.renderArea.offset = { 0, 0 };
        beginRPInfo.renderArea.extent = swapChainExtent;

        VkClearValue clearColor     = { 0.0f, 0.0f, 0.0f, 1.0f };    // black with opacity 1
        beginRPInfo.clearValueCount = 1;
        beginRPInfo.pClearValues    = &clearColor;

        vkCmdBeginRenderPass(commandBuffers[i], &beginRPInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

        // binding the vertexbuffer to be drawn
        VkBuffer vertexBuffers[] = { vertexBuffer };
        VkDeviceSize offsets[]   = { 0 };
        vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT16);    // bind the index buffer

        /*
            modification from the future to add the uniform buffer binding :)
        */

        vkCmdBindDescriptorSets(commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS /* we bins this to the graphics pipeline*/, pipelineLayout, 0 /*index of the first set*/,
                                1 /*how many sets there are*/, &descriptorSets[i] /*what to bind*/, 0, nullptr);    // the last two are, as of yet unknown

        /*
        ***********************************
        ** DRAWING THE MISTICAL TRIANGLE **
        ***********************************
        */

        vkCmdDrawIndexed(commandBuffers[i], static_cast<uint32_t>(verticiesIndices.size()), 1, 0 /*start from index 0*/, 0 /*offset*/, 0 /*start from the beginning*/);

        // vkCmdDraw(commandBuffers[i], static_cast<uint32_t>(verticies.size()) /*verticies to draw*/, 1 /*because we don't use this feature*/, 0 /*the first vertex*/,
        //          0 /*the first instance number*/ );
        // I substitute the above with another call cause now I have an index buffer

        vkCmdEndRenderPass(commandBuffers[i]);    // end the render pass
        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS)
            throw std::runtime_error("failed to record the command buffer. CRYYYYY DIO PORCOOO");
    }
}

void HelloTriangleApplication::drawFrame() {
    /*
        NOTE: The following is a test to see if it is possible to make it work this way, may lead to a crash
        if it fails, or creates problems it should be put at the end of createVertexBuffer()
        the reason of this test is to see if I can modify this memory between frames
    */

    // void *data;
    // vkMapMemory(device, vertexBufferMemory, 0, sizeof(verticies[0]) * verticies.size(), 0, &data);
    // memcpy(data, verticies.data(), sizeof(verticies[0]) * verticies.size());
    // vkUnmapMemory(device, vertexBufferMemory);
    // It works yeah just need to rotate it now

    /*
        The previous test had mixed results as while it did rotate the shape it also
        distorted it, I will now continue the tutorial as it was to continu
    */

    // acquire image from swapchain
    // execute commandbuffer with that image
    // return the image to swap chain for presentation
    // todo this I need semaphores DONE YAY

    // waiting for the fence
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphore[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {    // if the size of the window changed recreate the swapchain
        recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)    // or if there was an error chrash
        throw std::runtime_error("Failed to acquire swapchian image!!");

    // to make sure that it doesn't get rendered on while it is in flight
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE)
        vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    // mark it as now being in use
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    // update the uniform buffer
    updateUniforBuffer(imageIndex);

    // paramenters to configure syncronization
    VkSubmitInfo submitInfo {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    // specifing which semaphores to wait on
    VkSemaphore waitSemaphore[]       = { imageAvailableSemaphore[currentFrame] };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    submitInfo.waitSemaphoreCount     = 1;
    submitInfo.pWaitSemaphores        = waitSemaphore;
    submitInfo.pWaitDstStageMask      = waitStages;

    // what command buffer to submit to
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &commandBuffers[imageIndex];

    // what semaphore to signal once it has finished
    VkSemaphore signalSemaphores[]  = { renderFinishedSemaphore[currentFrame] };
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores    = signalSemaphores;

    // presentation back at the swapchain
    VkPresentInfoKHR presentInfo {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    // semaphores to wait on
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores    = signalSemaphores;
    // what swap chain to present to
    VkSwapchainKHR swapChains[] = { swapChain };
    presentInfo.swapchainCount  = 1;
    presentInfo.pSwapchains     = swapChains;
    presentInfo.pImageIndices   = &imageIndex;    // the index

    vkResetFences(device, 1, &inFlightFences[currentFrame]);    // reset the fence, so we can use it again :)

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS)    // important that this happens before vkQueuePresent or it will stall forever
        throw std::runtime_error("Unable to submit to queue");

    result = vkQueuePresentKHR(presentQueue, &presentInfo);    // submitting request to swapchain

    if (result == VK_ERROR_OUT_OF_DATE_KHR || framebufferResized || result == VK_SUBOPTIMAL_KHR) {    // if the size of the window changed recreate the swapchain
        recreateSwapChain();
        framebufferResized = false;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)    // or if there was an error chrash
        throw std::runtime_error("Failed to acquire swapchian image!!");

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;    // keep the right last frame
}

void HelloTriangleApplication::createSemaphores() {
    VkSemaphoreCreateInfo semaphoreInfo {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;    // necessary because otherwise we will wait for it to be set forever in main loop

    // resize the vectors
    imageAvailableSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphore.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);    // inizialises it to null

    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)    // for every semaphore
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
            throw std::runtime_error("Unable to create semaphores.");
}

void HelloTriangleApplication::createVertexBuffer() {
    VkDeviceSize bufferSize = sizeof(verticies[0]) * verticies.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(bufferSize /*the size of  all the vertex that are submitted*/, VK_BUFFER_USAGE_TRANSFER_SRC_BIT /* I am gonna transfer from this buffer to another*/,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT /*I can write to it from the CPU*/ | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT /*It is not yet know to me why I need this*/,
                 stagingBuffer /* the buffer to fill*/, stagingBufferMemory /*the memory*/);

    // I copy the data into the vertex buffer

    void *data;
    std::cout << "Mappinng memory" << std::endl;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    std::cout << "Copying memory" << std::endl;
    memcpy(data, verticies.data(), (size_t) bufferSize);
    std::cout << "Unmapping memory" << std::endl;
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT /*I am gonna recive data from another buffer*/ |
                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT /*specify that this thing is gonna contain vertex buffer things*/,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT /* only the GPU can access this, betterspeeds baby*/, vertexBuffer, vertexBufferMemory);

    copyBuffer(vertexBuffer, stagingBuffer, bufferSize);    // copy the data

    // cleaning up staging buffer
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void HelloTriangleApplication::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags propeeties, VkBuffer &buffer, VkDeviceMemory &buffermemory) {
    VkBufferCreateInfo bufferInfo {};    // the struct to fill in to create the buffer
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size  = size;
    bufferInfo.usage = usage;
    // bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;                  // it's gonna be used only by one queue so we set it to exclusive
    bufferInfo.sharingMode = VK_SHARING_MODE_CONCURRENT;    // cause also the tranfer queue will be used here

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
        throw std::runtime_error("Unable to allocate vertex buffer!");
    // the buffer now exist but it doesn't have any memory associated to it as nobody thought it was gonna need it

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo {};
    allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize  = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, propeeties);

    switch (vkAllocateMemory(device, &allocInfo, nullptr, &buffermemory)) {    // note doing this more than once is generally a bad idea as it usually is limited to a very low
                                                                               // number the right way would be condense everything into one call and use the offsets thingy
    case VK_SUCCESS:
        std::cout << "successful allocation of memory" << std::endl;
        break;
    case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR:
        throw std::runtime_error("invalid capture address");
        break;
    case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        throw std::runtime_error("device memory");
        break;
    case VK_ERROR_OUT_OF_HOST_MEMORY:
        throw std::runtime_error("host memory");
        break;
    case VK_ERROR_INVALID_EXTERNAL_HANDLE:
        throw std::runtime_error("external address");
        break;
    default:
        throw std::runtime_error("default, HALP");
        break;
    }
    vkBindBufferMemory(device, buffer, buffermemory, 0);    // I bind the memory to the buffer
    std::cout << "Memory binding successfull." << std::endl;
}

uint32_t HelloTriangleApplication::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags preperties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);    // query the GPU to see which  types of memories it has available

    // there various types of memory and picking the right one is important for performance, but for now it is good ebough to find one big enough to host my buffer
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & preperties) == preperties) {
            return i;    // first type of memory that we need
        }
    }
    throw std::runtime_error("failed to find suitable memory type! cause you an idiot");
}

void HelloTriangleApplication::copyBuffer(VkBuffer dstBuffer, VkBuffer srcBuffer, VkDeviceSize size) {
    // I create another command buffer since it is more optimal to do so for short lived things
    // like this
    VkCommandBufferAllocateInfo allocInfo {};
    allocInfo.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;    // I will call this buffer myself
    allocInfo.commandPool        = transferCommandPool;
    allocInfo.commandBufferCount = 1;

    // allocate the command buffer
    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    // I begin the recording of commands
    VkCommandBufferBeginInfo beginInfo {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;    // we are gonna use it only one time and wait for it to do its thing

    vkBeginCommandBuffer(commandBuffer, &beginInfo);    // start it

    VkBufferCopy copyRegion {};    // Info about what to copy from where
    copyRegion.size = size;
    // other parametes left to 0

    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);    // copy the buffer region
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo {};
    submitInfo.sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers    = &commandBuffer;

    vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(transferQueue);    // wait for it to finish

    vkFreeCommandBuffers(device, transferCommandPool, 1, &commandBuffer);    // I free the command buffers
}

void HelloTriangleApplication::createIndexBuffer() {
    VkDeviceSize bufferSize = sizeof(verticiesIndices[0]) * verticiesIndices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    createBuffer(bufferSize /*the size of  all the vertex that are submitted*/, VK_BUFFER_USAGE_TRANSFER_SRC_BIT /* I am gonna transfer from this buffer to another*/,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT /*I can write to it from the CPU*/ | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT /*It is not yet know to me why I need this*/,
                 stagingBuffer /* the buffer to fill*/, stagingBufferMemory /*the memory*/);

    // I copy the data into the vertex buffer

    void *data;
    std::cout << "Mappinng memory" << std::endl;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
    std::cout << "Copying memory" << std::endl;
    memcpy(data, verticiesIndices.data(), (size_t) bufferSize);
    std::cout << "Unmapping memory" << std::endl;
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT /*I am gonna recive data from another buffer*/ |
                     VK_BUFFER_USAGE_INDEX_BUFFER_BIT /*specify that this thing is gonna contain index buffer things*/,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT /* only the GPU can access this, betterspeeds baby*/, indexBuffer, indexBufferMemory);

    copyBuffer(indexBuffer, stagingBuffer, bufferSize);    // copy the data

    // cleaning up staging buffer
    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void HelloTriangleApplication::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding {};                        // struct to cre<te it
    uboLayoutBinding.binding         = 0;                                    // sets which binding the shader its gonna find this in
    uboLayoutBinding.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;    // what we are gonna bind
    uboLayoutBinding.descriptorCount = 1;                                    // how many things we are gonna bind, example two things for two different models

    uboLayoutBinding.stageFlags         = VK_SHADER_STAGE_VERTEX_BIT;    // in which stage of the pipeline this thing is gonna be used
    uboLayoutBinding.pImmutableSamplers = nullptr;                       // used for multisampling, that we are not doing at the time

    VkDescriptorSetLayoutCreateInfo layoutInfo {};    // information to create the layout
    layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;    // how many things we are gonna bound
    layoutInfo.pBindings    = &uboLayoutBinding;

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorLayout) != VK_SUCCESS)
        throw std::runtime_error("Unable to create the set layout");
}

void HelloTriangleApplication::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    // one per image
    uniformBuffers.resize(swapChainImages.size());
    uniformBuffersMemory.resize(swapChainImages.size());

    for (int i = 0; i < swapChainImages.size(); i++)
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i],
                     uniformBuffersMemory[i]);
    std::cout << "Created uniform buffers" << std::endl;
}

void HelloTriangleApplication::updateUniforBuffer(uint32_t currentImage) {
    static auto startTime = std::chrono::high_resolution_clock::now();    // stati: does not reset after the funtion ends, but remains set for the next call to the funtion
                                                                          // starts mesuring the time
    auto currentTime = std::chrono::high_resolution_clock::now();         // gets the current time

    float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    UniformBufferObject ubo {};
    ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(1.0f, -1.0f, 1.0f));    // generates rotation matrix of pi/2 every second around the z axis

    ubo.model =
        glm::scale(ubo.model, glm::vec3(1.0f + glm::sin(time * glm::radians(45.0f))/2, 1.0f + glm::sin(time * glm::radians(45.0f))/2, 1.0f + glm::sin(time * glm::radians(45.0f))/2));

    ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f) /*coordinates of the camera*/, glm::vec3(0.0f, 0.0f, 0.0f) /*where the camera is looking at*/,
                           glm::vec3(0.0f, 0.0f, 1.0f) /*where is up*/);    // setting the camera

    ubo.proj = glm::perspective(glm::radians(45.0f) /*field of view*/, swapChainExtent.width / (float) swapChainExtent.height /*aspect ration*/, 0.1f /*near plain*/,
                                10.0f /*far plane(aka render distance)*/);

    ubo.proj[1][1] *= -1;    // flip the Y axis, as glm was designed for openGL, which inverts the Y axis, while vulkan does not

    // loading the data into the buffer
    void *data;
    vkMapMemory(device, uniformBuffersMemory[currentImage], 0, sizeof(UniformBufferObject), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(device, uniformBuffersMemory[currentImage]);
}

void HelloTriangleApplication::createDescriptorPool() {
    // first I have to signal how many there are
    VkDescriptorPoolSize poolSize {};
    poolSize.type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;                // the type of buffer
    poolSize.descriptorCount = static_cast<uint32_t>(swapChainImages.size());    // how many there are

    // creating the pool
    VkDescriptorPoolCreateInfo poolInfo {};
    poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes    = &poolSize;
    poolInfo.maxSets       = static_cast<uint32_t>(swapChainImages.size());    // maximum number of descriptor that may be created

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS)
        throw std::runtime_error("Unable to create the descriptor pool.");
    std::cout << "created descriptor pool" << std::endl;
}

void HelloTriangleApplication::createDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(swapChainImages.size(), descriptorLayout);    // I create an array of copies of that layout

    VkDescriptorSetAllocateInfo allocInfo {};    // The data necessary to allocate them
    allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool     = descriptorPool;                                   // of which descriptor pool they are a part of
    allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());    // how many to allocate
    allocInfo.pSetLayouts        = layouts.data();                                   // where to allocate them

    descriptorSets.resize(swapChainImages.size());
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS)
        throw std::runtime_error("Unable to create descript sets");

    for (int i = 0; i < swapChainImages.size(); i++) {    // for every member I write the descriptor
        VkDescriptorBufferInfo bufferInfo {};             // what type of buffer am I using
        bufferInfo.buffer = uniformBuffers[i];            // what buffer
        bufferInfo.offset = 0;
        bufferInfo.range  = sizeof(UniformBufferObject);    // how much memory it occupies

        VkWriteDescriptorSet descriptorWrite {};    // what we write in this buffer
        descriptorWrite.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet          = descriptorSets[i];    // where to write to
        descriptorWrite.dstBinding      = 0;                    // what binding I am expecting in the shader
        descriptorWrite.dstArrayElement = 0;                    // the first element in the array

        descriptorWrite.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;    // what buffer it is
        descriptorWrite.descriptorCount = 1;                                    // how many elements to update

        descriptorWrite.pBufferInfo      = &bufferInfo;    // I use this one so I leave the other to nullptr
        descriptorWrite.pImageInfo       = nullptr;        // depending on the type of buffer I could've used one of these
        descriptorWrite.pTexelBufferView = nullptr;

        vkUpdateDescriptorSets(device, 1, &descriptorWrite, 0, nullptr);    // update the new info; DO IT YOU IDIOT, OTHERWISE IT CHRASHES
    }
}

// void HelloTriangleApplication::updatePos() {
//     endTime = new std::chrono::steady_clock::time_point(std::chrono::steady_clock::now());

//     std::chrono::nanoseconds timeElapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(*endTime - *beginTime);

//     double matrix[2][2];
//     double angularSpeed = M_PI / 1500000000.0;    // the angular speed

//     double angle = angularSpeed * timeElapsed.count();    // the angle to rotate the triangle by

//     // I create fill in the matrix
//     matrix[0][0] = cos(angle);     // the matrix is
//     matrix[0][1] = -sin(angle);    // cos()    -sin()
//     matrix[1][0] = sin(angle);     // sin()    cos()
//     matrix[1][1] = cos(angle);

//     // I create the new verticies
//     glm::vec2 newPos;
//     for (int i = 0; i < verticies.size(); i++) {
//         glm::vec2 oldPos = { verticies[i].pos.x, verticies[i].pos.y };    // extract the 2D coordinates
//         oldPos           = convertToPixel(oldPos);                        // convert them to pixel coordinates

//         // apply the rotation matrix
//         newPos.x = matrix[0][0] * oldPos.x + matrix[0][1] * oldPos.y;
//         newPos.y = matrix[1][0] * oldPos.x + matrix[1][1] * oldPos.y;

//         // transform back from pixel to screen
//         newPos = convertToScreen(newPos);
//         // Save them
//         verticies[i].pos.x = newPos.x;
//         verticies[i].pos.y = newPos.y;
//     }

//     // reset the timers
//     delete endTime;
//     delete beginTime;
//     beginTime = new std::chrono::steady_clock::time_point(std::chrono::steady_clock::now());
// }

// glm::vec2 HelloTriangleApplication::convertToPixel(glm::vec2 oldPos) {
//     glm::vec2 newPos;
//     newPos.x = oldPos.x * swapChainExtent.width / 2.0f;
//     newPos.y = oldPos.y * swapChainExtent.height / 2.0f;
//     return newPos;
// }

// glm::vec2 HelloTriangleApplication::convertToScreen(glm::vec2 oldPos) {
//     glm::vec2 newPos;
//     newPos.x = oldPos.x / (swapChainExtent.width / 2.0f);
//     newPos.y = oldPos.y / (swapChainExtent.height / 2.0f);
//     return newPos;
// }

static std::vector<char> readFile(const std::string &path) {
    std::fstream in(path, std::ios::in | std::ios::binary | std::ios::ate);    // ate start from the end of the file, useful to get its length

    if (!in.is_open())
        throw std::runtime_error("Unable to read the file");
    // get the size of the file by the position of the reading head (at the end of the file)
    size_t fileSize = static_cast<size_t>(in.tellg());
    std::vector<char> buffer(fileSize);    // I create space in memory to host the entire file
    in.seekg(0);                           // reposition the reading head at the beginning of the file
    in.read(buffer.data(), fileSize);      // read it and load it intomemory
    in.close();                            // close the file
    return buffer;
}
