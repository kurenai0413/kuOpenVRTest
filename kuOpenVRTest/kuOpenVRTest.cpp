#include <GLEW/glew.h>
#include <GLEW/wglew.h>
#include <GLFW/glfw3.h> 
#include <GLM/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "kuShaderHandler.h"

#include <OpenVR.h>
#include <opencv2/opencv.hpp>

#include <cstring>
#include <cassert>
#include <iostream>
#include <fstream>
#include <vector>

#include "vectors.h"
#include "Matrices.h"

#ifdef _DEBUG
#pragma comment(lib, "opengl32")
#pragma comment(lib, "glew32d")
#pragma comment(lib, "glfw3")
#pragma comment(lib, "opencv_world310d")
#pragma comment(lib, "openvr_api")
#endif

//#define _VR

#define pi			3.1415926

#define	numEyes		2

#define Left		0
#define Right		1

#define nearPlaneZ	0.1
#define farPlaneZ	100

using namespace std;
using namespace cv;

GLFWwindow		*	window = nullptr;
vr::IVRSystem	*	hmd    = nullptr;

kuShaderHandler		SceneShaderHandler;
kuShaderHandler		DistortShaderHandler;

#pragma region // Frame Buffer Containers
/////////////////////////////////////////////////////////////////////////////////////////
struct FrameBufferDesc
{
	GLuint	SceneTextureId;
	GLuint	SceneFrameBufferId;
	GLuint	DistortedTextureId;
	GLuint	DistortedFrameBufferId;
};

FrameBufferDesc	EyeFrameDesc[2];
/////////////////////////////////////////////////////////////////////////////////////////
#pragma endregion

uint32_t	framebufferWidth  = 1280;
uint32_t	framebufferHeight = 720;
int			windowWidth		  = 1280;
int			windowHeight	  = 720;

GLuint		FrameBufferID[numEyes];
GLuint		SceneTextureID[numEyes];
GLuint		DistortedFrameBufferId[numEyes];
GLuint		DistortedTexutreId[numEyes];
GLuint		depthRenderTarget[numEyes];

Matrix4		ProjectionMat[2];
Matrix4		EyePoseMat[2];  
Matrix4		MVPMat[2];

#pragma region // Lens distortion variables
/////////////////////////////////////////////////////////////////////////////////////////
struct VertexDataLens				// Vertex data for lens
{
	Vector2 position;
	Vector2 texCoordRed;
	Vector2 texCoordGreen;
	Vector2 texCoordBlue;
};

GLuint		 m_unLensVAO;
GLuint		 m_glIDVertBuffer;
GLuint		 m_glIDIndexBuffer;
unsigned int m_uiIndexSize;
/////////////////////////////////////////////////////////////////////////////////////////
#pragma endregion

							   // position	         // color
float	TriangleVertexs[] = {   0.0f,  1.0f, 0.0f,   1.0f, 0.0f, 0.0f,
							    1.0f, -1.0f, 0.0f,   0.0f, 1.0f, 0.0f,
							   -1.0f, -1.0f, 0.0f,   0.0f, 0.0f, 1.0f };


