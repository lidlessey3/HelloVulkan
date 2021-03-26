#ifndef TRIANGLE_H_INCLUDE
#define TRIANGLE_H_INCLIDE

#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS

#include <GLFW/glfw3.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.h>

const int MAX_FRAMES_IN_FLIGHT = 2;

class HelloTriangleApplication {
public:
    void run();

private:
    // Internal structs
    typedef struct {    // will store the indicies for the ques for vulkan
        std::optional<uint32_t> graphicsFamily;
        std::optional<uint32_t> presentFamily;
        std::optional<uint32_t> transferFamily;

        bool isComplete() {
            return graphicsFamily.has_value() && presentFamily.has_value();
        }
    } QueueFamilyIndices;

    struct SwapChainSupportDetails {
        VkSurfaceCapabilitiesKHR capabilities;
        std::vector<VkSurfaceFormatKHR> formats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    struct Vertex {
        glm::vec3 pos;
        glm::vec3 color;
        glm::vec3 texCoord;

        static VkVertexInputBindingDescription getBindingDescription() {    // telling vulkan how to pass this data to the GPU
            VkVertexInputBindingDescription bindingDescription {};          // Describes at which rate to feed them
            bindingDescription.binding   = 0;
            bindingDescription.stride    = sizeof(Vertex);    // bites from one entry to the next
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
            std::array<VkVertexInputAttributeDescription, 3> attributeDescription {};
            attributeDescription[0].binding  = 0;
            attributeDescription[0].location = 0;                             // location in the input
            attributeDescription[0].format   = VK_FORMAT_R32G32B32_SFLOAT;    // the dimension of the array
            attributeDescription[0].offset   = offsetof(Vertex, pos);

            attributeDescription[1].binding  = 0;
            attributeDescription[1].location = 1;
            attributeDescription[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescription[1].offset   = offsetof(Vertex, color);

            attributeDescription[2].binding  = 0;
            attributeDescription[2].location = 2;
            attributeDescription[2].format   = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescription[2].offset   = offsetof(Vertex, texCoord);

            return attributeDescription;
        }
    };

    struct UniformBufferObject {
        glm::mat4 model;    // model matrix where all transformation should go (I.e. rotation, translation, scaling)
        glm::mat4 view;     // view matrix
        glm::mat4 proj;     // projection matrix
    };

    // // Internal variables
    // const std::vector<Vertex> verticies = {
    //     { { 0.5f, -0.5f, -0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 0.0f } },    // constant values of vertices
    //     { { 0.5f, 0.5f, -0.5f }, { 0.8f, 0.0f, 1.0f }, { 1.0f, 1.0f, 0.0f } },   { { 0.5f, 0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 1.0f, 1.0f } },
    //     { { 0.5f, -0.5f, 0.5f }, { 0.0f, 0.0f, 1.0f }, { 1.0f, 0.0f, 1.0f } },   { { -0.5f, -0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 1.0f } },
    //     { { -0.5f, -0.5f, -0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 0.0f, 0.0f } }, { { -0.5f, 0.5f, 0.5f }, { 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 1.0f } },
    //     { { -0.5f, 0.5f, -0.5f }, { 0.8f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f } }
    // };

    // const std::vector<uint16_t> verticiesIndices = {
    //     0, 3, 1, 1, 3, 2, 0, 5, 3, 3, 5, 4, 3, 4, 2, 2, 4, 6, 0, 1, 5, 5, 1, 7, 5, 7, 4, 4, 7, 6, 7, 1, 6, 6, 1, 2
    // };    // in which order the verticies are to be drawn
    // these data will now be loaded via the apropriate function

    std::vector<Vertex> verticies;
    std::vector<uint32_t> verticiesIndices;

    GLFWwindow *window;
    VkInstance instance;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    QueueFamilyIndices indices;                          // don't do this again, it is a bad idea as they are never used other than during creation
    VkDevice device;                                     // virtual GPU handle (not sure what it means though)
    VkQueue graphicsQueue;                               // queue for graphic operations
    VkQueue presentQueue;                                // queue for displying images
    VkQueue transferQueue;                               // queue for stransfering data to the vertex buffer
    VkSurfaceKHR surface;                                // surface on which to draw
    VkSwapchainKHR swapChain;                            // swapchain finally
    std::vector<VkImage> swapChainImages;                // handles for the images in the swapchains
    VkFormat swapChainFormat;                            // the format of the swapchain
    VkExtent2D swapChainExtent;                          // the extent of the swapchain
    std::vector<VkImageView> swapChainImageViews;        // handles for some stuff
    VkRenderPass renderPass;                             // will contain the render pass
    VkPipelineLayout pipelineLayout;                     // the pipeline layout
    VkPipeline graphicsPipeline;                         // the pipeline finallyyyyyyy
    std::vector<VkFramebuffer> swapChainFrameBuffer;     // will be the framebuffer
    VkCommandPool commandPool;                           // allows the creation of command buffers
    VkCommandPool transferCommandPool;                   // command pool for the transfer queue
    std::vector<VkCommandBuffer> commandBuffers;         // that I have the pool I can do the buffers
    std::vector<VkSemaphore> imageAvailableSemaphore;    // signals that the image is ready to be drawn
    std::vector<VkSemaphore> renderFinishedSemaphore;    // signals that it finished rendering
    size_t currentFrame;                                 // the current frame
    std::vector<VkFence> inFlightFences;                 // the fences
    std::vector<VkFence> imagesInFlight;                 // fence to prevent rendering on an image already in flight
    bool framebufferResized;                             // set to true if window is resized
    VkBuffer vertexBuffer;                               // it's gonna hold the vertex buffer things
    VkDeviceMemory vertexBufferMemory;                   // will point to the allocated memory
    VkBuffer indexBuffer;                                // will contain the indicies in whose order is to be followed
    VkDeviceMemory indexBufferMemory;                    // allocate memory for the above thing
    VkDescriptorSetLayout descriptorLayout;              // gonna contain the layout to feed into the pipeline
    std::vector<VkBuffer> uniformBuffers;                // uniform buffers (one per vertex I guess) lies!! one per swapchain image
    std::vector<VkDeviceMemory> uniformBuffersMemory;    // their memory locations
    VkDescriptorPool descriptorPool;                     // the descriptor pool
    std::vector<VkDescriptorSet> descriptorSets;         // the sets of the above pool
    VkImage textureImage;                                // the handle to image
    VkDeviceMemory textureImageMemory;                   // the memory where the image will be stored
    VkImageView textureImageView;                        // the view of the texture image
    VkSampler textureSampler;                            // the handle for the sampling
    VkImage depthImage;                                  // created to account for depth
    VkDeviceMemory depthImageMemory;                     // the memory of the above
    VkImageView depthImageView;                          // the image view of the depth image
    std::vector<const char *> deviceRequiredExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };    // required extension from the graphic card

