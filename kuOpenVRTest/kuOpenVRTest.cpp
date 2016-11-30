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
#define farPlaneZ	10000

#define		ImgWidth	640
#define		ImgHeight	480

using namespace std;
using namespace cv;

GLFWwindow		*	window = nullptr;
vr::IVRSystem	*	hmd    = nullptr;

kuShaderHandler		ModelShaderHandler;
kuShaderHandler		BGImgShaderHandler;
kuShaderHandler		TexCubeShaderHandler;

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

uint32_t			framebufferWidth  = 1280;			// this will be updated after initialization of OpenVR
uint32_t			framebufferHeight = 720;			// this will be updated after initialization of OpenVR
int					windowWidth		  = 1280;
int					windowHeight	  = 720;

GLuint				FrameBufferID[numEyes];
GLuint				SceneTextureID[numEyes];
GLuint				DistortedFrameBufferId[numEyes];
GLuint				DistortedTexutreId[numEyes];
GLuint				depthRenderTarget[numEyes];

Matrix4				ProjectionMat[2];
Matrix4				EyePoseMat[2];  
Matrix4				MVPMat[2];

Mat					GrayImg;

// for AR visualization
VideoCapture	*	CamCapture = NULL;
Mat					CamFrame;

Mat					IntrinsicMat;
Mat					DistParam;
Mat					RotationVec;
Mat					RotationMat;
Mat					TranslationVec;

vector<Point2f>		CB2DPts;
vector<Point3f>		CB3DPts;

void				DispParam();
GLfloat				IntrinsicProjMat[16];
GLfloat				ExtrinsicProjMat[16];



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

GLuint					CreateTexturebyImage(Mat Img);
void					DrawBGImage(Mat BGImg, kuShaderHandler BGShader);
void					ImgChangeBR(Mat &Img);

bool					LoadCameraParameters(char * Filename);

void					SetCB3DPts();

void					IntrinsicCVtoGL(Mat IntParam, GLfloat GLProjection[16]);
void					ExtrinsicCVtoGL(Mat RotMat, Mat TransVec, GLfloat GLModelView[16]);

const GLfloat	CubeVertices[]
= {
	// Frontal Face
	-50.0f,  50.0f,  100.0f,			// 0:  Top Left  
	50.0f,  50.0f,  100.0f,			// 1:  Top Right
	50.0f, -50.0f,  100.0f,			// 2:  Bottom Right
	-50.0f, -50.0f,  100.0f,			// 3:  Bottom Left

										// Right Face
										-50.0f,  50.0f,   0.0f,			// 4:  Top Left
										-50.0f,  50.0f,  100.0f,			// 5:  Top Right
										-50.0f, -50.0f,  100.0f,			// 6:  Bottom Right
										-50.0f, -50.0f,   0.0f,			// 7:  Bottom Left

																		// Back face
																		50.0f,  50.0f,   0.0f,			// 8:  Top Left 
																		-50.0f,  50.0f,   0.0f,			// 9:  Top Right
																		-50.0f, -50.0f,   0.0f,			// 10: Bottom Right
																		50.0f, -50.0f,   0.0f,			// 11: Bottom Left

																										// Left Face
																										50.0f,  50.0f,  100.0f,			// 12: Top Left 
																										50.0f,  50.0f,   0.0f, 		// 13: Top Right
																										50.0f, -50.0f,   0.0f, 		// 14: Bottom Right
																										50.0f, -50.0f,  100.0f,  		// 15: Bottom Left

																																		// Up Face
																																		-50.0f,  50.0f,   0.0f,  		// 16: Top Left 
																																		50.0f,  50.0f,   0.0f,  		// 17: Top Right
																																		50.0f,  50.0f,  100.0f,  		// 18: Bottom Right
																																		-50.0f,  50.0f,  100.0f,  		// 19: Bottom Left

																																										// Down Face
																																										-50.0f, -50.0f,  100.0f,			// 20: Top Left 
																																										50.0f, -50.0f,  100.0f,  		// 21: Top Right
																																										50.0f, -50.0f,   0.0f,    		// 22: Bottom Right
																																										-50.0f, -50.0f,   0.0f  		// 23: Bottom Left
};