const GLfloat	vertices[]
= {
	// Frontal Face
	-0.5f,  0.5f,  0.5f,			// 0:  Top Left  
	 0.5f,  0.5f,  0.5f,			// 1:  Top Right
	 0.5f, -0.5f,  0.5f,			// 2:  Bottom Right
	-0.5f, -0.5f,  0.5f,			// 3:  Bottom Left

	// Right Face
	-0.5f,  0.5f, -0.5f,			// 4:  Top Left
	-0.5f,  0.5f,  0.5f,			// 5:  Top Right
	-0.5f, -0.5f,  0.5f,			// 6:  Bottom Right
	-0.5f, -0.5f, -0.5f,			// 7:  Bottom Left

	// Back face
	 0.5f,  0.5f, -0.5f,			// 8:  Top Left 
	-0.5f,  0.5f, -0.5f,			// 9:  Top Right
	-0.5f, -0.5f, -0.5f,			// 10: Bottom Right
	 0.5f, -0.5f, -0.5f,			// 11: Bottom Left

	// Left Face
	0.5f,  0.5f,  0.5f,			    // 12: Top Left 
	0.5f,  0.5f, -0.5f, 			// 13: Top Right
	0.5f, -0.5f, -0.5f, 			// 14: Bottom Right
	0.5f, -0.5f,  0.5f,  			// 15: Bottom Left

	// Up Face
	-0.5f,  0.5f, -0.5f,  		    // 16: Top Left 
	 0.5f,  0.5f, -0.5f,  		    // 17: Top Right
	 0.5f,  0.5f,  0.5f,  		    // 18: Bottom Right
	-0.5f,  0.5f,  0.5f,  		    // 19: Bottom Left

	// Down Face
	-0.5f, -0.5f,  0.5f,			// 20: Top Left 
	 0.5f, -0.5f,  0.5f,  			// 21: Top Right
	 0.5f, -0.5f, -0.5f,    		// 22: Bottom Right
	-0.5f, -0.5f, -0.5f  			// 23: Bottom Left
};

const GLfloat   texCoords[]
= {
	0.0f, 0.0f,    1.0f, 0.0f,    1.0f, 1.0f,    0.0f, 1.0f,
	0.0f, 0.0f,    1.0f, 0.0f,    1.0f, 1.0f,    0.0f, 1.0f,
	0.0f, 0.0f,    1.0f, 0.0f,    1.0f, 1.0f,    0.0f, 1.0f,
	0.0f, 0.0f,    1.0f, 0.0f,    1.0f, 1.0f,    0.0f, 1.0f,
	0.0f, 0.0f,    1.0f, 0.0f,    1.0f, 1.0f,    0.0f, 1.0f,
	0.0f, 0.0f,    1.0f, 0.0f,    1.0f, 1.0f,    0.0f, 1.0f
};

const GLuint    indices[]
= {
	// Frontal 
	0,  1,  3,	  1,  2,  3,
	// Right
	4,  5,  7,	  5,  6,  7,
	// Back
	8,  9, 11,	  9, 10, 11,
	// Left
	12, 13, 15,  13, 14, 15,
	// Up
	16, 17, 19,  17, 18, 19,
	// Down 
	20, 21, 23,  21, 22, 23
};


void Init();
GLFWwindow		*	initOpenGL(int width, int height, const std::string& title);
vr::IVRSystem	*	initOpenVR(uint32_t& hmdWidth, uint32_t& hmdHeight);
std::string getHMDString(vr::IVRSystem* pHmd, vr::TrackedDeviceIndex_t unDevice, 
						 vr::TrackedDeviceProperty prop, 
						 vr::TrackedPropertyError* peError = nullptr);

Matrix4 GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye);
Matrix4 GetHMDMatrixPoseEye(vr::Hmd_Eye nEye);

void WriteProjectionMatrixFile(char * Filename, vr::HmdMatrix44_t ProjMat);
void WriteEyePoseMatrixFile(char * Filename, vr::HmdMatrix34_t PoseMat);
void WriteMVPMatrixFile(char * Filename, Matrix4 matMVP);
void SetMatrix(vr::HmdMatrix44_t HMDProjMat, Matrix4& ProjMat);
void SetMatrix(vr::HmdMatrix34_t HMDEyePoseMat, Matrix4& PoseMat);
void CreateFrameBuffer(int BufferWidth, int BufferHeight, FrameBufferDesc &BufferDesc);
void SetupDistortion();
void RenderDistortion();

GLuint	CreateTexturebyImage(char * filename);

