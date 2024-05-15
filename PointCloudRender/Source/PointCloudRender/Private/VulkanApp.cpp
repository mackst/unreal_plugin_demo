
#include "VulkanApp.h"

#include "Interfaces/IPluginManager.h"
#include "Engine/Texture2D.h"
#include "HAL/UnrealMemory.h"
#include "ImageWriteBlueprintLibrary.h"
#include "Subsystems/EditorActorSubsystem.h"
//#include "Components/StaticMeshComponent.h"
//#include "Rendering/PositionVertexBuffer.h"
//#include "StaticMeshResources.h"


THIRD_PARTY_INCLUDES_START
#include <glm/gtc/matrix_transform.hpp>
#include <vulkan/vulkan.hpp>

#define VMA_IMPLEMENTATION
#define VMA_VULKAN_VERSION 1003000 // Vulkan 1.3
#include <vma/vk_mem_alloc.h>
THIRD_PARTY_INCLUDES_END


void VulkanApp::PointCloudRender(uint32_t width, uint32_t height, FSlateBrush& brush)
{
    // 创建一个unique instance
    const vk::ApplicationInfo appInfo = vk::ApplicationInfo(
        "Task 9", 1, "PointCloudRender", 1, VK_API_VERSION_1_3
    );

    vk::UniqueInstance instance = vk::createInstanceUnique(
        vk::InstanceCreateInfo(
            {}, &appInfo,
            0, nullptr
        )
    );
    

    // 选择物理设备
    
    vk::PhysicalDevice pDevice;
    size_t pQueueFamily;
    bool familyFound = false;
    for (const auto& device : instance->enumeratePhysicalDevices())
    {
        const auto queueFamilyProperties = device.getQueueFamilyProperties();
        for (size_t i = 0; i < queueFamilyProperties.size(); i++)
        {
            if (queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eCompute &&
                queueFamilyProperties[i].queueFlags & vk::QueueFlagBits::eTransfer)
            {
                pDevice = device;
                pQueueFamily = i;
                familyFound = true;
                break;
            }
        }
        if (familyFound) break;
    }

    if (!familyFound)
    {
        const FText message = FText::FromString(L"找不到合适的物理GPU设备");
        const FText title = FText::FromString(L"错误");
        FMessageDialog::Open(EAppMsgType::Ok, message, title);
        return;
    }

    vk::PhysicalDevice physicalDevice = pDevice;
    

    // 从物理设备创建一个 unique device
    constexpr float priority[] = { 1.0f };
    
    const vk::DeviceQueueCreateInfo queueCreateInfo = vk::DeviceQueueCreateInfo(
        {}, pQueueFamily, 1, priority
    );

    vk::UniqueDevice device = physicalDevice.createDeviceUnique(vk::DeviceCreateInfo(
        {}, 1U, &queueCreateInfo, 0U, nullptr
    ));

    vk::Queue queue = device->getQueue(pQueueFamily, 0U);
    

    // 创建一个unique command pool
    
    vk::UniqueCommandPool commandPool = device->createCommandPoolUnique(vk::CommandPoolCreateInfo(
        vk::CommandPoolCreateFlagBits::eResetCommandBuffer, pQueueFamily
    ));

    // 模型顶点信息
    glm::vec3 minimum, maximum;
    std::vector<Point> pointCloud;
    //std::string testfile = "E:/assets/flowers.obj";
    //ReadPointCloud(testfile.c_str(), pointCloud, minimum, maximum);
    GetPointCloudFromWorld(pointCloud, minimum, maximum);
    if (pointCloud.size() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("无顶点信息"));
        return;
    }

    const size_t pointSize = pointCloud.size() * sizeof(Point);
    const size_t imageSize = width * height * sizeof(uint32_t);
    const uint32_t workGroupSize = (static_cast<uint32_t>(pointCloud.size()) / 128U) + 1U;

    // 创建 vma allocator
    VmaAllocatorCreateInfo allocatorInfo = {};
    allocatorInfo.physicalDevice = physicalDevice;
    allocatorInfo.device = *device;
    allocatorInfo.instance = *instance;
    allocatorInfo.vulkanApiVersion = VK_API_VERSION_1_3;
    VmaAllocator allocator;
    vmaCreateAllocator(&allocatorInfo, &allocator);
    
    // buffers
    VkBuffer paramsBuffer, pointsBuffer, imageBuffer;
    VkBufferCreateInfo paramsBufferCreateInfo = vk::BufferCreateInfo({}, sizeof(Parameters), vk::BufferUsageFlagBits::eUniformBuffer);
    VkBufferCreateInfo pointsBufferCreateInfo = vk::BufferCreateInfo({}, pointSize, vk::BufferUsageFlagBits::eStorageBuffer);
    VkBufferCreateInfo imageBufferCreateInfo = vk::BufferCreateInfo({}, imageSize, vk::BufferUsageFlagBits::eStorageBuffer);
    VmaAllocationCreateInfo paramsBufferAllocCreateInfo = {};
    VmaAllocationCreateInfo pointsBufferAllocCreateInfo = {};
    VmaAllocationCreateInfo imageBufferAllocCreateInfo = {};
    paramsBufferAllocCreateInfo.usage = pointsBufferAllocCreateInfo.usage = imageBufferAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
    auto memReqFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    paramsBufferAllocCreateInfo.requiredFlags = pointsBufferAllocCreateInfo.requiredFlags = imageBufferAllocCreateInfo.requiredFlags = memReqFlags;
    paramsBufferAllocCreateInfo.flags = pointsBufferAllocCreateInfo.flags = imageBufferAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
    VmaAllocation paramsBufferAlloc, pointsBufferAlloc, imageBufferAlloc;
    VkResult bufferResult;
    bufferResult = vmaCreateBuffer(allocator, &paramsBufferCreateInfo, &paramsBufferAllocCreateInfo, &paramsBuffer, &paramsBufferAlloc, nullptr);
    if (bufferResult != VK_SUCCESS)
    {
        UE_LOG(LogTemp, Error, TEXT("无法创建 paramsBuffer"));
        return;
    }
    bufferResult = vmaCreateBuffer(allocator, &pointsBufferCreateInfo, &pointsBufferAllocCreateInfo, &pointsBuffer, &pointsBufferAlloc, nullptr);
    if (bufferResult != VK_SUCCESS)
    {
        UE_LOG(LogTemp, Error, TEXT("无法创建 pointsBuffer"));
        return;
    }
    bufferResult = vmaCreateBuffer(allocator, &imageBufferCreateInfo, &imageBufferAllocCreateInfo, &imageBuffer, &imageBufferAlloc, nullptr);
    if (bufferResult != VK_SUCCESS)
    {
        UE_LOG(LogTemp, Error, TEXT("无法创建 imageBuffer"));
        return;
    }

    // 创建 descriptor set
    std::vector<vk::DescriptorPoolSize> poolSizes = {
            { vk::DescriptorType::eUniformBuffer, 1 },
            { vk::DescriptorType::eStorageBuffer, 2 }
    };
    vk::DescriptorPoolCreateInfo poolCreateInfo(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1, poolSizes);
    vk::UniqueDescriptorPool descriptorPool = device->createDescriptorPoolUnique(poolCreateInfo);

    vk::DescriptorSetLayoutBinding paramsBinding(0, vk::DescriptorType::eUniformBuffer, 1, vk::ShaderStageFlagBits::eCompute);
    vk::DescriptorSetLayoutBinding pointsBinding(1, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute);
    vk::DescriptorSetLayoutBinding imageBinding(2, vk::DescriptorType::eStorageBuffer, 1, vk::ShaderStageFlagBits::eCompute);
    std::vector<vk::DescriptorSetLayoutBinding> bindings = { paramsBinding, pointsBinding, imageBinding };
    vk::UniqueDescriptorSetLayout descriptorSetLayout = device->createDescriptorSetLayoutUnique(vk::DescriptorSetLayoutCreateInfo({}, bindings));

    vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo(*descriptorPool, *descriptorSetLayout);
    std::vector<vk::UniqueDescriptorSet> descriptorSets = device->allocateDescriptorSetsUnique(descriptorSetAllocateInfo);
    vk::UniqueDescriptorSet& descriptorSet = descriptorSets[0];

    vk::DescriptorBufferInfo paramsBufferDescInfo(paramsBuffer, 0, VK_WHOLE_SIZE);
    vk::DescriptorBufferInfo pointsBufferDescInfo(pointsBuffer, 0, VK_WHOLE_SIZE);
    vk::DescriptorBufferInfo imageBufferDescInfo(imageBuffer, 0, VK_WHOLE_SIZE);
    vk::WriteDescriptorSet paramsDescWrite(*descriptorSet, 0, 0, 1, vk::DescriptorType::eUniformBuffer, {}, &paramsBufferDescInfo);
    vk::WriteDescriptorSet pointsDescWrite(*descriptorSet, 1, 0, 1, vk::DescriptorType::eStorageBuffer, {}, &pointsBufferDescInfo);
    vk::WriteDescriptorSet imageDescWrite(*descriptorSet, 2, 0, 1, vk::DescriptorType::eStorageBuffer, {}, &imageBufferDescInfo);
    device->updateDescriptorSets({ paramsDescWrite, pointsDescWrite, imageDescWrite }, {});

    // 相机参数
    const float fov = 45.0f;
    glm::vec3 center = (minimum + maximum) * 0.5f;
    float span = glm::length(maximum - minimum);
    auto view = glm::lookAt(center + glm::vec3(-0.9f * span, 0, 0.15f * span), center, glm::vec3(0, 0, -1));
    auto projection = glm::perspective(45.f, ((float)width) / height, 0.1f, 2 * span);

    // 设置渲染参数
    Parameters* pointcloudParams_mapped;
    vmaMapMemory(allocator, paramsBufferAlloc, (void**)&pointcloudParams_mapped);
    pointcloudParams_mapped->width = width;
    pointcloudParams_mapped->height = height;
    pointcloudParams_mapped->numPoints = (uint32_t)pointCloud.size();
    pointcloudParams_mapped->mvp = projection * view;

    // 写入云点数据到gpu显存
    glm::vec4* pointcloud_mapped;
    vmaMapMemory(allocator, pointsBufferAlloc, (void**)&pointcloud_mapped);
    std::memcpy(pointcloud_mapped, pointCloud.data(), pointCloud.size() * sizeof(Point));

    // 把图片像素设置成 0xFFFFFFFF
    uint32_t* image_mapped;
    vmaMapMemory(allocator, imageBufferAlloc, (void**)&image_mapped);
    std::memset(image_mapped, 0xFF, sizeof(uint32_t) * width * height);

    // 清理
    vmaUnmapMemory(allocator, paramsBufferAlloc);
    vmaUnmapMemory(allocator, pointsBufferAlloc);
    vmaUnmapMemory(allocator, imageBufferAlloc);
    vmaFlushAllocation(allocator, paramsBufferAlloc, 0, VK_WHOLE_SIZE);
    vmaFlushAllocation(allocator, pointsBufferAlloc, 0, VK_WHOLE_SIZE);
    vmaFlushAllocation(allocator, imageBufferAlloc, 0, VK_WHOLE_SIZE);

    // 创建shader
    FString resourcePath = IPluginManager::Get().FindPlugin("PointCloudRender")->GetBaseDir() / TEXT("Resources");
    FString shaderPath = resourcePath / TEXT("pointcloud.comp.spv");
    const char* shaderPathChar = TCHAR_TO_ANSI(*shaderPath);
    const auto shader = loadShader(shaderPathChar);
    vk::UniqueShaderModule shaderModule = device->createShaderModuleUnique(
        vk::ShaderModuleCreateInfo({}, shader)
    );
    // unique pipeline layout
    std::vector<vk::DescriptorSetLayout> descriptorSetLayouts = { *descriptorSetLayout };
    vk::UniquePipelineLayout pipelineLayout = device->createPipelineLayoutUnique(
        vk::PipelineLayoutCreateInfo(
            {},
            (uint32_t)descriptorSetLayouts.size(),
            descriptorSetLayouts.data()
        )
    );

    // unique pipeline cache
    vk::UniquePipelineCache pipelineCache = device->createPipelineCacheUnique(vk::PipelineCacheCreateInfo());

    // 创建计算管线
    vk::PipelineShaderStageCreateInfo ssCreateInfo = vk::PipelineShaderStageCreateInfo(
        {}, vk::ShaderStageFlagBits::eCompute, 
        *shaderModule, "main"
    );
    vk::ComputePipelineCreateInfo pipelineInfo = vk::ComputePipelineCreateInfo(
        {}, ssCreateInfo, *pipelineLayout, nullptr, 0
    );
    auto pResult = device->createComputePipelineUnique(*pipelineCache, pipelineInfo);
    if (pResult.result != vk::Result::eSuccess)
    {
        UE_LOG(LogTemp, Error, TEXT("无法创建计算管线"));
        return;
    }

    vk::UniquePipeline pipeline = std::move(pResult.value);

    // command buffer
    vk::CommandBufferAllocateInfo cbAllocateInfo(*commandPool, vk::CommandBufferLevel::ePrimary, 1U);
    auto cmdBuffers = device->allocateCommandBuffersUnique(cbAllocateInfo);

    vk::MemoryBarrier memoryBarrier(vk::AccessFlagBits::eShaderWrite, vk::AccessFlagBits::eHostRead);
    cmdBuffers[0]->begin(vk::CommandBufferBeginInfo{});
    cmdBuffers[0]->bindPipeline(vk::PipelineBindPoint::eCompute, *pipeline);
    cmdBuffers[0]->bindDescriptorSets(vk::PipelineBindPoint::eCompute, *pipelineLayout, 0, *descriptorSet, {});
    // 线程数(组)
    cmdBuffers[0]->dispatch(workGroupSize, 1, 1);
    cmdBuffers[0]->pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader, vk::PipelineStageFlagBits::eHost, {}, memoryBarrier, {}, {});
    cmdBuffers[0]->end();

    // 提交渲染命令
    queue.submit(vk::SubmitInfo({}, {}, *cmdBuffers[0]));

    // 等待渲染完成
    device->waitIdle();

    // 把图片像素数据从显存传回内存
    vmaMapMemory(allocator, imageBufferAlloc, (void**)&image_mapped);
    std::vector<uint32_t> image(image_mapped, image_mapped + width * height);
    std::for_each(image.begin(), image.end(), [](uint32_t& rgbd) {
        rgbd = 0xFF000000 | ((rgbd & 0xF) << 4) | ((rgbd & 0xF0) << 8) | ((rgbd & 0xF00) << 12);
    });

    //auto tembrush = createBrush(image, width, height);
    brush = createBrush(image, width, height);
    //stbi_write_png("output.png", width, height, STBI_rgb_alpha, image.data(), 0);
    vmaUnmapMemory(allocator, imageBufferAlloc);

    vmaDestroyBuffer(allocator, paramsBuffer, paramsBufferAlloc);
    vmaDestroyBuffer(allocator, pointsBuffer, pointsBufferAlloc);
    vmaDestroyBuffer(allocator, imageBuffer, imageBufferAlloc);
    vmaDestroyAllocator(allocator);
}

