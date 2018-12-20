#version 410

layout(location = 0) in vec2 texture_coord;

uniform sampler2D textureImage;
uniform ivec2 screenSize;
uniform int flipVertical;

// 0 - original
// 1 - grayscale
// 2 - blur
uniform int outputMode = 2;

// Flip texture horizontally when
vec2 textureCoord = vec2(texture_coord.x, (flipVertical != 0) ? 1 - texture_coord.y : texture_coord.y);

layout(location = 0) out vec4 out_color;

vec4 grayscale(vec4 color)
{
	float gray = 0.21 * color.r + 0.71 * color.g + 0.07 * color.b; 
	return vec4(gray, gray, gray,  0);
}

vec4 blur(int blurRadius)
{
	vec2 texelSize = 1.0f / screenSize;
	vec4 sum = vec4(0);
	for(int i = -blurRadius; i <= blurRadius; i++)
	{
		for(int j = -blurRadius; j <= blurRadius; j++)
		{
			sum += texture(textureImage, textureCoord + vec2(i, j) * texelSize);
		}
	}
		
	float samples = pow((2 * blurRadius + 1), 2);
	return sum / samples;
}

vec4 sobel_x()
{
	int sobel[9] = int[](-1, 0, 1, -2, 0, 2, -1, 0, 1);
	vec2 texelSize = 1.0f / screenSize;
	vec4 sum = vec4(0);
	for (int i = -1; i <= 1; i++)
	{
		for (int j = -1; j <= 1; j++)
		{
			int kernel_i = i + 1;
			int kernel_j = j + 1;
			sum += sobel[kernel_i * 3 + kernel_j] * grayscale(texture(textureImage, textureCoord + vec2(i, j) * texelSize));
		}
	}

	return sum;
}

vec4 sobel_y()
{
	int sobel[9] = int[](1, 2, 1, 0, 0, 0, -1, -2, -1);
	vec2 texelSize = 1.0f / screenSize;
	vec4 sum = vec4(0);
	for (int i = -1; i <= 1; i++)
	{
		for (int j = -1; j <= 1; j++)
		{
			int kernel_i = i + 1;
			int kernel_j = j + 1;
			sum += sobel[kernel_i * 3 + kernel_j] * grayscale(texture(textureImage, textureCoord + vec2(i, j) * texelSize));
		}
	}

	return sum;
}

float avg(int channel, int radius)
{
	vec2 texelSize = 1.0f / screenSize;
	float sum = 0;
	for (int i = -radius; i <= radius; i++)
	{
		for (int j = -radius; j <= radius; j++)
		{
			sum += grayscale(texture(textureImage, textureCoord + vec2(i, j) * texelSize))[channel];
		}
	}
	
	float samples = pow((2 * radius + 1), 2);
	return sum / samples;
}

void main()
{
	switch (outputMode)
	{
		case 1:
		{
			vec4 edge_color = abs(sobel_x()) + abs(sobel_y());

			edge_color = edge_color.r < avg(0, 11)? vec4(0.0f) : vec4(1.0f);

			out_color = texture(textureImage, textureCoord) - edge_color;
			out_color = floor(out_color * 9) / 9;
			break;
		}

		case 2:
		{
			out_color = blur(3);
			break;
		}

		default:
			out_color = texture(textureImage, textureCoord);
			break;
	}
}