#pragma once

#include <Core/Engine.h>
#include <Component/SimpleScene.h>
#include <CartoonFilter\WinAPIFileBrowser.h>

class CartoonFilterDemo : public SimpleScene
{
public:
	CartoonFilterDemo();
	virtual ~CartoonFilterDemo();

	virtual void Init() override;

private:
	enum Mode { SIMPLE = 0, GPU = 1, CPU = 2 };

	void FrameStart() override;
	void Update(float deltaTimeSeconds) override;
	void FrameEnd() override;

	// Input controls
	void OnKeyPress(int key, int mods) override;

	// Renders an image on the screen using the basic shader
	void RenderImage(Texture2D *image);

	// Applies the filter on the current image using 
	// Color Quantization on the GPU
	void RenderOnGpu();

	// Applies the filter using segmentation on CPU
	void RenderOnCpu();

	// Converts an RGB image to grayscale
	void Grayscale(Texture2D *image);

	// Applies the given kernel over the image at the 
	// given positin. If none kernel is given, 
	// a simple one (filled with 1) will be used
	glm::vec3 ApplyKernel(Texture2D *image, int posX, int posY, int *kernel, int radius);

	// Applies the sobel kernel to obtain the edges in the image
	void ApplySobelGpu(Texture2D *image);
	void ApplySobelCpu(Texture2D *image);

	// Dilates the given binary image
	void DilateImageGpu(Texture2D *image);
	void DilateImageCpu(Texture2D *image);

	// Adds up the 2 images
	void CombineImages(Texture2D *image1, Texture2D *image2, bool subtract = false);

	// Color Quantization of the image
	void ApplyCartoonShader(Texture2D *original, Texture2D *edgeImage);

	// Segmentation of the image based on color
	void ApplySegmentation(Texture2D *image);

	// Adjust the window size to match the aspect ratio
	void AdjustWindow();

	// Opens a new file browser window and opens the selected image
	void SelectImage();

	// Resets the processed image back to the original version
	void ResetToOriginal();

private:
	// Default Window Size
	glm::ivec2 windowSize;

	// Processing options
	Mode mode;
	bool processed;

	// Filter parameters
	int localThresholdRadius;
	int colorLevels;
	int dilationRadius;

	// Image
	Texture2D *originalImage;
	Texture2D *processedImage;

	// Frame Buffer
	std::unique_ptr<FrameBuffer> sobelBuffer;
	std::unique_ptr<FrameBuffer> edgeBuffer;
};