const GLfloat   CubeTexCoords[]
= {
	0.0f, 0.0f,    1.0f, 0.0f,    1.0f, 1.0f,    0.0f, 1.0f,
	0.0f, 0.0f,    1.0f, 0.0f,    1.0f, 1.0f,    0.0f, 1.0f,
	0.0f, 0.0f,    1.0f, 0.0f,    1.0f, 1.0f,    0.0f, 1.0f,
	0.0f, 0.0f,    1.0f, 0.0f,    1.0f, 1.0f,    0.0f, 1.0f,
	0.0f, 0.0f,    1.0f, 0.0f,    1.0f, 1.0f,    0.0f, 1.0f,
	0.0f, 0.0f,    1.0f, 0.0f,    1.0f, 1.0f,    0.0f, 1.0f
};

const GLuint    CubeIndices[]
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

int main()
{
	Init();
	
	kuModelObject	Model("1.stl");

	ModelShaderHandler.Load("ModelVertexShader.vert", "ModelFragmentShader.frag");
	BGImgShaderHandler.Load("BGImgVertexShader.vert", "BGImgFragmentShader.frag");
	TexCubeShaderHandler.Load("TexCubeVertexShader.vert", "TexCubeFragmentShader.frag");

	GLuint CubeVertexArray = 0;
	glGenVertexArrays(1, &CubeVertexArray);
	GLuint CubeVertexBuffer = 0;							// Vertex Buffer Object (VBO)
	glGenBuffers(1, &CubeVertexBuffer);						// give an ID to vertex buffer
	GLuint CubeTexCoordBuffer = 0;
	glGenBuffers(1, &CubeTexCoordBuffer);
	GLuint CubeElementBuffer = 0;							// Element Buffer Object (EBO)
	glGenBuffers(1, &CubeElementBuffer);

	glBindVertexArray(CubeVertexArray);

	glBindBuffer(GL_ARRAY_BUFFER, CubeVertexBuffer);		// Bind buffer as array buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(CubeVertices), CubeVertices, GL_STATIC_DRAW);
	// Position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, CubeTexCoordBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(CubeTexCoords), CubeTexCoords, GL_STATIC_DRAW);
	// TexCoord
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, CubeElementBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(CubeIndices), CubeIndices, GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	Mat		CubeTextureImg = imread("ihatepeople.jpg");
	ImgChangeBR(CubeTextureImg);
	GLuint	CubeTextureID = CreateTexturebyImage(CubeTextureImg);

	GLuint		ProjMatLoc, ViewMatLoc, ModelMatLoc, SceneMatrixLocation, CamPosLoc;
	glm::mat4	ProjMat, ModelMat, ViewMat;

	/*SceneMatrixLocation = glGetUniformLocation(ModelShaderHandler.ShaderProgramID, "matrix");
	ProjMatLoc			= glGetUniformLocation(ModelShaderHandler.ShaderProgramID, "ProjMat");
	ViewMatLoc			= glGetUniformLocation(ModelShaderHandler.ShaderProgramID, "ViewMat");
	ModelMatLoc			= glGetUniformLocation(ModelShaderHandler.ShaderProgramID, "ModelMat");

	CamPosLoc = glGetUniformLocation(ModelShaderHandler.ShaderProgramID, "CamPos");*/

	ProjMat  = glm::perspective(45.0f, (GLfloat)648 / (GLfloat)720, (float)nearPlaneZ, (float)farPlaneZ);
	//拿掉是因為在Init()裡面透過GetHMDMatrixProjectionEye取出Vive的projection matrix

	glm::vec3	CamPos = glm::vec3(0.0, 0.0, 1000);
	ViewMat = glm::lookAt(CamPos, glm::vec3(0.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));

	ModelMat = glm::rotate(ModelMat, (GLfloat)pi * 45.0f / 180.0f, glm::vec3(1.0, 1.0, 0.0));
	
	SceneMatrixLocation = glGetUniformLocation(TexCubeShaderHandler.ShaderProgramID, "matrix");
	ViewMatLoc  = glGetUniformLocation(TexCubeShaderHandler.ShaderProgramID, "ViewMat");
	ProjMatLoc  = glGetUniformLocation(TexCubeShaderHandler.ShaderProgramID, "ProjMat");
	ModelMatLoc = glGetUniformLocation(TexCubeShaderHandler.ShaderProgramID, "ModelMat");

	while (!glfwWindowShouldClose(window))
	{
		CamCapture->read(CamFrame);
		ImgChangeBR(CamFrame);

		vr::TrackedDevicePose_t trackedDevicePose[vr::k_unMaxTrackedDeviceCount];
		vr::VRCompositor()->WaitGetPoses(trackedDevicePose, vr::k_unMaxTrackedDeviceCount, nullptr, 0);

		for (int eye = 0; eye < numEyes; ++eye)
		{
			glBindFramebuffer(GL_FRAMEBUFFER, FrameBufferID[eye]);
			glViewport(0, 0, framebufferWidth, framebufferHeight);
			
			glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			DrawBGImage(CamFrame, BGImgShaderHandler);
			
			glEnable(GL_DEPTH_TEST);
			glDepthMask(GL_TRUE);

			TexCubeShaderHandler.Use();

			glBindTexture(GL_TEXTURE_2D, CubeTextureID);

			glUniformMatrix4fv(SceneMatrixLocation, 1, GL_FALSE, MVPMat[eye].get());
			glUniformMatrix4fv(ModelMatLoc, 1, GL_FALSE, glm::value_ptr(ModelMat));
			glUniformMatrix4fv(ViewMatLoc, 1, GL_FALSE, glm::value_ptr(ViewMat));
			glUniformMatrix4fv(ProjMatLoc, 1, GL_FALSE, glm::value_ptr(ProjMat));
			//glUniformMatrix4fv(ViewMatLoc, 1, GL_FALSE, ExtrinsicViewMat);
			//glUniformMatrix4fv(ProjMatLoc, 1, GL_FALSE, IntrinsicProjMat);

			glBindVertexArray(CubeVertexArray);
			glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);

			//ModelShaderHandler.Use();
		
			//ModelMat = glm::mat4(1.0);
			//ModelMat = glm::scale(ModelMat, glm::vec3(1.5, 1.5, 1.5));
			//ModelMat = glm::rotate(ModelMat, (GLfloat)pi * (float)glfwGetTime() * 10.0f / 180.0f,
			//					   glm::vec3(0.0f, 1.0f, 0.0f)); // mat, degree, axis. (use radians)
		
			//glUniformMatrix4fv(SceneMatrixLocation, 1, GL_FALSE, MVPMat[eye].get());
			////glUniformMatrix4fv(ProjMatLoc, 1, GL_FALSE, glm::value_ptr(ProjMat));
			//glUniformMatrix4fv(ViewMatLoc, 1, GL_FALSE, glm::value_ptr(ViewMat));
			//glUniformMatrix4fv(ModelMatLoc, 1, GL_FALSE, glm::value_ptr(ModelMat));
			//glUniform3fv(CamPosLoc, 1, glm::value_ptr(CamPos));

			//ModelShaderHandler.Use();
			//Model.Draw(ModelShaderHandler);

			glUseProgram(0);
		}

		vr::Texture_t LTexture = { reinterpret_cast<void*>(intptr_t(SceneTextureID[Left])), vr::API_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::EVREye(Left), &LTexture);
		vr::Texture_t RTesture = { reinterpret_cast<void*>(intptr_t(SceneTextureID[Right])), vr::API_OpenGL, vr::ColorSpace_Gamma };
		vr::VRCompositor()->Submit(vr::EVREye(Right), &RTesture);

		vr::VRCompositor()->PostPresentHandoff();

		// Mirror to the window
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, GL_NONE);
		glViewport(0, 0, 640, 720);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glBlitFramebuffer(0, 0, framebufferWidth, framebufferHeight, 0, 0, 640, 720, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, GL_NONE);

		glfwSwapBuffers(window);
		glfwPollEvents();	// This function processes only those events that are already 
							// in the event queue and then returns immediately
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

	window = initOpenGL(windowWidth, windowHeight, "kuOpenGLVRTest");


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

	ProjectionMat[Left] = GetHMDMatrixProjectionEye(vr::Eye_Left);
	ProjectionMat[Right] = GetHMDMatrixProjectionEye(vr::Eye_Right);

	EyePoseMat[Left] = GetHMDMatrixPoseEye(vr::Eye_Left);
	EyePoseMat[Right] = GetHMDMatrixPoseEye(vr::Eye_Right);

	MVPMat[Left]  = /*ProjectionMat[Left] **/ EyePoseMat[Left];
	MVPMat[Right] = /*ProjectionMat[Right] **/ EyePoseMat[Right];

	//WriteMVPMatrixFile("LeftMVPMatrix.txt",  MVPMat[Left]);
	//WriteMVPMatrixFile("RightMVPMatrix.txt", MVPMat[Right]);

	// initialize OpenCV video capture
	CamCapture = new VideoCapture(0);

	IntrinsicMat.create(3, 3, CV_32FC1);
	DistParam.create(1, 4, CV_32FC1);
	RotationVec.create(3, 1, CV_64FC1);
	RotationMat.create(3, 3, CV_64FC1);
	TranslationVec.create(3, 1, CV_64FC1);

	if (LoadCameraParameters("IntParam.txt"))
	{
		cout << "Intrinsic Parameters: " << endl;
		DispParam();
		cout << endl;

		IntrinsicCVtoGL(IntrinsicMat, IntrinsicProjMat);

		cout << "OpenGL projection matrix: " << endl;
		for (int i = 0; i < 4; i++)
		{
			cout << IntrinsicProjMat[4 * i] << " " << IntrinsicProjMat[4 * i + 1] << " "
				 << IntrinsicProjMat[4 * i + 2] << " " << IntrinsicProjMat[4 * i + 3] << endl;
		}
		cout << endl;
	}

	SetCB3DPts();
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