void main()
{
	Init();

	GLuint VertexArray = 0;
	glGenVertexArrays(1, &VertexArray);
	GLuint VertexBuffer = 0;  // Vertex Buffer Object (VBO)
	glGenBuffers(1, &VertexBuffer);
	GLuint TexCoordBuffer = 0;
	glGenBuffers(1, &TexCoordBuffer);
	GLuint ElementBuffer = 0;				// Element Buffer Object (EBO)
	glGenBuffers(1, &ElementBuffer);
	
	// Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).
	glBindVertexArray(VertexArray);

	// Position attribute
	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
	//glBufferData(GL_ARRAY_BUFFER, sizeof(TriangleVertexs), TriangleVertexs, GL_STATIC_DRAW);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	// Color attribute
	//glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	//glEnableVertexAttribArray(1);

	// TexCoord
	glBindBuffer(GL_ARRAY_BUFFER, TexCoordBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(texCoords), texCoords, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ElementBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glBindVertexArray(0); // Unbind VAO
	glBindVertexArray(0);

	DistortShaderHandler.Load("DistortVertexShader.vert", "DistortFragmentShader.frag");

	GLuint TextureID = CreateTexturebyImage("TexImage.jpg");

	GLuint		ProjMatLoc, ViewMatLoc, SceneMatrixLocation;
	glm::mat4	ProjMat, ViewMat;

	SceneMatrixLocation = glGetUniformLocation(SceneShaderHandler.ShaderProgramID, "matrix");
	ProjMatLoc = glGetUniformLocation(SceneShaderHandler.ShaderProgramID, "ProjMat");
	ViewMatLoc = glGetUniformLocation(SceneShaderHandler.ShaderProgramID, "ViewMat");

	ProjMat = glm::perspective(45.0f, (GLfloat)640 / (GLfloat)480, (float)nearPlaneZ, (float)farPlaneZ);


	ViewMat = glm::translate(ViewMat, glm::vec3(0.0f, 0.0f, -3.0f));
	ViewMat = glm::rotate(ViewMat, (GLfloat)pi * 90.0f / 180.0f,
						  glm::vec3(1.0, 0.0, 1.0)); // mat, degree, axis. (use radians)

	while (!glfwWindowShouldClose(window))
	{
		vr::TrackedDevicePose_t trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
		vr::VRCompositor()->WaitGetPoses(trackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);

		//RenderDistortion();

		for (int eye = 0; eye < numEyes; ++eye)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, FrameBufferID[eye]);
			glViewport(0, 0, framebufferWidth, framebufferHeight);
			
			glClearColor(0.1f, 0.5f, 0.3f, 0.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glBindTexture(GL_TEXTURE_2D, TextureID);

			SceneShaderHandler.Use();

			glEnable(GL_DEPTH_TEST);		

			glUniformMatrix4fv(SceneMatrixLocation, 1, GL_FALSE, MVPMat[eye].get());
			glUniformMatrix4fv(ProjMatLoc, 1, GL_FALSE, glm::value_ptr(ProjMat));
			glUniformMatrix4fv(ViewMatLoc, 1, GL_FALSE, glm::value_ptr(ViewMat));

			glBindVertexArray(VertexArray);
			glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);

			glUseProgram(0);
		}

		vr::Texture_t LTexture = { reinterpret_cast<void*>(intptr_t(SceneTextureID[Left])), vr::API_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::EVREye(Left), &LTexture);
		vr::Texture_t RTesture = { reinterpret_cast<void*>(intptr_t(SceneTextureID[Right])), vr::API_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::EVREye(Right), &RTesture);

		// Mirror to the window
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GL_NONE);
		glViewport(0, 0, 640, 720);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBlitFramebuffer(0, 0, framebufferWidth, framebufferHeight, 0, 0, 640, 720, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, GL_NONE);

		vr::VRCompositor()->PostPresentHandoff();

		glfwSwapBuffers(window);
		glfwPollEvents();	// This function processes only those events that are already 
							// in the event queue and then returns immediately
	}

	// Properly de-allocate all resources once they've outlived their purpose
	glDeleteVertexArrays(1, &VertexArray);
	glDeleteBuffers(1, &VertexBuffer);

	glfwDestroyWindow(window);
	glfwTerminate();

	system("pause");
}

