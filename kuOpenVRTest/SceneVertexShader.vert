#version 410 core

uniform mat4 matrix;
uniform mat4 ViewMat;
uniform mat4 ProjMat;

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;

out vec3 ourColor;
out vec2 TexCoord;

void main()
{
	gl_Position = matrix * ProjMat * ViewMat * vec4(position, 1.0);
	ourColor = vec3(1.0, 1.0, 1.0);
	TexCoord = texCoord;
}