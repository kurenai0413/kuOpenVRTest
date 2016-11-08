#version 410 core

uniform mat4 matrix;
uniform mat4 ViewMat;
uniform mat4 ProjMat;

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;

out vec3 ourColor;

void main()
{
	gl_Position = matrix * ProjMat * ViewMat * vec4(position, 1.0);
	ourColor = color;
}