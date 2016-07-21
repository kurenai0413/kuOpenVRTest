#include <GLEW/glew.h>
#include <GLEW/wglew.h>
#include <GLFW/glfw3.h> 
#include <opencv2/opencv.hpp>

#include <cstring>
#include <cassert>
#include <iostream>

#ifdef _DEBUG
#pragma comment(lib, "opengl32")
#pragma comment(lib, "glew32d")
#pragma comment(lib, "glfw3")
#pragma comment(lib, "opencv_world310d")
#endif

//#define _VR

GLFWwindow* initOpenGL(int width, int height, const std::string& title);

GLFWwindow * window = nullptr;

void main()
{
	const int numEyes = 2;
	uint32_t framebufferWidth = 1280, framebufferHeight = 720;

	const int windowHeight = 720;
	const int windowWidth = (framebufferWidth * windowHeight) / framebufferHeight;

	window = initOpenGL(windowWidth, windowHeight, "minimalOpenGL");

	GLuint framebuffer[numEyes];
	glGenFramebuffers(numEyes, framebuffer);



	system("pause");
}

GLFWwindow* initOpenGL(int width, int height, const std::string& title) {
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
	if (!window) {
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
	{ GLuint vao; glGenVertexArrays(1, &vao); glBindVertexArray(vao); }

	// Check for errors
	{ const GLenum error = glGetError(); assert(error == GL_NONE); }

	return window;
}