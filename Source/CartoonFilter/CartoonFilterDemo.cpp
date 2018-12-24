#include "CartoonFilterDemo.h"

#include <CartoonFilter\Region.h>

#include <vector>
#include <iostream>

CartoonFilterDemo::CartoonFilterDemo()
{
	colorLevels = 7;
	localThresholdRadius = 5;
	dilationRadius = 1;
	mode = Mode::CPU;
	processed = true;
	windowSize = glm::ivec2(1280, 720);
}

CartoonFilterDemo::~CartoonFilterDemo()
{
}

void CartoonFilterDemo::Init()
{
	// Init frame buffers ----------------------------------------------------------
	glm::vec2 resolution = window->GetResolution();
	sobelBuffer = std::unique_ptr<FrameBuffer>(new FrameBuffer());
	sobelBuffer->Generate(resolution.x, resolution.y, 1);	
	
	edgeBuffer = std::unique_ptr<FrameBuffer>(new FrameBuffer());
	edgeBuffer->Generate(resolution.x, resolution.y, 1);

	// Implicit image --------------------------------------------------------------
	SelectImage();

	// Load a simple quad mesh -----------------------------------------------------
	{
		Mesh* mesh = new Mesh("quad");
		mesh->LoadMesh(RESOURCE_PATH::MODELS + "Primitives", "screen_quad.obj");
		mesh->UseMaterials(false);
		meshes[mesh->GetMeshID()] = mesh;
	}

	// Sobel Filter Shader ---------------------------------------------------------
	{
		Shader *shader = new Shader("Sobel");
		shader->AddShader((RESOURCE_PATH::SHADERS + "Demo/Pass.VS.glsl").c_str(), GL_VERTEX_SHADER);
		shader->AddShader((RESOURCE_PATH::SHADERS + "Demo/Sobel.FS.glsl").c_str(), GL_FRAGMENT_SHADER);
		shader->CreateAndLink();
		shaders[shader->GetName()] = shader;
	}

	// Image dilatation shader -----------------------------------------------------
	{
		Shader *shader = new Shader("Dilation");
		shader->AddShader((RESOURCE_PATH::SHADERS + "Demo/Pass.VS.glsl").c_str(), GL_VERTEX_SHADER);
		shader->AddShader((RESOURCE_PATH::SHADERS + "Demo/Dilate.FS.glsl").c_str(), GL_FRAGMENT_SHADER);
		shader->CreateAndLink();
		shaders[shader->GetName()] = shader;
	}

	// Color quantization shader ---------------------------------------------------
	{
		Shader *shader = new Shader("Cartoon");
		shader->AddShader((RESOURCE_PATH::SHADERS + "Demo/Pass.VS.glsl").c_str(), GL_VERTEX_SHADER);
		shader->AddShader((RESOURCE_PATH::SHADERS + "Demo/Cartoon.FS.glsl").c_str(), GL_FRAGMENT_SHADER);
		shader->CreateAndLink();
		shaders[shader->GetName()] = shader;
	}

	// Simple shader ---------------------------------------------------------------
	{
		Shader *shader = new Shader("Basic");
		shader->AddShader((RESOURCE_PATH::SHADERS + "Demo/Pass.VS.glsl").c_str(), GL_VERTEX_SHADER);
		shader->AddShader((RESOURCE_PATH::SHADERS + "Demo/Simple.FS.glsl").c_str(), GL_FRAGMENT_SHADER);
		shader->CreateAndLink();
		shaders[shader->GetName()] = shader;
	}
}

void CartoonFilterDemo::FrameStart()
{
	FrameBuffer::BindDefault();
	ClearScreen();
}

void CartoonFilterDemo::Update(float deltaTimeSeconds)
{
	switch (mode)
	{
	case SIMPLE:
		RenderImage(originalImage);
		break;
	case GPU:
		RenderOnGpu();
		break;
	case CPU:
		RenderOnCpu();
		break;
	default:
		break;
	}
}

void CartoonFilterDemo::FrameEnd()
{
}

