#include <GLEW/glew.h>
#include <GLEW/wglew.h>
#include <GLFW/glfw3.h> 
#include <GLM/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "kuShaderHandler.h"
#include "kuModelObject.h"

#include <OpenVR.h>
#include <opencv2/opencv.hpp>

#include <cstring>
#include <cassert>
#include <iostream>
#include <fstream>
#include <vector>

#include "vectors.h"
#include "Matrices.h"

//#define _VR

#define pi			3.1415926

#define	numEyes		2

#define Left		0
#define Right		1

#define nearPlaneZ	0.1
#define farPlaneZ	1000

using namespace std;
using namespace cv;

GLFWwindow		*	window = nullptr;
vr::IVRSystem	*	hmd    = nullptr;

#pragma region // Frame Buffer Containers
/////////////////////////////////////////////////////////////////////////////////////////
struct FrameBufferDesc
{
	GLuint	SceneTextureId;
	GLuint	SceneFrameBufferId;
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

Matrix4		HMDProjectionMat[2];
Matrix4		EyePoseMat[2];  
Matrix4		MVPMat[2];

glm::vec3 CameraPos   = glm::vec3(0.0f, 0.0f, 200.0f);
glm::vec3 CameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 CameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);


bool			keyPressArray[1024];

void				Init();
GLFWwindow		*	initOpenGL(int width, int height, const std::string& title, GLFWkeyfun cbfun);
vr::IVRSystem	*	initOpenVR(uint32_t& hmdWidth, uint32_t& hmdHeight);
std::string			getHMDString(vr::IVRSystem* pHmd, vr::TrackedDeviceIndex_t unDevice, 
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

GLuint			CreateTexturebyImage(Mat Img);

void			key_callback(GLFWwindow * window, int key, int scancode, int action, int mode);
void			mouse_callback(GLFWwindow * window, double xPos, double yPos);
void			do_movement();
Matrix4			ConvertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t &matPose);

void			DrawImage(Mat Img, kuShaderHandler ImgShader);
void			DrawAxes(kuShaderHandler axesShader, float length, int eyeSide);
void			DrawPath(kuShaderHandler axesShader, int eyeSide);
void			DrawCube(kuShaderHandler cubeShader, int eyeSide, 
						 float posX, float posY, float posZ,
						 glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, 
						 float cubeSize = 0.5);

bool			firstMouse = true;

GLfloat			yaw = -90.0f;			
				// Yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right
GLfloat			pitch = 0.0f;
GLfloat			LastXPos, LastYPos;

GLfloat			deltaTime = 0.0f;
GLfloat			lastFrameT = 0.0f;

glm::mat4		ProjMat, ModelMat, ViewMat;
glm::mat4		TransCT2Model;

#pragma region // Shader uniform location
GLuint			CamPosLoc;
GLuint			ProjMatLoc, ViewMatLoc, ModelMatLoc, SceneMatrixLocation;
GLuint			ObjColorLoc;

GLuint			ImgModelMatLoc, ImgViewMatLoc, ImgProjMatLoc;
GLuint			ImgSceneMatrixLocation, TransCT2ModelLoc;

GLuint			AxesModelMatLoc, AxesViewMatLoc, AxesProjMatLoc;
#pragma endregion

GLfloat	ImgVertices[]
= {
	128.25f, -128.25f,  10.5f,	 1.0f,  0.0f,
	128.25f,  128.25f,  10.5f,	 1.0f,  1.0f,
   -128.25f,  128.25f,  10.5f,	 0.0f,  1.0f,
   -128.25f, -128.25f,  10.5f,	 0.0f,  0.0f
};

GLuint ImgIndices[]
= {
	0, 1, 3,
	1, 2, 3
};