    // to count the time
    // std::chrono::steady_clock::time_point *beginTime;
    // std::chrono::steady_clock::time_point *endTime;

    // methods declaration
    void initWindow();                                                                   // Inizializza la finestra
    static void framebufferResizeCallBack(GLFWwindow *window, int width, int height);    // callback in case I resize the window
    void initVulkan();                                                                   // Inizializia vulkan
    void mainLoop();                                                                     // fa cose mentre la finestra è attiva
    void cleanup();                                                                      // cleans it up
    void createInstance();                                                               // creates the vulcan instance by passing the required structs to Vulkan
    void pickGraphicsCard();                                                             // chooses the right GPU to use
    int rateDevice(VkPhysicalDevice device);                                             // checks if the given card is good enough
    void getQueueFamilies();                                                             // will set the indices for required/optional queues
    QueueFamilyIndices getQueueFamilies(VkPhysicalDevice physicalDevice);                // Same as above but for the specified device
    void createLogicalDevice();                                                          // Creates a logical device
    void createSurface();                                                                // will create a surface on which I can draw stuff :)
    bool checkDeviceExtentionSupport(VkPhysicalDevice device);                           // will check if the given physical device supports the required extensions
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);              // will retrive support swapchain features
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);    // Selects the best foramat(color depth)
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);     // Selects the best present mode (V-sync, double or trible buffering)
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);                              // chooses the best resolution for each frame
    void createSwapChain();                                                                                 // create the swap chain
    void cleanupSwapChain();                                                                                // cleansup the swapchain
    void recreateSwapChain();                                                                               // recretes the swap chain (for example after a resize)
    void createImageViews();                                                                                // create the images view handles
    void createGraphicsPipeline();                                       // will create the pipeline for drawing stuff, will probably kill myself afterwards
    void createRenderPass();                                             // it will create a render pass
    VkShaderModule createShaderModule(const std::vector<char> &code);    // wraps the code into the correct struct
    void createFrameBuffer();                                            // Creates the frame buffer
    void createCommandPool();                                            // creates a valid command pool
    void createCommandBuffers();                                         // creates the command buffers
    void createSemaphores();                                             // creo il semafor
    void drawFrame();                                                    // disegna un frame
    void createVertexBuffer();                                           // creates the vertex buffer
    void createIndexBuffer();                                            // creates the indexbuffer
    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags propeeties, VkBuffer &buffer,
                      VkDeviceMemory &buffermemory);                               // helper function of createVertexBuffer
    void copyBuffer(VkBuffer dstBuffer, VkBuffer srcBuffer, VkDeviceSize size);    // copies size bytes from src to dst
    // void updatePos();                                                                  // it rotates the three verticies
    // glm::vec2 convertToPixel(glm::vec2 oldPos);                                        // gets the data in screen coordinates and converts it to pìxel coordinates
    // glm::vec2 convertToScreen(glm::vec2 oldPos);                                       // gets the coordinates in pixel c. and converts it to screen c
    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags preperties);    // the GPU offers various types of memory, we are gonna find the best one for us :)
    void createDescriptorSetLayout();                                                  // sets the descriptor binding used in the shader, it is needed before pipeline creation
    void createUniformBuffers();                                                       // creates the uniform buffers
    void updateUniforBuffer(uint32_t currentImage);                                    // updates the matrixes of the uniform buffers
    void createDescriptorPool();                                                       // will create a descriptor pool for each uniform buffer
    void createDescriptorSets();                                                       // now that we have a pool we can create sets
    void createTextureImage();                                                         // loads an image into memory
    void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage &image,
                     VkDeviceMemory &imageMemory, uint32_t arrayLayers, VkImageCreateFlags flags);          // loads the image into memory
    VkCommandBuffer beginSingleTimeCommands(VkCommandPool commandPool);                                     // will initialize a command buffer to make a single time commnad
    void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkQueue queue, VkCommandPool commandPool);    // will terminate the buffer create in the above function
    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t layers);    // sets the image in the right layout so it can then be copied
    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, uint32_t layers);                         // will copy the image from buffer to image
    void createTextureImageView();                                                                                   // I create the image view for the textureimage
    VkImageView createImageView(VkImage image, VkFormat format, VkImageViewType viewType, uint32_t layerCount,
                                VkImageAspectFlags aspectFlag);    // helper function to create an image view
    void createTextureSampler();                                   // creates a textures sampler to tell the shader what sampling to apply when reading the image data
    void createDepthResources();                                   // creates the resource to deal with image depth
    void loadModel();                                              // loads a model to memory
    VkFormat findDepthFormat();                                    // finds the right depth format
    bool hasStencilComponent(VkFormat format);                     // checks wether the given format supports stencil
    void createCubeTextureImage();
    void createCubeTextureImageView();
    VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates, VkImageTiling tiling, VkFormatFeatureFlags features);    // finds the best format for the support thing
};

static std::vector<char> readFile(const std::string &path);    // loads the given file into memory

#endif