GLuint CreateTexturebyImage(Mat Img)
{
	GLuint	texture;

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

void DrawBGImage(Mat BGImg, kuShaderHandler BGShader)
{
	static const GLfloat BGVertices[]
		= {
		1.0f,  1.0f, 1.0f, 0.0f,
		1.0f, -1.0f, 1.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 1.0f,
		-1.0f,  1.0f, 0.0f, 0.0f
	};

	static const GLfloat BGTexCoords[]
		= {
		0.0
	};

	GLuint indices[]
		= { 0, 1, 3,
		1, 2, 3 };

	GLuint BGVertexArray = 0;
	glGenVertexArrays(1, &BGVertexArray);
	GLuint BGVertexBuffer = 0;						// Vertex Buffer Object (VBO)
	glGenBuffers(1, &BGVertexBuffer);				// give an ID to vertex buffer
	GLuint BGElementBuffer = 0;						// Element Buffer Object (EBO)
	glGenBuffers(1, &BGElementBuffer);

	glBindVertexArray(BGVertexArray);

	glBindBuffer(GL_ARRAY_BUFFER, BGVertexBuffer);  // Bind buffer as array buffer
	glBufferData(GL_ARRAY_BUFFER, sizeof(BGVertices), BGVertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, BGElementBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Assign vertex position data
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);
	// Assign texture coordinates
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (GLvoid*)(2 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);

	GLuint BGImgTextureID = CreateTexturebyImage(BGImg);

	BGShader.Use();

	glBindTexture(GL_TEXTURE_2D, BGImgTextureID);

	glBindVertexArray(BGVertexArray);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
	//glBindTexture(GL_TEXTURE_2D, 0);

	glDeleteTextures(1, &BGImgTextureID);

	glDeleteVertexArrays(1, &BGVertexArray);
	glDeleteBuffers(1, &BGVertexBuffer);
	glDeleteBuffers(1, &BGElementBuffer);
}

void ImgChangeBR(Mat & Img)
{
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
}

bool LoadCameraParameters(char * Filename)
{
	FILE	*	fp;
	errno_t		err;

	err = fopen_s(&fp, Filename, "r");
	if (err != 0)
	{
		return false;
	}
	else
	{
		for (int i = 0; i < 3; i++)
		{
			fscanf_s(fp, "%f %f %f\n", &IntrinsicMat.at<float>(i, 0),
				&IntrinsicMat.at<float>(i, 1),
				&IntrinsicMat.at<float>(i, 2));
		}
		fscanf_s(fp, "%f %f %f %f\n", &DistParam.at<float>(0, 0),
			&DistParam.at<float>(0, 1),
			&DistParam.at<float>(0, 2),
			&DistParam.at<float>(0, 3));
		fclose(fp);

		return true;
	}
}

void IntrinsicCVtoGL(Mat IntParam, GLfloat GLProjection[16])
{
	int			i, j;
	double		p[3][3];
	double		q[4][4];

	memset(GLProjection, 0, 16 * sizeof(GLfloat));

	for (i = 0; i < 3; i++)
	{
		for (j = 0; j < 3; j++)
		{
			p[i][j] = IntParam.at<float>(i, j);
		}
	}

	for (i = 0; i < 3; i++)
	{
		p[1][i] = (ImgHeight - 1) * p[2][i] - p[1][i];
	}

	q[0][0] = (2.0 * p[0][0] / (ImgWidth - 1));
	q[0][1] = (2.0 * p[0][1] / (ImgWidth - 1));
	q[0][2] = ((2.0 * p[0][2] / (ImgWidth - 1)) - 1.0);
	q[0][3] = 0.0;

	q[1][0] = 0.0;
	q[1][1] = (2.0 * p[1][1] / (ImgHeight - 1));
	q[1][2] = ((2.0 * p[1][2] / (ImgHeight - 1)) - 1.0);
	q[1][3] = 0.0;

	q[2][0] = 0.0;
	q[2][1] = 0.0;
	q[2][2] = (farPlaneZ + nearPlaneZ) / (farPlaneZ - nearPlaneZ);
	q[2][3] = -2.0 * farPlaneZ * nearPlaneZ / (farPlaneZ - nearPlaneZ);

	q[3][0] = 0.0;
	q[3][1] = 0.0;
	q[3][2] = 1.0;
	q[3][3] = 0.0;

	// transpose
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < 4; j++)
		{
			GLProjection[4 * i + j] = q[j][i];
		}
	}
}

