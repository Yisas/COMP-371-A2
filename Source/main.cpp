#include "glew.h"		// include GL Extension Wrangler

#include "glfw3.h"  // include GLFW helper library

#include "glm.hpp"
#include "gtc/matrix_transform.hpp"
#include "gtc/type_ptr.hpp"
#include "gtc/constants.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <cctype>

using namespace std;

#define M_PI        3.14159265358979323846264338327950288   /* pi */
#define DEG_TO_RAD	M_PI/180.0f

enum RenderType { POINTS, LINES, LINE_STRIP };

GLFWwindow* window = 0x00;

// Window dimensions
const GLuint WIDTH = 800, HEIGHT = 600;

GLuint shader_program = 0;

GLuint view_matrix_id = 0;
GLuint model_matrix_id = 0;
GLuint proj_matrix_id = 0;


///Transformations
glm::mat4 proj_matrix;
glm::mat4 view_matrix;
glm::mat4 model_matrix;


GLuint VBO, VAO, EBO;

GLfloat point_size = 3.0f;

vector<GLfloat> vertices;
vector<GLuint> indices;

// Begin globals.
static int numVal = 0; // Current selection index.
static float lengthArrowLine = 2.0; // Length of arrow line.
static int numVertices = 50; // Number of vertices on cubic.

int numberOfControlPoints;
std::vector<float*> controlPointsTest = std::vector<float*>();
std::vector<float*> tangentVectorsTest = std::vector<float*>();

// Control points.
static float controlPoints[2][3] =
{
	{ -0.5, 0.5, 0.0 }, { 0.5, 0.5, 0.0 }
};

// Tangent vectors at control points.
static float tangentVectors[2][3] =
{
	{ 0.0, 10.0, 0.0 }, { 10.0, 0.0, 0.0 }
};

// Lengths of tangent vectors.
static float squareLengthTangent[2];

// End points of tangent vectors.
static float endPointTangentVectors[2][3];

// End points of arrow lines.
static float endPointArrowLines[4][3];;
// End globals.

void indicesInitialize(){
	for (int i = 0; i < 5; i++)
		indices.push_back(i);
}


// Routine to compute tangent vector endpoints by adding tangent vector to control point vector.
void computeEndPointTangentVectors(void)
{
	for (int i = 0; i < 2; i++)
		for (int j = 0; j < 3; j++)
			endPointTangentVectors[i][j] = controlPoints[i][j] + tangentVectors[i][j];
}

// Routine to compute arrow line endpoints.
void computeEndPointArrowLines(void)
{
	for (int i = 0; i < 2; i++)
	{
		squareLengthTangent[i] = tangentVectors[i][0] * tangentVectors[i][0] +
			tangentVectors[i][1] * tangentVectors[i][1];

		if (squareLengthTangent[i] != 0.0)
		{

			endPointArrowLines[2 * i][0] = endPointTangentVectors[i][0] -
				lengthArrowLine *
				(tangentVectors[i][0] - tangentVectors[i][1]) /
				sqrt(2.0 * squareLengthTangent[i]);
			endPointArrowLines[2 * i][1] = endPointTangentVectors[i][1] -
				lengthArrowLine *
				(tangentVectors[i][0] + tangentVectors[i][1]) /
				sqrt(2.0 * squareLengthTangent[i]);
			endPointArrowLines[2 * i][2] = 0.0;

			endPointArrowLines[2 * i + 1][0] = endPointTangentVectors[i][0] -
				lengthArrowLine *
				(tangentVectors[i][0] + tangentVectors[i][1]) /
				sqrt(2.0 * squareLengthTangent[i]);
			endPointArrowLines[2 * i + 1][1] = endPointTangentVectors[i][1] -
				lengthArrowLine *
				(-tangentVectors[i][0] + tangentVectors[i][1]) /
				sqrt(2.0 * squareLengthTangent[i]);
			endPointArrowLines[2 * i + 1][2] = 0.0;
		}
	}
}

///Handle the keyboard input
void keyPressed(GLFWwindow *_window, int key, int scancode, int action, int mods) {

	float diam = 100.0f;
	float zNear = 1.0;
	float zFar = zNear + diam;

	float cx = 0.0f;
	float cy = 0.0f;
	float cz = 0.0f;

	GLdouble left = cx - diam;
	GLdouble right = cx + diam;
	GLdouble bottom = cy - diam;
	GLdouble top = cy + diam;
	GLdouble aspect;

	switch (key) {
	case GLFW_KEY_UP:
		view_matrix = glm::rotate(view_matrix, 0.1f, glm::vec3(1.0, 0.0, 0.0));
		break;
	case GLFW_KEY_DOWN:
		glm::lookAt(glm::vec3(0.5, 0.5, 0.0), glm::vec3(200.0, 0.0, 0.0),glm::vec3(0.0,1.0,0.0));
		break;
	default: break;
	}
	return;
}