void Init()
{
	hmd = initOpenVR(framebufferWidth, framebufferHeight);
	assert(hmd);

	const int windowHeight = 720;
	const int windowWidth = (framebufferWidth * windowHeight) / framebufferHeight;

	window = initOpenGL(windowWidth, windowHeight, "minimalOpenGL");


	glGenFramebuffers(numEyes, FrameBufferID);							// create frame buffers for each eyes.

	glGenTextures(numEyes, SceneTextureID);								// prepare texture memory space and give it an index
																		// In this case, glGenTextures creates two textures for colorRenderTarget
																		// and define them by index 1 and 2.
	glGenTextures(numEyes, depthRenderTarget);							// textures of depthRenderTarget are 3 and 4.

	//glGenFramebuffers(numEyes, DistortedFrameBufferId);

	//glGenTextures(numEyes, DistortedTexutreId);
	
	for (int eye = 0; eye < numEyes; ++eye) 
	{
		glBindTexture(GL_TEXTURE_2D, SceneTextureID[eye]);				//Bind which texture if active for processing
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, framebufferWidth, framebufferHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		
		glBindTexture(GL_TEXTURE_2D, depthRenderTarget[eye]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, framebufferWidth, framebufferHeight, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr);

		glBindFramebuffer(GL_FRAMEBUFFER, FrameBufferID[eye]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, SceneTextureID[eye], 0);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthRenderTarget[eye], 0);
		
		glBindTexture(GL_TEXTURE_2D, DistortedTexutreId[eye]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, framebufferWidth, framebufferHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		
		glBindFramebuffer(GL_FRAMEBUFFER, DistortedFrameBufferId[eye]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, DistortedTexutreId[eye], 0);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	vr::HmdMatrix44_t	ProjMat[2];
	vr::HmdMatrix34_t	PoseMat[2];

	ProjectionMat[Left] = GetHMDMatrixProjectionEye(vr::Eye_Left);
	ProjectionMat[Right] = GetHMDMatrixProjectionEye(vr::Eye_Right);

	EyePoseMat[Left] = GetHMDMatrixPoseEye(vr::Eye_Left);
	EyePoseMat[Right] = GetHMDMatrixPoseEye(vr::Eye_Right);


	MVPMat[Left]  = /*ProjectionMat[Left] * */  EyePoseMat[Left];
	MVPMat[Right] = /*ProjectionMat[Right] * */ EyePoseMat[Right];

	WriteMVPMatrixFile("LeftMVPMatrix.txt",  MVPMat[Left]);
	WriteMVPMatrixFile("RightMVPMatrix.txt", MVPMat[Right]);

	//SetupDistortion();

	SceneShaderHandler.Load("SceneVertexShader.vert", "SceneFragmentShader.frag");
}

GLFWwindow* initOpenGL(int width, int height, const std::string& title) 
{
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW\n");
		::exit(1);
	}

	// Without these, shaders actually won't initialize properly
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

#   ifdef _DEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#   endif

	GLFWwindow* window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
	if (!window) 
	{
		fprintf(stderr, "ERROR: could not open window with GLFW\n");
		glfwTerminate();
		::exit(2);
	}
	glfwMakeContextCurrent(window);

	// Start GLEW extension handler, with improved support for new features
	glewExperimental = GL_TRUE;
	glewInit();

	// Clear startup errors
	while (glGetError() != GL_NONE) {}

#   ifdef _DEBUG
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glEnable(GL_DEBUG_OUTPUT);
#   endif

	// Negative numbers allow buffer swaps even if they are after the vertical retrace,
	// but that causes stuttering in VR mode
	glfwSwapInterval(0);

	fprintf(stderr, "GPU: %s (OpenGL version %s)\n", glGetString(GL_RENDERER), glGetString(GL_VERSION));

	// Bind a single global vertex array (done this way since OpenGL 3)
	GLuint vao; 
	glGenVertexArrays(1, &vao); 
	glBindVertexArray(vao);

	// Check for errors
	const GLenum error = glGetError(); 
	assert(error == GL_NONE);

	return window;
}

