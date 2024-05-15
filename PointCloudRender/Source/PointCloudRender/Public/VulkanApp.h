// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>

//#include "Misc/MessageDialog.h"
#include "Styling/SlateBrush.h"


THIRD_PARTY_INCLUDES_START
#include <glm/glm.hpp>
#include <glm/matrix.hpp>
THIRD_PARTY_INCLUDES_END


namespace VulkanApp
{
    struct Point
    {
        alignas(16) glm::vec3 position;
        alignas(16) glm::vec3 color;
    };

    struct Parameters
    {
        alignas(16) uint32_t width;
        alignas(16) uint32_t height;
        alignas(16) uint32_t numPoints;
        alignas(16) glm::mat4 mvp;
    };


    void PointCloudRender(uint32_t width, uint32_t height, FSlateBrush& brush);

    void ReadPointCloud(const char* path, std::vector<Point>& pointCloud, glm::vec3& minimum, glm::vec3& maximum);
    void GetPointCloudFromWorld(std::vector<Point>& pointCloud, glm::vec3& minimum, glm::vec3& maximum);
    std::vector<uint32_t> loadShader(const char* path);
    FSlateBrush createBrush(std::vector<uint32_t>& image, uint32_t width, uint32_t height);
}




