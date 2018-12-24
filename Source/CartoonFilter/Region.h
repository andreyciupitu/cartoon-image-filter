#pragma once

#include <vector>

#include <Core\Engine.h>

class Region
{
public:
	static const float GLOBAL_THRESHOLD;

public:
	Region() : avg(glm::vec3(0.0f)), sqrDev(glm::vec3(0.0f)), pixelCount(0) {}

public:
	// Returns the average intensity of the region
	glm::vec3 GetAvg();

	// Adds a pixel to the region
	void AddPixel(glm::vec3 pixel);

	// Check if a pixel is similar to the others in the region
	bool CheckIfSimilar(glm::vec3 pixel);

private:
	int pixelCount;

	glm::vec3 avg;
	glm::vec3 sqrDev;
};