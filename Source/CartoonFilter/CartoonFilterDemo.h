#pragma once

#include <Core/Engine.h>
#include <Component/SimpleScene.h>

class CartoonFilterDemo : public SimpleScene
{
public:
	CartoonFilterDemo();
	virtual ~CartoonFilterDemo();

	virtual void Init() override;

private:
	void FrameStart() override;
	void Update(float deltaTimeSeconds) override;
	void FrameEnd() override;

	// Input controls
	void OnKeyPress(int key, int mods) override;

	// Applies the sobel kernel to obtain the edges in the image
	void ApplySobel(Texture2D *image);

	// Dilates the given binary image
	void DilateImage(Texture2D *image);

	void ApplyColorLevels(Texture2D *image);

private:
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