void CartoonFilterDemo::RenderImage(Texture2D *image)
{
	Shader *shader = shaders["Basic"];

	if (!image || !shader || !shader->program)
		return;

	shader->Use();

	// Flip the image coz tex_coords are inversed 
	int flip_loc = shader->GetUniformLocation("flip");
	glUniform1i(flip_loc, 1);

	// Send image to shader
	int locTexture = shader->GetUniformLocation("texture_image");
	glUniform1i(locTexture, 0);
	image->BindToTextureUnit(GL_TEXTURE0);

	RenderMesh(meshes["quad"], shader, glm::mat4(1.0f));

	image->UnBind();
}

void CartoonFilterDemo::RenderOnGpu()
{	
	sobelBuffer->Bind();
	ClearScreen();

	// Apply sobel to determine edges
	ApplySobelGpu(originalImage);

	edgeBuffer->Bind();
	ClearScreen();

	// Dilate edges
	DilateImageGpu(sobelBuffer->GetTexture(0));

	FrameBuffer::BindDefault();
	ClearScreen();

	// Combine outline with the original image to apply the filter
	ApplyCartoonShader(originalImage, edgeBuffer->GetTexture(0));
}

void CartoonFilterDemo::ApplySobelGpu(Texture2D *image)
{
	Shader *shader = shaders["Sobel"];

	if (!image || !shader || !shader->program)
		return;

	shader->Use();

	// Send resolution
	int screenSize_loc = shader->GetUniformLocation("screenSize");
	glm::ivec2 resolution = window->GetResolution();
	glUniform2i(screenSize_loc, resolution.x, resolution.y);

	// Send local threshold radius
	int threshold_radius_loc = shader->GetUniformLocation("threshold_radius");
	glUniform1i(threshold_radius_loc, localThresholdRadius);

	// Send image to shader
	int locTexture = shader->GetUniformLocation("texture_image");
	glUniform1i(locTexture, 0);
	image->BindToTextureUnit(GL_TEXTURE0);

	RenderMesh(meshes["quad"], shader, glm::mat4(1.0f));

	image->UnBind();
}

void CartoonFilterDemo::DilateImageGpu(Texture2D *image)
{
	Shader *shader = shaders["Dilation"];

	if (!image || !shader || !shader->program)
		return;
	
	shader->Use();

	// Send resolution
	int screenSize_loc = shader->GetUniformLocation("screenSize");
	glm::ivec2 resolution = window->GetResolution();
	glUniform2i(screenSize_loc, resolution.x, resolution.y);

	// Send local threshold radius
	int radius_loc = shader->GetUniformLocation("radius");
	glUniform1i(radius_loc, dilationRadius);

	// Send image to shader
	int locTexture = shader->GetUniformLocation("binary_image");
	glUniform1i(locTexture, 0);
	image->BindToTextureUnit(GL_TEXTURE0);

	RenderMesh(meshes["quad"], shader, glm::mat4(1.0f));

	image->UnBind();
}

void CartoonFilterDemo::ApplyCartoonShader(Texture2D *original, Texture2D *edgeImage)
{
	Shader *shader = shaders["Cartoon"];

	if (!edgeImage || !original || !shader || !shader->program)
		return;

	shader->Use();

	// Send resolution
	int screenSize_loc = shader->GetUniformLocation("screenSize");
	glm::ivec2 resolution = window->GetResolution();
	glUniform2i(screenSize_loc, resolution.x, resolution.y);

	// Flip the image coz tex_coords are inversed 
	int flip_loc = shader->GetUniformLocation("flip");
	glUniform1i(flip_loc, 1);	
	
	// Send color levels 
	int levels_loc = shader->GetUniformLocation("color_levels");
	glUniform1i(levels_loc, colorLevels);

	// Send image to shader
	int locTexture = shader->GetUniformLocation("texture_image");
	glUniform1i(locTexture, 0);
	original->BindToTextureUnit(GL_TEXTURE0);

	// Send image to shader
	locTexture = shader->GetUniformLocation("edge_image");
	glUniform1i(locTexture, 1);
	edgeImage->BindToTextureUnit(GL_TEXTURE1);

	RenderMesh(meshes["quad"], shader, glm::mat4(1.0f));

	edgeImage->UnBind();
	original->UnBind();
}

void CartoonFilterDemo::RenderOnCpu()
{
	// Process only once
	if (!processed)
	{
		processed = true;

		// Convert image to grayscale
		Grayscale(processedImage);

		// Get edges
		ApplySobelCpu(processedImage);

		// Dilate edges
		DilateImageCpu(processedImage);

		// Add edges over the original image
		CombineImages(originalImage, processedImage, true);

		// Segmentation
		ApplySegmentation(processedImage);
	}

	RenderImage(processedImage);
}