bool initialize() {
	/// Initialize GL context and O/S window using the GLFW helper library
	if (!glfwInit()) {
		fprintf(stderr, "ERROR: could not start GLFW3\n");
		return false;
	}

	glfwWindowHint(GLFW_DOUBLEBUFFER, GL_TRUE);
	window = glfwCreateWindow(WIDTH, HEIGHT, "COMP371: Assignment 2", NULL, NULL);
	if (!window) {
		fprintf(stderr, "ERROR: could not open window with GLFW3\n");
		glfwTerminate();
		return false;
	}

	int w, h;
	glfwGetWindowSize(window, &w, &h);
	///Register the keyboard callback function: keyPressed(...)
	glfwSetKeyCallback(window, keyPressed);

	glfwMakeContextCurrent(window);

	/// Initialize GLEW extension handler
	glewExperimental = GL_TRUE;	///Needed to get the latest version of OpenGL
	glewInit();

	/// Get the current OpenGL version
	const GLubyte* renderer = glGetString(GL_RENDERER); /// Get renderer string
	const GLubyte* version = glGetString(GL_VERSION); /// Version as a string
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	/// Enable the depth test i.e. draw a pixel if it's closer to the viewer
	glEnable(GL_DEPTH_TEST); /// Enable depth-testing
	glDepthFunc(GL_LESS);	/// The type of testing i.e. a smaller value as "closer"

	return true;
}

bool cleanUp() {
	glDisableVertexAttribArray(0);
	//Properly de-allocate all resources once they've outlived their purpose
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);

	// Close GL context and any other GLFW resources
	glfwTerminate();

	return true;
}

GLuint loadShaders(std::string vertex_shader_path, std::string fragment_shader_path)	{
	// Create the shaders
	GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
	GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	// Read the Vertex Shader code from the file
	std::string VertexShaderCode;
	std::ifstream VertexShaderStream(vertex_shader_path, std::ios::in);
	if (VertexShaderStream.is_open()) {
		std::string Line = "";
		while (getline(VertexShaderStream, Line))
			VertexShaderCode += "\n" + Line;
		VertexShaderStream.close();
	}
	else {
		printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_shader_path.c_str());
		getchar();
		exit(-1);
	}

	// Read the Fragment Shader code from the file
	std::string FragmentShaderCode;
	std::ifstream FragmentShaderStream(fragment_shader_path, std::ios::in);
	if (FragmentShaderStream.is_open()) {
		std::string Line = "";
		while (getline(FragmentShaderStream, Line))
			FragmentShaderCode += "\n" + Line;
		FragmentShaderStream.close();
	}

	GLint Result = GL_FALSE;
	int InfoLogLength;

	// Compile Vertex Shader
	printf("Compiling shader : %s\n", vertex_shader_path.c_str());
	char const * VertexSourcePointer = VertexShaderCode.c_str();
	glShaderSource(VertexShaderID, 1, &VertexSourcePointer, nullptr);
	glCompileShader(VertexShaderID);

	// Check Vertex Shader
	glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> VertexShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(VertexShaderID, InfoLogLength, nullptr, &VertexShaderErrorMessage[0]);
		printf("%s\n", &VertexShaderErrorMessage[0]);
	}

	// Compile Fragment Shader
	printf("Compiling shader : %s\n", fragment_shader_path.c_str());
	char const * FragmentSourcePointer = FragmentShaderCode.c_str();
	glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer, nullptr);
	glCompileShader(FragmentShaderID);

	// Check Fragment Shader
	glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> FragmentShaderErrorMessage(InfoLogLength + 1);
		glGetShaderInfoLog(FragmentShaderID, InfoLogLength, nullptr, &FragmentShaderErrorMessage[0]);
		printf("%s\n", &FragmentShaderErrorMessage[0]);
	}

	// Link the program
	printf("Linking program\n");
	GLuint ProgramID = glCreateProgram();
	glAttachShader(ProgramID, VertexShaderID);
	glAttachShader(ProgramID, FragmentShaderID);

	glBindAttribLocation(ProgramID, 0, "in_Position");

	//appearing in the vertex shader.
	glBindAttribLocation(ProgramID, 1, "in_Color");

	glLinkProgram(ProgramID);

	// Check the program
	glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
	glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0) {
		std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
		glGetProgramInfoLog(ProgramID, InfoLogLength, nullptr, &ProgramErrorMessage[0]);
		printf("%s\n", &ProgramErrorMessage[0]);
	}

	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	//The three variables below hold the id of each of the variables in the shader
	//If you read the vertex shader file you'll see that the same variable names are used.
	view_matrix_id = glGetUniformLocation(ProgramID, "view_matrix");
	model_matrix_id = glGetUniformLocation(ProgramID, "model_matrix");
	proj_matrix_id = glGetUniformLocation(ProgramID, "proj_matrix");

	return ProgramID;
}

void glBufferStart(){
	
	indicesInitialize();

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	// Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, indices.size() * sizeof(vertices[0]), &vertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(indices[0]), &indices[0], GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);
}

