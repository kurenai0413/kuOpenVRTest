#version 410 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec2 texCoord;

out vec2 TexCoord;

uniform mat4 matrix;
uniform mat4 TransCT2Model;
uniform mat4 ModelMat;
uniform mat4 ViewMat;
uniform mat4 ProjMat;
								   
void main()
{
	gl_Position = ProjMat * matrix * ViewMat * ModelMat * TransCT2Model * vec4(position, 1.0);
	TexCoord = texCoord;
}