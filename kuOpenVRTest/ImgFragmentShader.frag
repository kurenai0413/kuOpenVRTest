#version 410 core
in	vec3 ourColor;
in	vec2 TexCoord;

out vec4 color;

uniform sampler2D ourTexture;

void main()
{
	color = texture(ourTexture, TexCoord);
	if (color.r == 0 && color.g == 0 && color.b == 0)
	{
		color.a = 0.15;
	}
	else
	{
		color.a = 0.85;
	}
}