// 读取obj文件
void VulkanApp::ReadPointCloud(const char* path, std::vector<Point>& pointCloud, glm::vec3& minimum, glm::vec3& maximum)
{
    std::ifstream infile(path);
    if (!infile.good())
    {
        const FText message = FText::FromString(L"无法加载模型文件");
        const FText title = FText::FromString(L"错误");
        FMessageDialog::Open(EAppMsgType::Ok, message, title);
        return;
    }

    minimum = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
    maximum = glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    std::string line;
    Point p;
    while (std::getline(infile, line))
    {
        if (line.substr(0, 1) == "v")
        {
            std::istringstream iss(line.substr(1));
            if (!(iss >> p.position.x >> p.position.y >> p.position.z))
                continue;
            if (!(iss >> p.color.x >> p.color.y >> p.color.z))
                p.color = glm::vec3(0.5f, 0.5f, 0.5f);

            pointCloud.push_back(p);
            minimum = glm::min(p.position, minimum);
            maximum = glm::max(p.position, maximum);
        }
    }
}

void VulkanApp::GetPointCloudFromWorld(std::vector<Point>& pointCloud, glm::vec3& minimum, glm::vec3& maximum)
{
    minimum = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
    maximum = glm::vec3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    
    // 获取UEditorActorSubsystem
    UEditorActorSubsystem* EditorActorSubsystem = GEditor->GetEditorSubsystem<UEditorActorSubsystem>();
    if (!EditorActorSubsystem) return;

    // 获取所有actor
    TArray<AActor*> AllActorsInLevel = EditorActorSubsystem->GetAllLevelActors();
    for (auto actor : AllActorsInLevel)
    {
        // 位移信息用来计算世界坐标
        auto& aTran = actor->GetActorTransform();
        auto aPos = actor->GetActorLocation();

        // 获取UStaticMeshComponent
        TArray<UStaticMeshComponent*> StaticComps;
        actor->GetComponents<UStaticMeshComponent*>(StaticComps);
        for (auto comp : StaticComps)
        {
            UStaticMesh* mesh = comp->GetStaticMesh();
            if (!mesh) continue;
            //UE_LOG(LogTemp, Display, TEXT("%s"), *actor->GetActorLabel());
            FStaticMeshRenderData* rdata = mesh->GetRenderData();
            FStaticMeshLODResources& smlra = rdata->LODResources[0];
            FStaticMeshVertexBuffers& buffers = smlra.VertexBuffers;
            FPositionVertexBuffer& vertexBuffer = buffers.PositionVertexBuffer;
            const uint32_t vnum = vertexBuffer.GetNumVertices();
            for (uint32_t i = 0; i < vnum; i++)
            {
                //UE_LOG(LogTemp, Display, TEXT("%d"), vnum);
                auto& vpos = vertexBuffer.VertexPosition(i);
                //const auto pos = comp->GetComponentToWorld().TransformPosition(FVector(vpos.X, vpos.Y, vpos.Z));
                auto pos = aPos + aTran.TransformVector(FVector(vpos.X, vpos.Y, vpos.Z));
                Point p;
                p.position.x = pos.X;
                p.position.y = pos.Y;
                p.position.z = pos.Z;
                p.color.r = FMath::RandRange(0.0f, 1.0f);
                p.color.g = FMath::RandRange(0.0f, 1.0f);
                p.color.b = FMath::RandRange(0.0f, 1.0f);
                pointCloud.push_back(p);
                minimum = glm::min(p.position, minimum);
                maximum = glm::max(p.position, maximum);
            }
            //TMap<FVector3f, FColor> vertexColorData;
            //mesh->GetVertexColorData(vertexColorData);
            //for (const auto& pair : vertexColorData)
            //{
            //    //const FVector3f& pos = pair.Key;
            //    const auto pos = comp->GetComponentToWorld().TransformPosition(FVector(pair.Key.X, pair.Key.Y, pair.Key.Z));
            //    const FColor& color = pair.Value;

            //    Point p;
            //    p.position.x = pos.X;
            //    p.position.y = pos.Y;
            //    p.position.z = pos.Z;
            //    p.color.r = FMath::RandRange(0.0f, 1.0f);
            //    p.color.g = FMath::RandRange(0.0f, 1.0f);
            //    p.color.b = FMath::RandRange(0.0f, 1.0f);
            //    pointCloud.push_back(p);
            //    minimum = glm::min(p.position, minimum);
            //    maximum = glm::max(p.position, maximum);
            //}
        }
    }
}