vr::IVRSystem* initOpenVR(uint32_t& hmdWidth, uint32_t& hmdHeight) 
{
	vr::EVRInitError eError = vr::VRInitError_None;
	vr::IVRSystem* hmd = vr::VR_Init(&eError, vr::VRApplication_Scene);

	if (eError != vr::VRInitError_None) {
		fprintf(stderr, "OpenVR Initialization Error: %s\n", vr::VR_GetVRInitErrorAsEnglishDescription(eError));
		return nullptr;
	}

	const std::string& driver = getHMDString(hmd, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String);		// Graphic card name
	const std::string& model  = getHMDString(hmd, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_ModelNumber_String);				// HMD device name
	const std::string& serial = getHMDString(hmd, vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_SerialNumber_String);			// HMD device serial
	const float freq = hmd->GetFloatTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_DisplayFrequency_Float);	// HMD FPS

	//get the proper resolution of the hmd
	hmd->GetRecommendedRenderTargetSize(&hmdWidth, &hmdHeight);

	fprintf(stderr, "HMD: %s '%s' #%s (%d x %d @ %g Hz)\n", driver.c_str(), model.c_str(), serial.c_str(), hmdWidth, hmdHeight, freq);

	// Initialize the compositor
	vr::IVRCompositor* compositor = vr::VRCompositor();
	if (!compositor) {
		fprintf(stderr, "OpenVR Compositor initialization failed. See log file for details\n");
		vr::VR_Shutdown();
		assert("VR failed" && false);
	}

	return hmd;
}

std::string getHMDString(vr::IVRSystem * pHmd, vr::TrackedDeviceIndex_t unDevice, vr::TrackedDeviceProperty prop, vr::TrackedPropertyError * peError)
{
	uint32_t unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, nullptr, 0, peError);
	if (unRequiredBufferLen == 0) 
	{
		return "";
	}

	char* pchBuffer = new char[unRequiredBufferLen];
	unRequiredBufferLen = pHmd->GetStringTrackedDeviceProperty(unDevice, prop, pchBuffer, unRequiredBufferLen, peError);
	std::string sResult = pchBuffer;
	delete[] pchBuffer;

	return sResult;
}


Matrix4 GetHMDMatrixProjectionEye(vr::Hmd_Eye nEye)
{
	if (!hmd)
		return Matrix4();

	vr::HmdMatrix44_t mat = hmd->GetProjectionMatrix(nEye, nearPlaneZ, farPlaneZ, vr::API_OpenGL);

	if (nEye == vr::Eye_Left)
	{
		WriteProjectionMatrixFile("LeftProjectionMatrix.txt", mat);
	}
	else
	{
		WriteProjectionMatrixFile("RightProjectionMatrix.txt", mat);
	}

	return Matrix4(
		mat.m[0][0], mat.m[1][0], mat.m[2][0], mat.m[3][0],
		mat.m[0][1], mat.m[1][1], mat.m[2][1], mat.m[3][1],
		mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
		mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]
	);
}

Matrix4 GetHMDMatrixPoseEye(vr::Hmd_Eye nEye)
{
	if (!hmd)
		return Matrix4();

	vr::HmdMatrix34_t matEyeRight = hmd->GetEyeToHeadTransform(nEye);

	if (nEye == vr::Eye_Left)
	{
		WriteEyePoseMatrixFile("LeftPoseMatrix.txt", matEyeRight);
	}
	else
	{
		WriteEyePoseMatrixFile("RightPoseMatrix.txt", matEyeRight);
	}

	Matrix4 matrixObj(
		matEyeRight.m[0][0], matEyeRight.m[1][0], matEyeRight.m[2][0], 0.0,
		matEyeRight.m[0][1], matEyeRight.m[1][1], matEyeRight.m[2][1], 0.0,
		matEyeRight.m[0][2], matEyeRight.m[1][2], matEyeRight.m[2][2], 0.0,
		matEyeRight.m[0][3], matEyeRight.m[1][3], matEyeRight.m[2][3], 1.0f
	);

	return matrixObj.invert();
}

void WriteProjectionMatrixFile(char * Filename, vr::HmdMatrix44_t ProjMat)
{
	fstream File;

	File.open(Filename, ios::out);
	for (int i = 0; i < 4; i++)
	{
		File << ProjMat.m[i][0] << " " << ProjMat.m[i][1] << " "
			 << ProjMat.m[i][2] << " " << ProjMat.m[i][3] << "\n";
	}
	File.close();
}