int main()
{
	Init();
	
	//kuModelObject	Model("LAI-WEN-HSIEN-big.surf.stl");
	//kuModelObject	Model("1.stl");

	kuModelObject		FaceModel("kuFace_7d5wf_SG_Center.stl");
	kuModelObject		BoneModel("kuBone_7d5wf_SG_Center.stl");
	kuShaderHandler		ModelShaderHandler;
	kuShaderHandler		ImgShader;
	kuShaderHandler		AxesShaderHandler;
	
	ModelShaderHandler.Load("ModelVertexShader.vert", "ModelFragmentShader.frag");
	ImgShader.Load("ImgVertexShader.vert", "ImgFragmentShader.frag");
	AxesShaderHandler.Load("AxesVertexShader.vert", "AxesFragmentShader.frag");

	TransCT2Model = glm::translate(TransCT2Model, glm::vec3(-128.249, -281.249, -287));

	// Set model shader uniform location
	SceneMatrixLocation = glGetUniformLocation(ModelShaderHandler.ShaderProgramID, "matrix");
	ProjMatLoc			= glGetUniformLocation(ModelShaderHandler.ShaderProgramID, "ProjMat");
	ViewMatLoc			= glGetUniformLocation(ModelShaderHandler.ShaderProgramID, "ViewMat");
	ModelMatLoc			= glGetUniformLocation(ModelShaderHandler.ShaderProgramID, "ModelMat");
	CamPosLoc			= glGetUniformLocation(ModelShaderHandler.ShaderProgramID, "CamPos");
	ObjColorLoc			= glGetUniformLocation(ModelShaderHandler.ShaderProgramID, "ObjColor");

	// Set image shader uniform location
	ImgSceneMatrixLocation = glGetUniformLocation(ImgShader.ShaderProgramID, "matrix");
	ImgProjMatLoc		   = glGetUniformLocation(ImgShader.ShaderProgramID, "ProjMat");
	ImgViewMatLoc		   = glGetUniformLocation(ImgShader.ShaderProgramID, "ViewMat");
	ImgModelMatLoc		   = glGetUniformLocation(ImgShader.ShaderProgramID, "ModelMat");
	TransCT2ModelLoc	   = glGetUniformLocation(ImgShader.ShaderProgramID, "TransCT2Model");

	// Set coordinate axes shader uniform location
	AxesViewMatLoc  = glGetUniformLocation(AxesShaderHandler.ShaderProgramID, "ViewMat");
	AxesProjMatLoc  = glGetUniformLocation(AxesShaderHandler.ShaderProgramID, "ProjMat");
	AxesModelMatLoc = glGetUniformLocation(AxesShaderHandler.ShaderProgramID, "ModelMat");

	// 不設定ProjMat的值是因為在Init()裡面透過GetHMDMatrixProjectionEye取出Vive的projection matrix
	// 而且這樣打進shader "matrix"那個location裡面那個轉換矩陣就已經是MVP了....model, view, projection乘一起

	GLfloat FaceColorVec[4] = { 0.745f, 0.447f, 0.235f, 0.5f };
	GLfloat BoneColorVec[4] = {   1.0f,   1.0f,   1.0f, 1.0f };
	GLfloat CubeColorVec[4] = {   1.0f,   0.0f,   0.0f, 1.0f };

	Mat AxiImg = imread("HSIEH-CHUNG-HUNG-OrthoSlice.to-byte.0000.bmp", 1);

	ModelMat = glm::rotate(ModelMat, (GLfloat)pi * -90.0f / 180.0f,
						   glm::vec3(1.0f, 0.0f, 0.0f)); // mat, degree, axis. (use radians)
	//ModelMat = glm::translate(ModelMat, glm::vec3(0.0f, 0.0f, 20.0f));
	ModelMat = glm::scale(ModelMat, glm::vec3(0.005f, 0.005f, 0.005f));

	while (!glfwWindowShouldClose(window))
	{
		GLfloat currFrameT = glfwGetTime();
		deltaTime = currFrameT - lastFrameT;
		lastFrameT = currFrameT;

		glfwPollEvents();	// This function processes only those events that are already 
							// in the event queue and then returns immediately
		do_movement();

		vr::TrackedDevicePose_t trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
		vr::VRCompositor()->WaitGetPoses(trackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);

		Matrix4 HMDPoseMat = ConvertSteamVRMatrixToMatrix4(trackedDevicePose[0].mDeviceToAbsoluteTracking);
		HMDPoseMat.invert();

		MVPMat[Left]  = HMDProjectionMat[Left] * EyePoseMat[Left] * HMDPoseMat;
		MVPMat[Right] = HMDProjectionMat[Right] * EyePoseMat[Right] * HMDPoseMat;

		for (int eye = 0; eye < numEyes; ++eye)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, FrameBufferID[eye]);
			glViewport(0, 0, framebufferWidth, framebufferHeight);
			
			glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glEnable(GL_DEPTH_TEST);		
		
			ViewMat = glm::lookAt(CameraPos, CameraPos + CameraFront, CameraUp);

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			#pragma region // Render virtual model to the frame buffer //
			ModelShaderHandler.Use();
			glUniformMatrix4fv(SceneMatrixLocation, 1, GL_FALSE, MVPMat[eye].get());
			glUniformMatrix4fv(ProjMatLoc, 1, GL_FALSE, glm::value_ptr(ProjMat));
			glUniformMatrix4fv(ViewMatLoc, 1, GL_FALSE, glm::value_ptr(ViewMat));
			glUniformMatrix4fv(ModelMatLoc, 1, GL_FALSE, glm::value_ptr(ModelMat));
			glUniform3fv(CamPosLoc, 1, glm::value_ptr(CameraPos));

			// Inner object first.
			glUniform4fv(ObjColorLoc, 1, BoneColorVec);
			BoneModel.Draw(ModelShaderHandler, glm::vec3(0.3f, 0.3f, 0.3f),
						   glm::vec3(0.5f, 0.5f, 0.5f),
						   glm::vec3(0.3f, 0.3f, 0.3f));
			//BoneModel.Draw(ModelShaderHandler);

			// Draw outside object latter
			glUniform4fv(ObjColorLoc, 1, FaceColorVec);
			FaceModel.Draw(ModelShaderHandler, glm::vec3(0.3f, 0.3f, 0.3f),
						   glm::vec3(0.5f, 0.5f, 0.5f),
						   glm::vec3(0.3f, 0.3f, 0.3f));
			//FaceModel.Draw(ModelShaderHandler);

			glUniform4fv(ObjColorLoc, 1, CubeColorVec);
			DrawCube(ModelShaderHandler, eye,
				5.25f, -10.5f, 10.5f,
				glm::vec3(0.3f, 0.3f, 0.3f),
				glm::vec3(1.0f, 1.0f, 1.0f),
				glm::vec3(0.3f, 0.3f, 0.3f),
				3.0f);
			#pragma endregion

			DrawAxes(AxesShaderHandler, 0.3f, eye);
			DrawPath(AxesShaderHandler, eye);

			glDisable(GL_DEPTH_TEST);

			#pragma region // Render medical image slice to the frame buffer
			ImgShader.Use();
			glUniformMatrix4fv(ImgSceneMatrixLocation, 1, GL_FALSE, MVPMat[eye].get());
			glUniformMatrix4fv(ImgProjMatLoc, 1, GL_FALSE, glm::value_ptr(ProjMat));
			glUniformMatrix4fv(ImgViewMatLoc, 1, GL_FALSE, glm::value_ptr(ViewMat));
			glUniformMatrix4fv(ImgModelMatLoc, 1, GL_FALSE, glm::value_ptr(ModelMat));
			glUniformMatrix4fv(TransCT2ModelLoc, 1, GL_FALSE, glm::value_ptr(TransCT2Model));
			DrawImage(AxiImg, ImgShader);
			#pragma endregion

			glUseProgram(0);
		}

		// Pop frame buffer to VR compositor
		vr::Texture_t LTexture = { reinterpret_cast<void*>(intptr_t(SceneTextureID[Left])), vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::EVREye(Left), &LTexture);
		vr::Texture_t RTesture = { reinterpret_cast<void*>(intptr_t(SceneTextureID[Right])), vr::TextureType_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::EVREye(Right), &RTesture);

		vr::VRCompositor()->PostPresentHandoff();

		// Mirror to the window
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GL_NONE);
		glViewport(0, 0, 640, 720);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBlitFramebuffer(0, 0, framebufferWidth, framebufferHeight, 0, 0, 640, 720, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, GL_NONE);

		glfwSwapBuffers(window);
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

