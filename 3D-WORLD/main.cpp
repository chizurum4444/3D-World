/************************************************
 *
 *             Example Six
 *
 *  An example showing how tiny obj loader can
 *  be used to load an obj file.  In this case
 * the object is the Blender Monkey
 *
 ************************************************/

#include <gl/glew.h>
#define GLFW_DLL
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "shaders.h"
#include <stdio.h>
#include "tiny_obj_loader.h"
#include <iostream>

/*
 * The following structure represents a single mesh in the scene
*/
struct Mesh {
	GLuint vbuffer;		// vertex buffer name
	GLuint ibuffer;		// index buffer name
	GLuint triangles;	// number of triangles
	GLuint vbytes;		// size of vertices in bytes
	GLuint program;		// program for drawing the mesh
	glm::mat4 model;	// model transformation for the mesh
};

std::vector<Mesh*> Model;	// list of meshes in the scene

double theta, phi;
double r;

float cx, cy, cz;

glm::mat4 projection;	// projection matrix
// float eyex, eyey, eyez;	// eye position

glm::vec3 cameraPos = glm::vec3(0.0f, 0.2f, 15.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
/*
 *  The loadObject procedure loads an object from an OBJ
 *  file and creates a mesh structure for it.  It also
 *  sets up the buffers for vertex information and indices.
 *  The parameter to this procedure is the file name for the model.
 */

Mesh* loadOBJ(char* filename) {
	GLuint vbuffer;
	GLuint ibuffer;
	GLuint objVAO;
	GLfloat* vertices;
	GLfloat* normals;
	GLuint* indices;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	int nv;
	int nn;
	int ni;
	int i;
	float xmin, ymin, zmin;
	float xmax, ymax, zmax;
	float dx, dy, dz;
	float scale;
	int triangles;
	Mesh* mesh;

	glGenVertexArrays(1, &objVAO);
	glBindVertexArray(objVAO);

	/*  Load the obj file */

	std::string err = tinyobj::LoadObj(shapes, materials, filename, 0);

	if (!err.empty()) {
		std::cerr << err << std::endl;
		return(NULL);
	}

	/*  Retrieve the vertex coordinate data */

	nv = shapes[0].mesh.positions.size();
	vertices = new GLfloat[nv];
	for (i = 0; i < nv; i++) {
		vertices[i] = shapes[0].mesh.positions[i];
	}

	/*
	 *  Find the range of the x, y and z
	 *  coordinates.
	 */

	xmin = ymin = zmin = 1000000.0;
	xmax = ymax = zmax = -1000000.0;
	for (i = 0; i < nv / 3; i++) {
		if (vertices[3 * i] < xmin)
			xmin = vertices[3 * i];
		if (vertices[3 * i] > xmax)
			xmax = vertices[3 * i];
		if (vertices[3 * i + 1] < ymin)
			ymin = vertices[3 * i + 1];
		if (vertices[3 * i + 1] > ymax)
			ymax = vertices[3 * i + 1];
		if (vertices[3 * i + 2] < zmin)
			zmin = vertices[3 * i + 2];
		if (vertices[3 * i + 2] > zmax)
			zmax = vertices[3 * i + 2];
	}

	/*
	 *  Scales the vertices so the longest axis is unit length
	*/
	
	dx = xmax - xmin;
	dy = ymax - ymin;
	dz = zmax - zmin;

	scale = dx;
	if (dy > scale)
		scale = dy;
	if (dz > scale)
		scale = dz;

	scale = 1.0 / scale;
	for (i = 0; i < nv; i++) {
		vertices[i] = vertices[i] * scale;
	}
	
	/*  Retrieve the vertex normals */

	nn = shapes[0].mesh.normals.size();
	normals = new GLfloat[nn];
	for (i = 0; i < nn; i++) {
		normals[i] = shapes[0].mesh.normals[i];
	}

	/*  Retrieve the triangle indices */

	ni = shapes[0].mesh.indices.size();
	triangles = ni / 3;
	indices = new GLuint[ni];
	for (i = 0; i < ni; i++) {
		indices[i] = shapes[0].mesh.indices[i];
	}

	/*
	 *  load the vertex coordinate data
	 */
	glGenBuffers(1, &vbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vbuffer);
	glBufferData(GL_ARRAY_BUFFER, (nv + nn) * sizeof(GLfloat), NULL, GL_STATIC_DRAW);
	glBufferSubData(GL_ARRAY_BUFFER, 0, nv * sizeof(GLfloat), vertices);
	glBufferSubData(GL_ARRAY_BUFFER, nv * sizeof(GLfloat), nn * sizeof(GLfloat), normals);

	/*
	 *  load the vertex indexes
	 */
	glGenBuffers(1, &ibuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, ni * sizeof(GLuint), indices, GL_STATIC_DRAW);

	/*
	 *  Allocate the mesh structure and fill it in
	*/
	mesh = new Mesh();
	mesh->vbuffer = vbuffer;
	mesh->ibuffer = ibuffer;
	mesh->triangles = triangles;
	mesh->vbytes = nv * sizeof(GLfloat);

	delete[] vertices;
	delete[] normals;
	delete[] indices;

	return(mesh);
}

/*
 *  The loadProgram procedure reads the vertex and fragment programs
 *  compiles them, and load them to produce the complete GPU program.
 *  The parameters to this procedure and the file names for the vertex
 *  and fragment programs.
*/

GLuint loadProgram(char* vertex, char* fragment) {
	GLuint vs;
	GLuint fs;
	GLuint program;

	/*
	 *  compile and build the shader program
	 */
	vs = buildShader(GL_VERTEX_SHADER, vertex);
	fs = buildShader(GL_FRAGMENT_SHADER, fragment);
	program = buildProgram(vs, fs, 0);

	return(program);
}

/*
 *  The copyObject procedure makes a copy of an existing mesh.  This
 *  makes it easier to create multiple copies of an object.  The
 *  paraneter to this procedure is the mesh to be copied.
*/

Mesh* copyObject(Mesh* obj) {
	Mesh* result;

	result = new Mesh();
	*result = *obj;
	return(result);
}

Mesh* cylinder(double radius, double height, int sides) {
	double* x;		// the x coordinates of the vertices
	double* y;		// the y coordinates of the vertices
	Mesh* result;
	GLfloat normals[] = { 0.0f,1.0f,0.0f };
	GLuint vao;
	double angle;
	double dangle;
	int i;
	GLfloat* vertices;	// vertices to be stored in the buffer
	GLuint* indices;	// triangle indices to be stored in the buffer
	int j;
	int base;
	GLuint vbuffer;
	GLuint ibuffer;
	GLint vPosition;

	result = new Mesh;

	
	/*result->vao = vao;
	result->indices = 3 * 4 * sides;
	*/

	/*
	 *  Compute the x and y coordinates of the vertices
	 *  around the edge of the sylinder
	 */
	x = new double[sides];
	y = new double[sides];
	dangle = 6.28 / sides;
	angle = 0.0;
	for (i = 0; i < sides; i++) {
		x[i] = radius * cos(angle);
		y[i] = radius * sin(angle);
		angle += dangle;
	}

	vertices = new GLfloat[3 * 2 * (sides + 1)];
	j = 0;

	/*  vertices on the bottom of the cylinder */
	vertices[j++] = 0.0;
	vertices[j++] = 0.0;
	vertices[j++] = 0.0;
	for (i = 0; i < sides; i++) {
		vertices[j++] = x[i];
		vertices[j++] = y[i];
		vertices[j++] = 0.0;
	}

	/* vertices on the top of the cylinder */
	vertices[j++] = 0.0;
	vertices[j++] = 0.0;
	vertices[j++] = height;
	for (i = 0; i < sides; i++) {
		vertices[j++] = x[i];
		vertices[j++] = y[i];
		vertices[j++] = height;
	}

	/*
	 *  Now construct the triangle indices
	 */
	indices = new GLuint[3 * 4 * sides];
	j = 0;

	/* triangles of the bottom of the cylinder */
	for (i = 0; i < sides; i++) {
		indices[j++] = 0;
		indices[j++] = i + 1;
		indices[j++] = i + 2;
	}
	indices[j - 1] = 1;

	/* triangles of the top of the cylinder */
	base = sides + 1;
	for (i = 0; i < sides; i++) {
		indices[j++] = base;
		indices[j++] = base + i + 1;
		indices[j++] = base + i + 2;
	}
	indices[j - 1] = base + 1;

	/* triangles on the sides of the cylinder */
	for (i = 1; i < sides; i++) {
		indices[j++] = i;
		indices[j++] = base + i;
		indices[j++] = i + 1;
		indices[j++] = base + i;
		indices[j++] = base + i + 1;
		indices[j++] = i + 1;
	}
	indices[j++] = sides;
	indices[j++] = base + sides;
	indices[j++] = 1;
	indices[j++] = base + sides;
	indices[j++] = base + 1;
	indices[j++] = 1;

	/*
	 *  Now copy the vertices and indices into the appropriate buffers
	 */

	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glGenBuffers(1, &vbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vbuffer);
	glBufferData(GL_ARRAY_BUFFER, 3 * 2 * (sides + 1) * sizeof(GLfloat), vertices, GL_STATIC_DRAW);

	glGenBuffers(1, &ibuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * 4 * sides * sizeof(GLuint), indices, GL_STATIC_DRAW);
	
	result->vbuffer = vbuffer;
	result->ibuffer = ibuffer; 
	result->triangles =  4 * sides;
	result->vbytes = sizeof(vertices);
	
	delete[] vertices;
	delete[] indices;

	return(result);
}

// This procedure create a rectangular plane object made of 2 triangles. The other objects will be placed on this mesh
Mesh* planeObject() {
    Mesh* mesh;
    GLuint vbuffer;
    GLuint ibuffer;
    GLuint objVAO;
    GLfloat normals[] = { 0.0f,1.0f,0.0f };
    GLfloat vertices[] = {
         5.0f,0.0f,5.0f,
         5.0f,0.0f,-5.0f,
        -5.0f,0.0f,-5.0f,
        -5.0f,0.0f,5.0f
    };
    GLuint indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    glGenVertexArrays(1, &objVAO);
    glBindVertexArray(objVAO);

    glGenBuffers(1, &vbuffer);
    glBindBuffer(GL_ARRAY_BUFFER, vbuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices) + sizeof(normals), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBufferSubData(GL_ARRAY_BUFFER,  sizeof(vertices), sizeof(normals), normals);

    glGenBuffers(1, &ibuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    mesh = new Mesh();
    mesh->vbuffer = vbuffer;
    mesh->ibuffer = ibuffer;
    mesh->triangles = 2;
    mesh->vbytes = sizeof(vertices);

    return(mesh);
}

/*
 *  The buildModel procedure constructs the scene from the indivdual meshes.
*/
void buildModel() {
	Mesh* mesh;
	GLuint program, groundProgram, bunnyProgram, humanProgram, horseProgram, sunProgram, leavesProgram;

	// Use the same GPU program for all of the meshes.

	groundProgram = loadProgram((char*)"/Users/cza/Desktop/Computer Graphics/CSCI 3090/Assignment1/example6.vs", (char*)"/Users/cza/Desktop/Computer Graphics/CSCI 3090/Assignment1/ground.fs");
	bunnyProgram = loadProgram((char*)"/Users/cza/Desktop/Computer Graphics/CSCI 3090/Assignment1/example6.vs", (char*)"/Users/cza/Desktop/Computer Graphics/CSCI 3090/Assignment1/bunny.fs");
	humanProgram = loadProgram((char*)"/Users/cza/Desktop/Computer Graphics/CSCI 3090/Assignment1/example6.vs", (char*)"/Users/cza/Desktop/Computer Graphics/CSCI 3090/Assignment1/human.fs");
	horseProgram = loadProgram((char*)"/Users/cza/Desktop/Computer Graphics/CSCI 3090/Assignment1/example6.vs", (char*)"/Users/cza/Desktop/Computer Graphics/CSCI 3090/Assignment1/horse.fs");
	sunProgram = loadProgram((char*)"/Users/cza/Desktop/Computer Graphics/CSCI 3090/Assignment1/example6.vs", (char*)"/Users/cza/Desktop/Computer Graphics/CSCI 3090/Assignment1/sun.fs");
	leavesProgram = loadProgram((char*)"/Users/cza/Desktop/Computer Graphics/CSCI 3090/Assignment1/example6.vs", (char*)"/Users/cza/Desktop/Computer Graphics/CSCI 3090/Assignment1/leaves.fs");
	

	// Start with the bunny

	mesh = loadOBJ((char*)"/Users/cza/Desktop/Computer Graphics/CSCI 3090/Assignment1/bunny.obj");
	mesh->model = glm::translate(glm::mat4(1.0), glm::vec3(2.0, -0.2, 1.0));
	mesh->model = glm::scale(mesh->model, glm::vec3(0.3, 0.3, 0.3));
	mesh->model = glm::rotate(mesh->model, 1.57f, glm::vec3(1.0, 0.0, 0.0));
	mesh->program = bunnyProgram;
	Model.push_back(mesh);

	// Now load the human

	mesh = loadOBJ((char*)"/Users/cza/Desktop/Computer Graphics/CSCI 3090/Assignment1/human.obj");
	mesh->model = glm::scale(mesh->model, glm::vec3(2.0, 2.0, 2.0));
	mesh->model = glm::translate(glm::mat4(1.0), glm::vec3(2.0, 0.0, 0.0));
	mesh->model = glm::rotate(mesh->model, 0.0f, glm::vec3(1.0, 0.0, 0.0));
	mesh->model = glm::rotate(mesh->model, -1.57f, glm::vec3(0.0, 1.0, 0.0));
	mesh->program = humanProgram;
	Model.push_back(mesh);

	// Make a copy of the human at a different location

	mesh = copyObject(mesh);
	mesh->model = glm::translate(glm::mat4(1.0), glm::vec3(-2.0, 0.0, 0.0));
	mesh->model = glm::rotate(mesh->model, 0.0f, glm::vec3(1.0, 0.0, 0.0));
	mesh->model = glm::rotate(mesh->model, 1.57f, glm::vec3(0.0, 1.0, 0.0));
	Model.push_back(mesh);

	// Horse

	mesh = loadOBJ((char*)"/Users/cza/Desktop/Computer Graphics/CSCI 3090/Assignment1/horse.obj");
	mesh->program = horseProgram;
	mesh->model = glm::translate(glm::mat4(1.0), glm::vec3(-2.0, 0.0, 2.3));
	mesh->model = glm::rotate(mesh->model, 1.57f, glm::vec3(0.0, 1.0, 0.0));
	mesh->model = glm::rotate(mesh->model, -1.57f, glm::vec3(1.0, 0.0, 0.0));
	//mesh->model = glm::rotate(mesh->model, -0.5f, glm::vec3(0.0, 0.0, 0.0));
	Model.push_back(mesh);

	// Sun

	mesh = loadOBJ((char*)"/Users/cza/Desktop/Computer Graphics/CSCI 3090/Assignment1/sun.obj");
	mesh->program = sunProgram;
	mesh->model = glm::translate(glm::mat4(1.0), glm::vec3(5.0, 4.0, 0.0));
	Model.push_back(mesh);

	// Trees

	mesh = cylinder(0.2,1.5,10);
	mesh->program = horseProgram;
	mesh->model = glm::translate(glm::mat4(1.0), glm::vec3(3.0, 0.0, 0.0));
	mesh->model = glm::rotate(mesh->model, -1.57f, glm::vec3(1.0, 0.0, 0.0));
	Model.push_back(mesh);

	mesh = copyObject(mesh);
	mesh->model = glm::translate(glm::mat4(1.0), glm::vec3(-3.0, 0.0, 0.0));
	mesh->model = glm::rotate(mesh->model, -1.57f, glm::vec3(1.0, 0.0, 0.0));
	Model.push_back(mesh);

	mesh = cylinder(2.0,0.4,10);
	mesh->program = leavesProgram;
	mesh->model = glm::translate(glm::mat4(1.0), glm::vec3(3.0, 1.5, 0.0));
	mesh->model = glm::rotate(mesh->model, -1.57f, glm::vec3(1.0, 0.0, 0.0));
	Model.push_back(mesh);

	mesh = copyObject(mesh);
	mesh->model = glm::translate(glm::mat4(1.0), glm::vec3(-3.0, 1.5, 0.0));
	mesh->model = glm::rotate(mesh->model, -1.57f, glm::vec3(1.0, 0.0, 0.0));
	Model.push_back(mesh);

	// Plane

	mesh = planeObject();
	mesh->model = glm::translate(glm::mat4(1.0), glm::vec3(0.0, 0.0, -0.5));
	mesh->model = glm::rotate(mesh->model, 0.1f, glm::vec3(1.0, 0.0, 0.0));
	mesh->program = groundProgram;
	Model.push_back(mesh);

}


/*
 *  Executed each time the window is resized,
 *  usually once at the start of the program.
 */
void framebufferSizeCallback(GLFWwindow *window, int w, int h) {

	// Prevent a divide by zero, when window is too short
	// (you cant make a window of zero width).

	if (h == 0)
		h = 1;

	float ratio = 1.0f * w / h;

	glfwMakeContextCurrent(window);

	glViewport(0, 0, w, h);

	projection = glm::perspective(0.8f, ratio, 1.0f, 800.0f);

}

/*
 *  This procedure is called each time the screen needs
 *  to be redisplayed
 */
void display() {
	glm::mat4 view;
	glm::mat4 modelViewPerspective;
	glm::mat3 normal;
	int modelLoc;
	int normalLoc;
	int i;
	Mesh* mesh;
	GLuint vPosition;
	GLuint vNormal;

	view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);					

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Iterate through the model displaying each of the meshes

	for (i = 0; i < Model.size(); i++) {
		mesh = Model[i];
		glUseProgram(mesh->program);

		// Set up the position and normal attributes

		glBindBuffer(GL_ARRAY_BUFFER, mesh->vbuffer);
		vPosition = glGetAttribLocation(mesh->program, "vPosition");
		glVertexAttribPointer(vPosition, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(vPosition);
		vNormal = glGetAttribLocation(mesh->program, "vNormal");
		glVertexAttribPointer(vNormal, 3, GL_FLOAT, GL_FALSE, 0, (void*)(mesh->vbytes));
		glEnableVertexAttribArray(vNormal);

		// Set up the transformation matrices for the vertices and normal vectors

		modelLoc = glGetUniformLocation(mesh->program, "model");
		modelViewPerspective = projection * view * mesh->model;
		glUniformMatrix4fv(modelLoc, 1, 0, glm::value_ptr(modelViewPerspective));
		normalLoc = glGetUniformLocation(mesh->program, "normalMat");
		normal = glm::transpose(glm::inverse(glm::mat3(view*mesh->model)));
		glUniformMatrix3fv(normalLoc, 1, 0, glm::value_ptr(normal));

		// Finally draw the mesh

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ibuffer);
		glDrawElements(GL_TRIANGLES, 3 * mesh->triangles, GL_UNSIGNED_INT, NULL);
	}
}

/*
 *  Called each time a key is pressed on
 *  the keyboard.
 */
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	const float cameraSpeed = 0.15f;

	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);

	if (key == GLFW_KEY_A && action == GLFW_PRESS)
		cameraFront-= glm::normalize(glm::cross(cameraFront, cameraUp)) * (float)cos(theta)* cameraSpeed;
	if (key == GLFW_KEY_D && action == GLFW_PRESS)
		cameraFront += glm::normalize(glm::cross(cameraFront, cameraUp)) * (float)cos(theta) * cameraSpeed;
	if (key == GLFW_KEY_W && action == GLFW_PRESS)
		cameraPos += cameraSpeed * cameraFront;
	if (key == GLFW_KEY_S && action == GLFW_PRESS)
		cameraPos -= cameraSpeed * cameraFront;


}