glm::vec3 CartoonFilterDemo::ApplyKernel(Texture2D *image, int posY, int posX, int *kernel, int radius)
{
	// Get image data
	unsigned int channels = image->GetNrChannels();
	unsigned char *data = image->GetImageData();

	glm::ivec2 imageSize = glm::ivec2(image->GetWidth(), image->GetHeight());

	int stride = 2 * radius + 1;
	glm::vec3 sum = glm::vec3(0.0f);

	// Apply the kernel centered on the pixel
	for (int k = -radius; k <= radius; k++)
	{
		if (posY + k < 0 || posY + k >= imageSize.y)
			continue;

		for (int l = -radius; l <= radius; l++)
		{
			if (posX + l < 0 || posX + l >= imageSize.x)
				continue;

			int offset = channels * ((posY + k) * imageSize.x + (posX + l));

			// Get kernel coords from top left corner
			int kernel_i = k + 1;
			int kernel_j = l + 1;

			glm::vec3 color = glm::vec3(data[offset], data[offset + 1], data[offset + 2]);

			// Use kernel filled with 1s if no kernel is provided
			if (kernel == nullptr)
				sum += color;
			else
				sum += (float)kernel[kernel_i * stride + kernel_j] * color;
		}
	}

	return sum;
}

void CartoonFilterDemo::ApplySobelCpu(Texture2D *image)
{
	if (!image)
		return;

	// Sobel Kernels
	int sobelX[9] = { -1, 0, 1, -2, 0, 2, -1, 0, 1 };
	int sobelY[9] = { 1, 2, 1, 0, 0, 0, -1, -2, -1 };

	// Get image data
	unsigned int channels = image->GetNrChannels();
	unsigned char *data = image->GetImageData();
	glm::ivec2 imageSize = glm::ivec2(image->GetWidth(), image->GetHeight());

	if (channels < 3)
		return;

	// Work copy of the image
	unsigned char *newData = new unsigned char[imageSize.x * imageSize.y * channels];
	
	for (int i = 0; i < imageSize.y; i++)
	{
		for (int j = 0; j < imageSize.x; j++)
		{
			// Compute Dx & Dy
			glm::vec3 sumX = ApplyKernel(image, i, j, sobelX, 1);
			glm::vec3 sumY = ApplyKernel(image, i, j, sobelY, 1);

			// Compute Sobel result
			glm::vec3 result = abs(sumX) + abs(sumY);

			// Compute the average value of the local area
			// to use as a threshold for binarization
			glm::vec3 threshold = ApplyKernel(image, i, j, nullptr, localThresholdRadius);
			threshold /= (2 * localThresholdRadius + 1) * (2 * localThresholdRadius + 1);

			// Binarize the Sobel result
			unsigned char value = static_cast<unsigned char>(result.x >= threshold.x ? 255 : 0);

			// Write new data
			memset(&newData[channels * (i * imageSize.x + j)], value, 3);
		}
	}

	// Write back the data in the image
	for (int i = 0; i < imageSize.y; i++)
	{
		for (int j = 0; j < imageSize.x; j++)
		{
			int offset = channels * (i * imageSize.x + j);
			memcpy(&data[offset], &newData[offset], 3);
		}
	}
	image->UploadNewData(data);

	delete newData;
}

void CartoonFilterDemo::DilateImageCpu(Texture2D *image)
{
	// Get image data
	unsigned int channels = image->GetNrChannels();
	unsigned char *data = image->GetImageData();
	glm::ivec2 imageSize = glm::ivec2(image->GetWidth(), image->GetHeight());

	if (channels < 3)
		return;

	// Work copy of the image
	unsigned char *newData = new unsigned char[imageSize.x * imageSize.y * channels];

	for (int i = 0; i < imageSize.y; i++)
	{
		for (int j = 0; j < imageSize.x; j++)
		{
			// Apply a simple kernel
			glm::vec3 result = ApplyKernel(image, i, j, nullptr, dilationRadius);

			// Clamp the result in 0 - 255
			unsigned char value = static_cast<unsigned char>(result.x > 255 ? 255 : result.x);

			// Set the new value
			memset(&newData[channels * (i * imageSize.x + j)], value, 3);
		}
	}

	// Write back the data in the image
	for (int i = 0; i < imageSize.y; i++)
	{
		for (int j = 0; j < imageSize.x; j++)
		{
			int offset = channels * (i * imageSize.x + j);
			memcpy(&data[offset], &newData[offset], 3);
		}
	}
	image->UploadNewData(data);

	delete newData;
}

