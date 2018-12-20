#version 410

layout(location = 0) in vec2 texture_coord;

uniform sampler2D texture_image;
uniform ivec2 screenSize;
uniform int threshold_radius;

layout(location = 0) out vec4 out_color;

int sobel_kernel[9] = int[](-1, 0, 1, -2, 0, 2, -1, 0, 1);
vec2 texelSize = 1.0f / screenSize;

// Converts the color to grayscale
vec4 grayscale(vec4 color)
{
	float gray = 0.21 * color.r + 0.71 * color.g + 0.07 * color.b; 
	return vec4(gray, gray, gray,  0);
}

// Applies the sobel kernel over the pixel
vec4 sobel()
{
	vec4 sum_x = vec4(0);
	vec4 sum_y = vec4(0);

	for (int i = -1; i <= 1; i++)
	{
		for (int j = -1; j <= 1; j++)
		{
			// Get kernel coords from top left corner
			int kernel_i = i + 1;
			int kernel_j = j + 1;

			// Compute Dx and Dy
			// Convert the image to grayscale for better results
			sum_x += sobel_kernel[kernel_i * 3 + kernel_j] * grayscale(texture(texture_image, texture_coord + vec2(i, j) * texelSize));
			sum_y += sobel_kernel[kernel_j * 3 + kernel_i] * grayscale(texture(texture_image, texture_coord + vec2(i, j) * texelSize));
		}
	}

	return abs(sum_x) + abs(sum_y);
}

// Compute the average value of area on the selected channel
float avg(int channel, int radius)
{
	float sum = 0;
	for (int i = -radius; i <= radius; i++)
	{
		for (int j = -radius; j <= radius; j++)
		{
			sum += grayscale(texture(texture_image, texture_coord + vec2(i, j) * texelSize))[channel];
		}
	}
	
	float samples = pow((2 * radius + 1), 2);
	return sum / samples;
}

void main()
{
	// Apply sobel
	out_color = sobel();

	// Binarize output based on a local threshold
	out_color = out_color[0] < avg(0, threshold_radius)? vec4(0.0f) : vec4(1.0f);
}