void Init()
{
	hmd = initOpenVR(framebufferWidth, framebufferHeight);
	assert(hmd);

	const int windowHeight = 720;
	const int windowWidth = (framebufferWidth * windowHeight) / framebufferHeight;

	window = initOpenGL(windowWidth, windowHeight, "kuOpenGLVRTest", key_callback);

	glGenFramebuffers(numEyes, FrameBufferID);							// create frame buffers for each eyes.

	glGenTextures(numEyes, SceneTextureID);								// prepare texture memory space and give it an index
																		// In this case, glGenTextures creates two textures for colorRenderTarget
																		// and define them by index 1 and 2.
	glGenTextures(numEyes, depthRenderTarget);							// textures of depthRenderTarget are 3 and 4.

	glGenFramebuffers(numEyes, DistortedFrameBufferId);

	glGenTextures(numEyes, DistortedTexutreId);
	
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

	HMDProjectionMat[Left]  = GetHMDMatrixProjectionEye(vr::Eye_Left);
	HMDProjectionMat[Right] = GetHMDMatrixProjectionEye(vr::Eye_Right);

	EyePoseMat[Left]  = GetHMDMatrixPoseEye(vr::Eye_Left);
	EyePoseMat[Right] = GetHMDMatrixPoseEye(vr::Eye_Right);

	//WriteMVPMatrixFile("LeftMVPMatrix.txt",  MVPMat[Left]);
	//WriteMVPMatrixFile("RightMVPMatrix.txt", MVPMat[Right]);
}