void CartoonFilterDemo::CombineImages(Texture2D *image1, Texture2D *image2, bool subtract)
{	
	// Get image data
	unsigned int channels = image1->GetNrChannels();
	unsigned char *data1 = image1->GetImageData();
	unsigned char *data2 = image2->GetImageData();
	glm::ivec2 imageSize = glm::ivec2(image1->GetWidth(), image1->GetHeight());

	if (channels < 3)
		return;

	for (int i = 0; i < imageSize.y; i++)
	{
		for (int j = 0; j < imageSize.x; j++)
		{
			int offset = channels * (i * imageSize.x + j);
			glm::ivec3 color1 = glm::vec3(data1[offset], data1[offset + 1], data1[offset + 2]);
			glm::ivec3 color2 = glm::vec3(data2[offset], data2[offset + 1], data2[offset + 2]);

			// Combine the colors
			glm::ivec3 resultingPixel;
			if (subtract)
			{
				resultingPixel = color1 - color2; 
			}
			else
			{
				resultingPixel = color1 + color2;
			}
			resultingPixel = glm::clamp(resultingPixel, glm::ivec3(0), glm::ivec3(255));

			// Write back the values in the first image
			data2[offset] = static_cast<unsigned char>(resultingPixel.x);
			data2[offset + 1] = static_cast<unsigned char>(resultingPixel.y);
			data2[offset + 2] = static_cast<unsigned char>(resultingPixel.z);
		}
	}

	image2->UploadNewData(data2);
}

void CartoonFilterDemo::ApplySegmentation(Texture2D *image)
{
	// Get image data
	unsigned int channels = image->GetNrChannels();
	unsigned char *data = image->GetImageData();
	glm::ivec2 imageSize = glm::ivec2(image->GetWidth(), image->GetHeight());

	if (channels < 3)
		return;

	std::vector< std::vector<Region*> > regions;

	// Store all the unique regions for later deletion
	std::vector<Region*> uniqueRegions;

	for (int i = 0; i < imageSize.y; i++)
	{
		regions.push_back(std::vector<Region*>());

		for (int j = 0; j < imageSize.x; j++)
		{
			// Get current pixel
			int offset = channels * (i * imageSize.x + j);
			glm::vec3 color = glm::vec3(data[offset], data[offset + 1], data[offset + 2]);

			Region* newRegion = nullptr;

			// TopLeft neighbour
			if (i > 0 && j > 0 && regions[i - 1][j - 1]->CheckIfSimilar(color))
			{
				newRegion = regions[i - 1][j - 1];
			}

			// Top neighbour
			if (i > 0 && regions[i - 1][j]->CheckIfSimilar(color))
			{
				float diff = glm::distance(color, regions[i - 1][j]->GetAvg());

				// Assign only if distance is less than the previous region
				if (newRegion == nullptr || glm::distance(color, newRegion->GetAvg()) > diff)
				{
					newRegion = regions[i - 1][j];
				}
			}

			// Left neighbour
			if (j > 0 && regions[i][j - 1]->CheckIfSimilar(color))
			{
				float diff = glm::distance(color, regions[i][j - 1]->GetAvg());

				// Assign only if distance is less than the previous region
				if (newRegion == nullptr || glm::distance(color, newRegion->GetAvg()) > diff)
				{
					newRegion = regions[i][j - 1];
				}
			}

			// Assign the current pixel to a region
			if (newRegion == nullptr)
			{
				regions[i].push_back(new Region());
				uniqueRegions.push_back(regions[i][j]);
				regions[i][j]->AddPixel(color);
			}
			else
			{
				regions[i].push_back(newRegion);
				regions[i][j]->AddPixel(color);
			}
		}
	}

	// Blend the pixels in each region
	for (int i = 0; i < imageSize.y; i++)
	{
		for (int j = 0; j < imageSize.x; j++)
		{
			int offset = channels * (i * imageSize.x + j);

			// Get average color
			glm::vec3 avg = regions[i][j]->GetAvg();

			// The new color will be the average of the region
			data[offset] = static_cast<unsigned char>(avg.x);
			data[offset + 1] = static_cast<unsigned char>(avg.y);
			data[offset + 2] = static_cast<unsigned char>(avg.z);
		}
	}

	// Delete regions
	for (auto r : uniqueRegions)
	{
		delete r;
	}

	image->UploadNewData(data);
}