void ExtrinsicCVtoGL(Mat RotMat, Mat TransVec, GLfloat GLModelView[16])
{
	memset(GLModelView, 0, 16 * sizeof(GLfloat));

	for (int i = 0; i < 3; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			GLModelView[4 * i + j] = RotMat.at<double>(j, i);
		}
		GLModelView[12 + i] = TransVec.at<double>(i, 0);
	}
	GLModelView[15] = 1;
}

void DispParam()
{
	for (int i = 0; i < 3; i++)
	{
		printf("%f %f %f\n", IntrinsicMat.at<float>(i, 0), IntrinsicMat.at<float>(i, 1),
			IntrinsicMat.at<float>(i, 2));
	}
	printf("%f %f %f %f\n", DistParam.at<float>(0, 0), DistParam.at<float>(0, 1),
		DistParam.at<float>(0, 2), DistParam.at<float>(0, 3));
}

void SetCB3DPts()
{
	FILE * fp;

	errno_t err = fopen_s(&fp, "CB3DPts.txt", "w");
	for (int i = 0; i < 7; i++)
	{
		for (int j = 0; j < 5; j++)
		{
			CB3DPts.push_back(Point3f(-50 + 25 * j, 75 - 25 * i, 0));

			fprintf_s(fp, "%f %f %f\n", CB3DPts[5 * i + j].x, CB3DPts[5 * i + j].y, CB3DPts[5 * i + j].z);
		}
	}
	fclose(fp);
}