void WriteEyePoseMatrixFile(char * Filename, vr::HmdMatrix34_t PoseMat)
{
	fstream File;

	File.open(Filename, ios::out);
	for (int i = 0; i < 3; i++)
	{
		File << PoseMat.m[i][0] << " " << PoseMat.m[i][1] << " "
			 << PoseMat.m[i][2] << " " << PoseMat.m[i][3] << "\n";
	}
	File.close();
}

void WriteMVPMatrixFile(char * Filename, Matrix4 matMVP)
{
	fstream File;

	File.open(Filename, ios::out);
	for (int i = 0; i < 4; i++)
	{
		File << matMVP.m[4 * i] << " " << matMVP.m[4 * i + 1] << " "
			 << matMVP.m[4 * i + 2] << " " << matMVP.m[4 * i + 3] << "\n";
	}
	File.close();
}

void SetMatrix(vr::HmdMatrix44_t HMDProjMat, Matrix4 & ProjMat)
{
	for (int i = 0; i < 4; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			ProjMat.m[4 * i + j] = HMDProjMat.m[i][j];
		}
	}
}

void SetMatrix(vr::HmdMatrix34_t HMDEyePoseMat, Matrix4 & PoseMat)
{
	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 4; j++)
		{
			PoseMat.m[4 * i+j] = HMDEyePoseMat.m[i][j];
		}	
	}

	PoseMat.m[12] = 0.0f;
	PoseMat.m[13] = 0.0f;
	PoseMat.m[14] = 0.0f;
	PoseMat.m[15] = 1.0f;
}

void CreateFrameBuffer(int BufferWidth, int BufferHeight, FrameBufferDesc & BufferDesc)
{
	//glGenBuffers();
}