GLFWwindow * initOpenGL(int width, int height, const std::string& title, GLFWkeyfun cbfun)
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

	glfwSetKeyCallback(window, cbfun);

	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwSetCursorPosCallback(window, mouse_callback);				// 顧名思義...大概只有位置資訊而沒有button事件資訊吧

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

vr::IVRSystem * initOpenVR(uint32_t& hmdWidth, uint32_t& hmdHeight) 
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

	vr::HmdMatrix44_t mat = hmd->GetProjectionMatrix(nEye, nearPlaneZ, farPlaneZ);

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

GLuint CreateTexturebyImage(Mat Img)
{
	GLuint	texture;

	for (int i = 0; i < Img.cols; i++)
	{
		for (int j = 0; j < Img.rows; j++)
		{
			int		PixelIdx = Img.cols * j + i;
			uchar	temp;

			temp = Img.data[3 * PixelIdx];
			Img.data[3 * PixelIdx] = Img.data[3 * PixelIdx + 2];
			Img.data[3 * PixelIdx + 2] = temp;
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

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, Img.cols, Img.rows, 0, GL_RGB, GL_UNSIGNED_BYTE, Img.data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	return texture;
}

void key_callback(GLFWwindow * window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
		{
			keyPressArray[key] = true;
		}
		else if (action == GLFW_RELEASE)
		{
			keyPressArray[key] = false;
		}
	}

	if (key == GLFW_KEY_C)
	{
		cout << "CameraPos: (" << CameraPos.x << ", " << CameraPos.y << ", " << CameraPos.z << ")" << endl;
	}
}

void mouse_callback(GLFWwindow * window, double xPos, double yPos)
{
	if (firstMouse)
	{
		LastXPos = xPos;
		LastYPos = yPos;

		firstMouse = false;
	}

	GLfloat xOffset = xPos - LastXPos;
	GLfloat yOffset = yPos - LastYPos;
	LastXPos = xPos;
	LastYPos = yPos;

	GLfloat sensitivity = 0.05;
	xOffset *= sensitivity;
	yOffset *= sensitivity;

	yaw += xOffset;
	pitch += yOffset;

	// Make sure that when pitch is out of bounds, screen doesn't get flipped
	if (pitch > 89.0f)
		pitch = 89.0f;
	if (pitch < -89.0f)
		pitch = -89.0f;

	glm::vec3 front;
	front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
	front.y = sin(glm::radians(pitch));
	front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
	CameraFront = glm::normalize(front);
}

void do_movement()
{
	// Camera controls
	GLfloat CameraSpeedX = 2.0f;	// 其實就是一個frame要動的量
	GLfloat CameraSpeedY = 2.0f;
	GLfloat CameraSpeedZ = 2.0f;

	GLfloat CameraSpeed = 100.0f * deltaTime;

	if (keyPressArray[GLFW_KEY_W])
	{
		CameraPos += CameraSpeed * CameraFront;
	}
	if (keyPressArray[GLFW_KEY_S])
	{
		CameraPos -= CameraSpeed * CameraFront;
	}
	if (keyPressArray[GLFW_KEY_A])
	{
		//CameraPos -= glm::normalize(glm::cross(CameraFront, CameraUp)) * cameraSpeed;
		CameraPos = glm::vec3(CameraPos.x -= CameraSpeed, CameraPos.y, CameraPos.z);
	}
	if (keyPressArray[GLFW_KEY_D])
	{
		//CameraPos += glm::normalize(glm::cross(CameraFront, CameraUp)) * cameraSpeed;
		CameraPos = glm::vec3(CameraPos.x += CameraSpeed, CameraPos.y, CameraPos.z);
	}
	if (keyPressArray[GLFW_KEY_Z])
	{
		CameraPos = glm::vec3(CameraPos.x, CameraPos.y += CameraSpeed, CameraPos.z);
	}
	if (keyPressArray[GLFW_KEY_X])
	{
		CameraPos = glm::vec3(CameraPos.x, CameraPos.y -= CameraSpeed, CameraPos.z);
	}
}

void DrawImage(Mat Img, kuShaderHandler ImgShader)
{
	GLuint ImgVertexArray = 0;
	glGenVertexArrays(1, &ImgVertexArray);
	GLuint ImgVertexBuffer = 0;						// Vertex Buffer Object (VBO)
	glGenBuffers(1, &ImgVertexBuffer);				// give an ID to vertex buffer
	GLuint ImgElementBuffer = 0;						// Element Buffer Object (EBO)
	glGenBuffers(1, &ImgElementBuffer);

	glBindVertexArray(ImgVertexArray);

	glBindBuffer(GL_ARRAY_BUFFER, ImgVertexBuffer);  // Bind buffer as array buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(ImgVertices), ImgVertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ImgElementBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(ImgIndices), ImgIndices, GL_STATIC_DRAW);

	// Assign vertex position data
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	// Assign texture coordinates
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	GLuint ImgTextureID = CreateTexturebyImage(Img);

	ImgShader.Use();

	glBindTexture(GL_TEXTURE_2D, ImgTextureID);

	glBindVertexArray(ImgVertexArray);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
	//glBindTexture(GL_TEXTURE_2D, 0);

	glDeleteTextures(1, &ImgTextureID);

	glDeleteVertexArrays(1, &ImgVertexArray);
	glDeleteBuffers(1, &ImgVertexBuffer);
	glDeleteBuffers(1, &ImgElementBuffer);
}

void DrawAxes(kuShaderHandler axesShader, float length, int eyeSide)
{
	GLuint axesVertexArray;

	const GLfloat AxesPts[] = {
		// position				// color
		0.0, 0.0,  0.0,		1.0, 0.0, 0.0,
		length, 0.0,  0.0,		1.0, 0.0, 0.0,
		0.0,   0.0,  0.0,	0.0, 1.0, 0.0,
		0.0,   length,  0.0,	0.0, 1.0, 0.0,
		0.0,   0.0,  0.0,	0.0, 0.0, 1.0,
		0.0,   0.0,  length,	0.0, 0.0, 1.0
	};

#pragma region // GL rendering setting //
	glGenVertexArrays(1, &axesVertexArray);
	GLuint AxesVertexBuffer = 0;				// Vertex Buffer Object (VBO)
	glGenBuffers(1, &AxesVertexBuffer);			// give an ID to vertex buffer

	glBindVertexArray(axesVertexArray);
	glBindBuffer(GL_ARRAY_BUFFER, AxesVertexBuffer); // Bind buffer as array buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(AxesPts), AxesPts, GL_STATIC_DRAW);

	// position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	// color
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	//glBindBuffer(GL_ARRAY_BUFFER, 0);
	//glBindVertexArray(0);
#pragma endregion

	axesShader.Use();

	glUniformMatrix4fv(AxesModelMatLoc, 1, GL_FALSE, glm::value_ptr(ModelMat));
	glUniformMatrix4fv(AxesViewMatLoc, 1, GL_FALSE, MVPMat[eyeSide].get());
	glUniformMatrix4fv(AxesProjMatLoc, 1, GL_FALSE, glm::value_ptr(ProjMat));

	glBindVertexArray(axesVertexArray);
	//glDrawArrays(GL_TRIANGLES, 0, 3); // Starting from vertex 0; 3 vertices total -> 1 triangle
	glEnable(GL_LINE_SMOOTH);
	glLineWidth(10);
	glDrawArrays(GL_LINES, 0, 6); // Starting from vertex 0; 3 vertices total -> 1 triangle
	glBindVertexArray(0);
}

void DrawPath(kuShaderHandler axesShader, int eyeSide)
{
	const GLfloat SurgicalPathVertices[]
		= {
		// Position				// Color
		0.0f, 0.0f, 0.0f,		1.0f, 0.0f, 1.0f,
		1.5f, 3.0f, 3.0f,		1.0f, 0.0f, 1.0f
	};

	GLuint axesVertexArray;

#pragma region // GL rendering setting //
	glGenVertexArrays(1, &axesVertexArray);
	GLuint AxesVertexBuffer = 0;				// Vertex Buffer Object (VBO)
	glGenBuffers(1, &AxesVertexBuffer);			// give an ID to vertex buffer

	glBindVertexArray(axesVertexArray);
	glBindBuffer(GL_ARRAY_BUFFER, AxesVertexBuffer); // Bind buffer as array buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(SurgicalPathVertices), SurgicalPathVertices, GL_STATIC_DRAW);

	// position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	// color
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	//glBindBuffer(GL_ARRAY_BUFFER, 0);
	//glBindVertexArray(0);
#pragma endregion

	axesShader.Use();

	glUniformMatrix4fv(AxesModelMatLoc, 1, GL_FALSE, glm::value_ptr(ModelMat));
	glUniformMatrix4fv(AxesViewMatLoc, 1, GL_FALSE, MVPMat[eyeSide].get());
	glUniformMatrix4fv(AxesProjMatLoc, 1, GL_FALSE, glm::value_ptr(ProjMat));

	glBindVertexArray(axesVertexArray);
	//glDrawArrays(GL_TRIANGLES, 0, 3); // Starting from vertex 0; 3 vertices total -> 1 triangle
	glEnable(GL_LINE_SMOOTH);
	glLineWidth(10);
	glDrawArrays(GL_LINES, 0, 2); // Starting from vertex 0; 3 vertices total -> 1 triangle
	glBindVertexArray(0);
}

void DrawCube(kuShaderHandler cubeShader, int eyeSide, 
			  float posX, float posY, float posZ,
			  glm::vec3 ambient, glm::vec3 diffuse, glm::vec3 specular, float cubeSize)
{
	const GLfloat	vertices[]
		= {
		// Frontal Face
		posX - cubeSize, posY + cubeSize, posZ + cubeSize,		// 0:  Top Left  
		posX + cubeSize, posY + cubeSize, posZ + cubeSize,			// 1:  Top Right
		posX + cubeSize, posY - cubeSize, posZ + cubeSize,			// 2:  Bottom Right
		posX - cubeSize, posY - cubeSize, posZ + cubeSize,		// 3:  Bottom Left
							
		// Right Face	po	Y + s		posZ +
		posX - cubeSize, posY + cubeSize, posZ - cubeSize,			// 4:  Top Left
		posX - cubeSize, posY + cubeSize, posZ + cubeSize,			// 5:  Top Right
		posX - cubeSize, posY - cubeSize, posZ + cubeSize,			// 6:  Bottom Right
		posX - cubeSize, posY - cubeSize, posZ - cubeSize,			// 7:  Bottom Left
								
		// Back face	po	Y + s		posZ +
		posX + cubeSize, posY + cubeSize, posZ - cubeSize,			// 8:  Top Left 
		posX - cubeSize, posY + cubeSize, posZ - cubeSize,			// 9:  Top Right
		posX - cubeSize, posY - cubeSize, posZ - cubeSize,			// 10: Bottom Right
		posX + cubeSize, posY - cubeSize, posZ - cubeSize,			// 11: Bottom Left
		 						
		// Left Face	po	Y + s		posZ +
		posX + cubeSize, posY +  cubeSize, posZ + cubeSize,			// 12: Top Left 
		posX + cubeSize, posY +  cubeSize, posZ +-cubeSize, 			// 13: Top Right
		posX + cubeSize, posY - cubeSize, posZ +-cubeSize, 			// 14: Bottom Right
		posX + cubeSize, posY - cubeSize, posZ + cubeSize,  			// 15: Bottom Left
		 				
		// Up Face		po	Y + s		posZ +
		posX - cubeSize, posY + cubeSize, posZ - cubeSize,  		    // 16: Top Left 
		posX + cubeSize, posY + cubeSize, posZ - cubeSize,  		    // 17: Top Right
		posX + cubeSize, posY + cubeSize, posZ + cubeSize,  		    // 18: Bottom Right
		posX - cubeSize, posY + cubeSize, posZ + cubeSize,  		    // 19: Bottom Left
		 						
		// Down Face	po	Y + s		posZ +
		posX - cubeSize, posY - cubeSize, posZ + cubeSize,			// 20: Top Left 
		posX + cubeSize, posY - cubeSize, posZ + cubeSize,  			// 21: Top Right
		posX + cubeSize, posY - cubeSize, posZ - cubeSize,    		// 22: Bottom Right
		posX - cubeSize, posY - cubeSize, posZ - cubeSize  			// 23: Bottom Left
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

#pragma region // Set VAO, VBO and EBO //
	GLuint VertexArray = 0;
	glGenVertexArrays(1, &VertexArray);
	GLuint VertexBuffer = 0;				// Vertex Buffer Object (VBO)
	glGenBuffers(1, &VertexBuffer);			// give an ID to vertex buffer
	GLuint TexCoordBuffer = 0;
	glGenBuffers(1, &TexCoordBuffer);
	GLuint ElementBuffer = 0;				// Element Buffer Object (EBO)
	glGenBuffers(1, &ElementBuffer);

	glBindVertexArray(VertexArray);

	glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer); // Bind buffer as array buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	// Position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, TexCoordBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(texCoords), texCoords, GL_STATIC_DRAW);
	// TexCoord
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ElementBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
#pragma endregion

	cubeShader.Use();

	glUniform3f(glGetUniformLocation(cubeShader.ShaderProgramID, "material.ambient"),
		ambient.r, ambient.g, ambient.b);
	glUniform3f(glGetUniformLocation(cubeShader.ShaderProgramID, "material.diffuse"),
		diffuse.r, diffuse.g, diffuse.b);
	glUniform3f(glGetUniformLocation(cubeShader.ShaderProgramID, "material.specular"),
		specular.r, specular.g, specular.b);

	//glUniformMatrix4fv(AxesModelMatLoc, 1, GL_FALSE, glm::value_ptr(ModelMat));
	//glUniformMatrix4fv(AxesViewMatLoc, 1, GL_FALSE, MVPMat[eyeSide].get());
	//glUniformMatrix4fv(AxesProjMatLoc, 1, GL_FALSE, glm::value_ptr(ProjMat));

	glBindVertexArray(VertexArray);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

Matrix4 ConvertSteamVRMatrixToMatrix4(const vr::HmdMatrix34_t &matPose)
{
	Matrix4 matrixObj(
		matPose.m[0][0], matPose.m[1][0], matPose.m[2][0], 0.0,
		matPose.m[0][1], matPose.m[1][1], matPose.m[2][1], 0.0,
		matPose.m[0][2], matPose.m[1][2], matPose.m[2][2], 0.0,
		matPose.m[0][3], matPose.m[1][3], matPose.m[2][3], 1.0f
	);
	return matrixObj;
}