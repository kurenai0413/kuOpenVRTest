#version 410 core

uniform mat4 matrix;		// OpenVR MVP
uniform mat4 ModelMat;
uniform mat4 ViewMat;
uniform mat4 ProjMat;

layout (location = 0) in vec3 Position_VS_In;
layout (location = 1) in vec3 Normal_VS_In;
layout (location = 2) in vec2 TexCoord_VS_In;

//out vec3 ourColor;
out vec3 WorldPos_TCS_In;		// Replace original gl_Position
out vec3 Normal_TCS_In;
out vec2 TexCoord_TCS_In;
out vec3 FragPos;

void main()
{
	//gl_Position = ProjMat *  matrix /** ViewMat*/ * ModelMat * vec4(position, 1.0);

	WorldPos_TCS_In = ModelMat * vec4(position_VS_In, 1.0);
	TexCoord_TCS_In = TexCoord_VS_In;
	Normal_TCS_In   = mat3(transpose(inverse(ModelMat))) * vec4(Normal_VS_In, 0.0);

	//Normal = vec3(-Normal.x, -Normal.y, -Normal.z);
}