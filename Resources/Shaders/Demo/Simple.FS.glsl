#version 410

layout(location = 0) in vec2 texture_coord;

uniform sampler2D texture_image;

uniform int flip;

layout(location = 0) out vec4 out_color;

void main()
{
	vec2 flipped_coord = texture_coord;
	if (flip == 1)
	{
		flipped_coord.y = 1 - texture_coord.y;
	}

	out_color = texture(texture_image, flipped_coord);
}