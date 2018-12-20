#include "CartoonFilterDemo.h"

#include <vector>
#include <iostream>

CartoonFilterDemo::CartoonFilterDemo()
{
	colorLevels = 7;
	localThresholdRadius = 11;
	dilationRadius = 1;
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
	originalImage = TextureManager::LoadTexture(RESOURCE_PATH::ROOT + "Test Images/elizabeth.jpg", nullptr, "original", true, true);

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
		Shader *shader = new Shader("ColorQuantization");
		shader->AddShader((RESOURCE_PATH::SHADERS + "Demo/Pass.VS.glsl").c_str(), GL_VERTEX_SHADER);
		shader->AddShader((RESOURCE_PATH::SHADERS + "Demo/ColorQuantization.FS.glsl").c_str(), GL_FRAGMENT_SHADER);
		shader->CreateAndLink();
		shaders[shader->GetName()] = shader;
	}
}

void CartoonFilterDemo::FrameStart()
{
	sobelBuffer->Bind();
	ClearScreen();
}

void CartoonFilterDemo::Update(float deltaTimeSeconds)
{
	// Apply sobel to determine edges
	ApplySobel(originalImage);

	edgeBuffer->Bind();
	ClearScreen();

	// Dilate edges
	DilateImage(sobelBuffer->GetTexture(0));

	FrameBuffer::BindDefault();
	ClearScreen();

	// Combine outline with the original image to apply the filter
	ApplyColorLevels(edgeBuffer->GetTexture(0));
}

void CartoonFilterDemo::FrameEnd()
{
}

void CartoonFilterDemo::ApplySobel(Texture2D *image)
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

void CartoonFilterDemo::DilateImage(Texture2D *image)
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

void CartoonFilterDemo::ApplyColorLevels(Texture2D *image)
{
	Shader *shader = shaders["ColorQuantization"];

	if (!image || !shader || !shader->program)
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
	originalImage->BindToTextureUnit(GL_TEXTURE0);

	// Send image to shader
	locTexture = shader->GetUniformLocation("edge_image");
	glUniform1i(locTexture, 1);
	image->BindToTextureUnit(GL_TEXTURE1);

	RenderMesh(meshes["quad"], shader, glm::mat4(1.0f));

	image->UnBind();
}

void CartoonFilterDemo::OnKeyPress(int key, int mods)
{
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