void SetupDistortion()
{
	if (!hmd)
		return;

	GLushort m_iLensGridSegmentCountH = 43;
	GLushort m_iLensGridSegmentCountV = 43;

	float w = (float)(1.0 / float(m_iLensGridSegmentCountH - 1));
	float h = (float)(1.0 / float(m_iLensGridSegmentCountV - 1));

	float u, v = 0;

	std::vector<VertexDataLens> vVerts(0);
	VertexDataLens vert;

	//left eye distortion verts
	float Xoffset = -1;
	for (int y = 0; y<m_iLensGridSegmentCountV; y++)
	{
		for (int x = 0; x<m_iLensGridSegmentCountH; x++)
		{
			u = x*w; v = 1 - y*h;
			vert.position = Vector2(Xoffset + u, -1 + 2 * y*h);

			vr::DistortionCoordinates_t dc0 = hmd->ComputeDistortion(vr::Eye_Left, u, v);

			vert.texCoordRed   = Vector2(dc0.rfRed[0], 1 - dc0.rfRed[1]);
			vert.texCoordGreen = Vector2(dc0.rfGreen[0], 1 - dc0.rfGreen[1]);
			vert.texCoordBlue  = Vector2(dc0.rfBlue[0], 1 - dc0.rfBlue[1]);

			vVerts.push_back(vert);
		}
	}

	//right eye distortion verts
	Xoffset = 0;
	for (int y = 0; y<m_iLensGridSegmentCountV; y++)
	{
		for (int x = 0; x<m_iLensGridSegmentCountH; x++)
		{
			u = x*w; v = 1 - y*h;
			vert.position = Vector2(Xoffset + u, -1 + 2 * y*h);

			vr::DistortionCoordinates_t dc0 = hmd->ComputeDistortion(vr::Eye_Right, u, v);

			vert.texCoordRed = Vector2(dc0.rfRed[0], 1 - dc0.rfRed[1]);
			vert.texCoordGreen = Vector2(dc0.rfGreen[0], 1 - dc0.rfGreen[1]);
			vert.texCoordBlue = Vector2(dc0.rfBlue[0], 1 - dc0.rfBlue[1]);

			vVerts.push_back(vert);
		}
	}

	std::vector<GLushort> vIndices;
	GLushort a, b, c, d;

	GLushort offset = 0;
	for (GLushort y = 0; y<m_iLensGridSegmentCountV - 1; y++)
	{
		for (GLushort x = 0; x<m_iLensGridSegmentCountH - 1; x++)
		{
			a = m_iLensGridSegmentCountH*y + x + offset;
			b = m_iLensGridSegmentCountH*y + x + 1 + offset;
			c = (y + 1)*m_iLensGridSegmentCountH + x + 1 + offset;
			d = (y + 1)*m_iLensGridSegmentCountH + x + offset;
			vIndices.push_back(a);
			vIndices.push_back(b);
			vIndices.push_back(c);

			vIndices.push_back(a);
			vIndices.push_back(c);
			vIndices.push_back(d);
		}
	}

	offset = (m_iLensGridSegmentCountH)*(m_iLensGridSegmentCountV);
	for (GLushort y = 0; y<m_iLensGridSegmentCountV - 1; y++)
	{
		for (GLushort x = 0; x<m_iLensGridSegmentCountH - 1; x++)
		{
			a = m_iLensGridSegmentCountH*y + x + offset;
			b = m_iLensGridSegmentCountH*y + x + 1 + offset;
			c = (y + 1)*m_iLensGridSegmentCountH + x + 1 + offset;
			d = (y + 1)*m_iLensGridSegmentCountH + x + offset;
			vIndices.push_back(a);
			vIndices.push_back(b);
			vIndices.push_back(c);

			vIndices.push_back(a);
			vIndices.push_back(c);
			vIndices.push_back(d);
		}
	}
	m_uiIndexSize = vIndices.size();

	glGenVertexArrays(1, &m_unLensVAO);
	glBindVertexArray(m_unLensVAO);

	glGenBuffers(1, &m_glIDVertBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, m_glIDVertBuffer);
	glBufferData(GL_ARRAY_BUFFER, vVerts.size() * sizeof(VertexDataLens), &vVerts[0], GL_STATIC_DRAW);

	glGenBuffers(1, &m_glIDIndexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_glIDIndexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, vIndices.size() * sizeof(GLushort), &vIndices[0], GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataLens), (void *)offsetof(VertexDataLens, position));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataLens), (void *)offsetof(VertexDataLens, texCoordRed));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataLens), (void *)offsetof(VertexDataLens, texCoordGreen));

	glEnableVertexAttribArray(3);
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(VertexDataLens), (void *)offsetof(VertexDataLens, texCoordBlue));

	glBindVertexArray(0);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);
	glDisableVertexAttribArray(3);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void RenderDistortion()
{
	//glDisable(GL_DEPTH_TEST);
	glViewport(0, 0, windowWidth, windowHeight);

	glBindVertexArray(m_unLensVAO);
	DistortShaderHandler.Use();

	//render left lens (first half of index array )
	//glBindTexture(GL_TEXTURE_2D, DistortedTexutreId[Left]);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	//glDrawElements(GL_TRIANGLES, m_uiIndexSize / 2, GL_UNSIGNED_SHORT, 0);

	//render right lens (second half of index array )
	//glBindTexture(GL_TEXTURE_2D, DistortedTexutreId[Right]);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	//glDrawElements(GL_TRIANGLES, m_uiIndexSize / 2, GL_UNSIGNED_SHORT, (const void *)(m_uiIndexSize));

	glBindVertexArray(0);
	glUseProgram(0);
}

GLuint CreateTexturebyImage(char * filename)
{
	GLuint	texture;
	Mat		TexImage = imread(filename, 1);

	for (int i = 0; i < TexImage.cols; i++)
	{
		for (int j = 0; j < TexImage.rows; j++)
		{
			int		PixelIdx = TexImage.cols * j + i;
			uchar	temp;

			temp = TexImage.data[3 * PixelIdx];
			TexImage.data[3 * PixelIdx] = TexImage.data[3 * PixelIdx + 2];
			TexImage.data[3 * PixelIdx + 2] = temp;
		}
	}

	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	// Set the texture wrapping parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	// Set texture wrapping to GL_REPEAT (usually basic wrapping method)
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	// Set texture filtering parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, TexImage.cols, TexImage.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, TexImage.data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	return texture;
}

