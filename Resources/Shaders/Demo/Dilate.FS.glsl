#version 410

layout(location = 0) in vec2 texture_coord;

uniform sampler2D binary_image;
uniform ivec2 screenSize;
uniform int radius;

layout(location = 0) out vec4 out_color;

void main()
{
	vec2 texelSize = 1.0f / screenSize;

	// Add up all the neighbours and determine the resulting color
	out_color = vec4(0);
	for (int i = -radius; i <= radius; i++)
	{
		for (int j = -radius; j <= radius; j++)
		{
			out_color += texture(binary_image, texture_coord + vec2(i, j) * texelSize);
		}
	}
}