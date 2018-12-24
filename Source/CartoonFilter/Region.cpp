#include "Region.h"

const float Region::GLOBAL_THRESHOLD = 60.0f;

void Region::AddPixel(glm::vec3 pixel)
{
	float n = pixelCount++;
	avg = (avg * n + pixel) / (n + 1);

	if (n > 0)
		sqrDev = sqrDev * (n - 1) / n + (pixel - avg) * (pixel - avg) / (n + 1);
}

bool Region::CheckIfSimilar(glm::vec3 pixel)
{
	float n = pixelCount;

	// Compute average with the new intensity
	glm::vec3 testAvg = (avg * n + pixel) / (n + 1);
	
	// Compute standard deviation with the new intensity
	glm::vec3 testDev = sqrDev * (n - 1) / n + (pixel - avg) * (pixel - avg) / (n + 1);
	testDev = sqrt(testDev);

	return glm::distance(pixel, avg) < (1 - glm::length(testDev / testAvg)) * GLOBAL_THRESHOLD;
}

glm::vec3 Region::GetAvg()
{
	return avg;
}