void commonDrawArrays(RenderType type){

	glBufferStart();

	glBindVertexArray(VAO);

	switch (type)
	{
	case POINTS:
		glDrawArrays(GL_POINTS, 0, vertices.size() / 3);
		break;
	case LINES:
		glDrawArrays(GL_LINES, 0, vertices.size() / 3);
		break;
	case LINE_STRIP:
		glDrawArrays(GL_LINE_STRIP, 0, vertices.size() / 3);
		break;
	default:
		break;
	}

	glBindVertexArray(0);
}

void startUserInput(){

	cout << "Enter the number of control points to be used:\n";
	cin >> numberOfControlPoints;
	
	for (int i = 0; i < numberOfControlPoints; i++)
	{
		float* tempArray= new float[3];

		cout << "Enter X value for control point " << i << " as a float:\n";
		cin >> tempArray[0];

		cout << "Enter Y value for control point " << i << " as a float:\n";
		cin >> tempArray[1];

		cout << "Enter Z value for control point " << i << " as a float:\n";
		cin >> tempArray[2];

		controlPointsTest.push_back(tempArray);

		float* tempArray2 = new float[3];

		cout << "Enter a value for tangent vector of point " << i << " as a float:\n";
		cin >> tempArray2[0];

		cout << "Enter b value for tangent vector of point " << i << " as a float:\n";
		cin >> tempArray2[1];

		cout << "Enter c value for tangent vector of point " << i << " as a float:\n";
		cin >> tempArray2[2];

		tangentVectorsTest.push_back(tempArray2);
	}
		
}

int main() {

	startUserInput();

	initialize();

	///Load the shaders
	shader_program = loadShaders("../Source/COMP371_hw1.vs", "../Source/COMP371_hw1.fss");

	// Game loop
	while (!glfwWindowShouldClose(window))
	{
		// Check if any events have been activiated (key pressed, mouse moved etc.) and call corresponding response functions
		glfwPollEvents();

		// Render
		// Clear the colorbuffer
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(0.1f, 0.2f, 0.2f, 1.0f);
		glPointSize(point_size);

		// Draw our first triangle
		glUseProgram(shader_program);

		//Pass the values of the three matrices to the shaders
		glUniformMatrix4fv(proj_matrix_id, 1, GL_FALSE, glm::value_ptr(proj_matrix));
		glUniformMatrix4fv(view_matrix_id, 1, GL_FALSE, glm::value_ptr(view_matrix));
		glUniformMatrix4fv(model_matrix_id, 1, GL_FALSE, glm::value_ptr(model_matrix));

		int i, j;
		float u, H0, H1, H2, H3;

		computeEndPointTangentVectors(); // Compute tangent vector endpoints.
		computeEndPointArrowLines(); // Compute arrow endpoints.

		vertices.clear();

		for (i = 0; i < 2; i++)
			for (j = 0; j < 3; j++)
				vertices.push_back(controlPoints[i][j]);
				
		commonDrawArrays(RenderType::POINTS);

		vertices.clear();

		// Draw the tangent vectors as arrows.
		for (i = 0; i < 2; i++)
		{
			if (squareLengthTangent[i] != 0.0)
			{
				for (j = 0; j < 3; j++)
					vertices.push_back(controlPoints[i][j]);


				for (j = 0; j < 3; j++)
					vertices.push_back(endPointTangentVectors[i][j]);

				for (j = 0; j < 3; j++)
					vertices.push_back(endPointTangentVectors[i][j]);

				for (j = 0; j < 3; j++)
					vertices.push_back(endPointArrowLines[2 * i][j]);

				for (j = 0; j < 3; j++)
					vertices.push_back(endPointTangentVectors[i][j]);
				
				for (j = 0; j < 3; j++)
					vertices.push_back(endPointArrowLines[2 * i][j]);

			}
			else
			{
				for (j = 0; j < 3; j++)
					vertices.push_back(controlPoints[i][j]);
			}
		}

		commonDrawArrays(RenderType::LINES);

		vertices.clear();

		// Draw the cubic curve as a line strip.
		for (i = 0; i <= numVertices; ++i)
		{
			u = (float)i / numVertices;
			H0 = 2.0*u*u*u - 3 * u*u + 1.0;
			H1 = -2.0*u*u*u + 3 * u*u;
			H2 = u*u*u - 2.0*u*u + u;
			H3 = u*u*u - u*u;
			vertices.push_back(H0*controlPoints[0][0] + H1*controlPoints[1][0] +
				H2*tangentVectors[0][0] + H3*tangentVectors[1][0]);
			vertices.push_back(H0*controlPoints[0][1] + H1*controlPoints[1][1] +
				H2*tangentVectors[0][1] + H3*tangentVectors[1][1]);
			vertices.push_back(0.0);
		}

		commonDrawArrays(RenderType::LINE_STRIP);
		
		// Swap the screen buffers
		glfwSwapBuffers(window);
	}

	cleanUp();
	return 0;
}