void CartoonFilterDemo::Grayscale(Texture2D *image)
{
	// Get image data
	unsigned int channels = image->GetNrChannels();
	unsigned char* data = image->GetImageData();

	glm::ivec2 imageSize = glm::ivec2(originalImage->GetWidth(), originalImage->GetHeight());

	if (channels < 3)
		return;

	for (int i = 0; i < imageSize.y; i++)
	{
		for (int j = 0; j < imageSize.x; j++)
		{
			int offset = channels * (i * imageSize.x + j);

			// Convert color to grayscale value 
			char value = static_cast<char>(data[offset + 0] * 0.21f + data[offset + 1] * 0.71f + data[offset + 2] * 0.07);
			memset(&data[offset], value, 3);
		}
	}
	image->UploadNewData(data);
}

void CartoonFilterDemo::AdjustWindow()
{
	float aspectRatio = static_cast<float>(originalImage->GetWidth()) / originalImage->GetHeight();
	window->SetSize(static_cast<int>(windowSize.y * aspectRatio), windowSize.y);

	// Resize frame buffers
	glm::vec2 resolution = window->GetResolution();
	sobelBuffer->Resize(resolution.x, resolution.y);
	edgeBuffer->Resize(resolution.x, resolution.y);
}

void CartoonFilterDemo::SelectImage()
{
	// Open a new file browser
	std::string newImage = FileBrowser::OpenDialogue();

	if (newImage.empty())
		return;

	originalImage = TextureManager::LoadTexture(newImage.c_str(), nullptr, "original", true, true);
	processedImage = TextureManager::LoadTexture(newImage.c_str(), nullptr, "processed", true, true);

	// Adjust window to match aspect ratio
	AdjustWindow();

	processed = false;
}

void CartoonFilterDemo::ResetToOriginal()
{	
	// Get image data
	unsigned int channels = processedImage->GetNrChannels();
	unsigned char *data = processedImage->GetImageData();
	unsigned char *originalData = originalImage->GetImageData();

	glm::ivec2 imageSize = glm::ivec2(processedImage->GetWidth(), processedImage->GetHeight());

	if (channels < 3)
		return;
	
	// Write back the data in the image
	for (int i = 0; i < imageSize.y; i++)
	{
		for (int j = 0; j < imageSize.x; j++)
		{
			int offset = channels * (i * imageSize.x + j);
			memcpy(&data[offset], &originalData[offset], 3);
		}
	}
	processedImage->UploadNewData(data);
}

void CartoonFilterDemo::OnKeyPress(int key, int mods)
{
	// Change rendering mode
	if (key == GLFW_KEY_SPACE)
	{
		mode = (Mode)((mode + 1) % 3);
	}

	// Select a new image
	if (key == GLFW_KEY_ENTER)
	{
		SelectImage();
	}

	// Reload image on CPU
	if (key == GLFW_KEY_R && mode == Mode::CPU)
	{
		processed = false;
		ResetToOriginal();
	}

	// Can only modify parameters in GPU mode
	if (mode != Mode::GPU)
	{
		return;
	}

	// Outline modifier
	if (key == GLFW_KEY_P)
	{
		dilationRadius++;
	}
	if (key == GLFW_KEY_O)
	{
		dilationRadius--;
		dilationRadius = dilationRadius < 0 ? 0 : dilationRadius;
	}

	// Color levels
	if (key == GLFW_KEY_M)
	{
		colorLevels++;
	}
	if (key == GLFW_KEY_N)
	{
		colorLevels--;
		colorLevels = colorLevels < 0 ? 0 : colorLevels;
	}

	// Binarization threshold
	if (key == GLFW_KEY_KP_ADD)
	{
		localThresholdRadius++;
	}
	if (key == GLFW_KEY_KP_SUBTRACT)
	{
		localThresholdRadius--;
		localThresholdRadius = localThresholdRadius < 0 ? 0 : localThresholdRadius;
	}
}