std::vector<uint32_t> VulkanApp::loadShader(const char* path)
{
    std::ifstream infile(path, std::ios::binary | std::ios::ate);
    if (!infile.good())
        throw std::runtime_error("Unable to open shader file \"" + std::string(path) + "\"");
    std::streamsize size = infile.tellg();
    std::vector<uint32_t> buffer((size + 3) / 4);
    infile.seekg(std::ios::beg);
    infile.read((char*)buffer.data(), size);
    return buffer;
}

FSlateBrush VulkanApp::createBrush(std::vector<uint32_t>& image, uint32_t width, uint32_t height)
{
    // 创建纹理资源
    UTexture2D* texture = UTexture2D::CreateTransient(width, height);// , EPixelFormat::PF_B8G8R8A8);
    if (!texture)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create texture."));
        return FSlateBrush();
    }
    texture->SRGB = 0;
    texture->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
    texture->AddToRoot();
    texture->UpdateResource();

    // 将压缩数据解压到纹理中
    FTexture2DMipMap& mipMap = texture->GetPlatformData()->Mips[0];
    void* textureData = mipMap.BulkData.Lock(LOCK_READ_WRITE);
    size_t count = (size_t)(sizeof(uint8) * 4 * width * height);
    FMemory::Memcpy(textureData, image.data(), count);
    mipMap.BulkData.Unlock();

    // 更新纹理资源的参数
    texture->UpdateResource();

    // 保存成文件
    //FString testfile = "E:/test.png";
    //FImageWriteOptions option;
    //option.bOverwriteFile = true;
    //UImageWriteBlueprintLibrary::ExportToDisk(texture, testfile, option);

    // 创建FSlateBrush
    FSlateBrush brush;
    brush.SetResourceObject(texture);
    brush.ImageSize.X = width;
    brush.ImageSize.Y = height;
    brush.DrawAs = ESlateBrushDrawType::Image;

    return brush;

    //return FSlateBrush();
}