void error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

int main(int argc, char **argv) {
	GLFWwindow *window;

	// start by setting error callback in case something goes wrong

	glfwSetErrorCallback(error_callback);

	// initialize glfw

	if (!glfwInit()) {
		fprintf(stderr, "can't initialize GLFW\n");
	}

	// create the window used by our application
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); 
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1); 
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); 
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
	window = glfwCreateWindow(768, 768, "Assignment 1", NULL, NULL);

	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	// establish framebuffer size change and input callbacks

	glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);

	glfwSetKeyCallback(window, key_callback);
	
	/*
	 *  initialize glew
	 */
	glfwMakeContextCurrent(window);
	glewExperimental = GL_TRUE;
	GLenum error = glewInit();
	if(error != GLEW_OK) {
		printf("Error starting GLEW: %s\n",glewGetErrorString(error));
		exit(0);
	}
	
	// eyex = 0.0;
	// eyey = -0.5;
	// eyez = 6.97;

	theta = 0.1;
	// phi = -1.5;
	// r = 7.0;
	
	buildModel();

	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0,1.0,1.0,0.0);
	glViewport(0, 0, 768, 768);

	projection = glm::perspective(0.8f, 1.0f, 1.0f, 800.0f);

	glfwSwapInterval(1);

	// GLFW main loop, display model, swapbuffer and check for input

	while (!glfwWindowShouldClose(window)) {
		display();
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glfwTerminate();

}