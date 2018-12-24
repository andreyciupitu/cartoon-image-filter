#version 410

layout(location = 0) in vec2 texture_coord;

uniform sampler2D texture_image;
uniform sampler2D edge_image;

uniform int flip;
uniform int color_levels;

layout(location = 0) out vec4 out_color;

void main()
{
	vec2 flipped_coord = texture_coord;
	if (flip == 1)
	{
		flipped_coord.y = 1 - texture_coord.y;
	}

	// Subtract the edges t make them black
	out_color = texture(texture_image, flipped_coord) - texture(edge_image, flipped_coord);

	// Assign the color to the nearest level
	out_color = floor(out_color * color_levels) / color_levels;
}