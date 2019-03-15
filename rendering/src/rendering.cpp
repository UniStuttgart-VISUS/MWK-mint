// adapted from https://www.glfw.org/docs/latest/quick_guide.html

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define GLM_FORCE_CTOR_INIT
#define GLM_FORCE_EXPLICIT_CTOR
#include <glm/glm.hpp> // vec2, vec3, mat4, radians
#include <glm/ext.hpp> // perspective, translate, rotate
#include <glm/gtx/transform.hpp> // rotate in degrees around axis

#include <iostream>

#include <interop.hpp>
glm::vec4 toGlmVec(const interop::vec4& v) {
	return glm::vec4{v.x, v.y, v.z, v.w};
}
glm::mat4 toGlmMat(const interop::mat4& m) {
	return glm::mat4{
		toGlmVec(m.data[0]),
		toGlmVec(m.data[1]),
		toGlmVec(m.data[2]),
		toGlmVec(m.data[3])
	};
}

static const struct
{
	float x, y;
	float r, g, b;
} vertices[3] =
{
	{ -0.6f, -0.4f, 1.f, 0.f, 0.f },
	{  0.6f, -0.4f, 0.f, 1.f, 0.f },
	{   0.f,  0.6f, 0.f, 0.f, 1.f }
};

static const char* vertex_shader_source = R"(
	uniform mat4 MVP;
	attribute vec3 vCol;
	attribute vec2 vPos;
	varying vec3 color;
	void main()
	{
		gl_Position = MVP * vec4(vPos, 0.0, 1.0);
		color = vCol;
	}\
)";

static const char* fragment_shader_source = R"(
	varying vec3 color;
	void main()
	{
		gl_FragColor = vec4(color, 1.0);
	}\
)";

static void glfw_error_callback(int error, const char* description)
{
	std::cout << "GLFW Error " << error << ": " << description << std::endl;
}

static void glfw_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void APIENTRY opengl_debug_message_callback(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei length,
	const GLchar* message,
	const void* userParam)
{
	/* Message Sources 
		Source enum                      Generated by
		GL_DEBUG_SOURCE_API              Calls to the OpenGL API
		GL_DEBUG_SOURCE_WINDOW_SYSTEM    Calls to a window - system API
		GL_DEBUG_SOURCE_SHADER_COMPILER  A compiler for a shading language
		GL_DEBUG_SOURCE_THIRD_PARTY      An application associated with OpenGL
		GL_DEBUG_SOURCE_APPLICATION      Generated by the user of this application
		GL_DEBUG_SOURCE_OTHER            Some source that isn't one of these
	*/
	/* Message Types
		Type enum                          Meaning
		GL_DEBUG_TYPE_ERROR                An error, typically from the API
		GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR  Some behavior marked deprecated has been used
		GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR   Something has invoked undefined behavior
		GL_DEBUG_TYPE_PORTABILITY          Some functionality the user relies upon is not portable
		GL_DEBUG_TYPE_PERFORMANCE          Code has triggered possible performance issues
		GL_DEBUG_TYPE_MARKER               Command stream annotation
		GL_DEBUG_TYPE_PUSH_GROUP           Group pushing
		GL_DEBUG_TYPE_POP_GROUP            foo
		GL_DEBUG_TYPE_OTHER                Some type that isn't one of these
	*/
	/* Message Severity
		Severity enum                    Meaning
		GL_DEBUG_SEVERITY_HIGH           All OpenGL Errors, shader compilation / linking errors, or highly - dangerous undefined behavior
		GL_DEBUG_SEVERITY_MEDIUM         Major performance warnings, shader compilation / linking warnings, or the use of deprecated functionality
		GL_DEBUG_SEVERITY_LOW            Redundant state change performance warning, or unimportant undefined behavior
		GL_DEBUG_SEVERITY_NOTIFICATION   Anything that isn't an error or performance issue.
	*/
	if (source == GL_DEBUG_SOURCE_API || source == GL_DEBUG_SOURCE_SHADER_COMPILER)
		if (type == GL_DEBUG_TYPE_ERROR || type == GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR || type == GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR)
			if (severity == GL_DEBUG_SEVERITY_HIGH || severity == GL_DEBUG_SEVERITY_MEDIUM)
				std::cout << "GL Error: " << message << std::endl;
}

int main(void)
{
	GLFWwindow* window;
	GLuint vertex_array, vertex_buffer, vertex_shader, fragment_shader, program;
	GLint mvp_location, vpos_location, vcol_location;
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		exit(EXIT_FAILURE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(640, 480, "Triangle Rendering", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	glfwSetKeyCallback(window, glfw_key_callback);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);
	gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(opengl_debug_message_callback, nullptr);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glClearDepth(1.0f);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glEnable(GL_CULL_FACE);

	glGenVertexArrays(1, &vertex_array);
	glBindVertexArray(vertex_array);
	glGenBuffers(1, &vertex_buffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
	glCompileShader(vertex_shader);
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
	glCompileShader(fragment_shader);
	program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);

	mvp_location = glGetUniformLocation(program, "MVP");
	vpos_location = glGetAttribLocation(program, "vPos");
	vcol_location = glGetAttribLocation(program, "vCol");
	glEnableVertexAttribArray(vpos_location);
	glVertexAttribPointer(vpos_location, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*) 0);
	glEnableVertexAttribArray(vcol_location);
	glVertexAttribPointer(vcol_location, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 5, (void*) (sizeof(float) * 2));

	interop::glFramebuffer fbo; // has RGBA color and depth buffer
	fbo.init();
	interop::TextureSender ts; // has spout sender
	ts.init("render_interop_test");

	interop::DataReceiver modelPoseReceiver;
	modelPoseReceiver.start("tcp://localhost:12345", "ModelPose");
	auto modelPose = interop::ModelPose(); // identity matrix or received from unity

	while (!glfwWindowShouldClose(window))
	{
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		float ratio = width / (float) height;
		// default framebuffer
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// render into custom framebuffer
		if(width != fbo.m_width || height != fbo.m_height) {
			fbo.resizeTexture(width, height);
		}
		fbo.bind();
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		modelPoseReceiver.getData<interop::ModelPose>(modelPose); // if has new data, returns true and overwrites modelPose
		glm::mat4 modelPoseMat = toGlmMat(modelPose.modelMatrix);
		const auto model = modelPoseMat * glm::rotate((float)glfwGetTime(), glm::vec3{0.0f, 0.0f, 1.0f});

		const auto view = glm::lookAt(
			glm::vec3{0.0f, 0.0f, 1.0f}, // eye
			glm::vec3{0.0f}, // center
			glm::vec3{0.0f, 1.0f, 0.0f}); // up
		const auto projection = glm::perspective(90.0f, ratio, 0.1f, 10.0f);
		const auto mvp = projection * view * model;

		glUseProgram(program);
		glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*) glm::value_ptr(mvp));
		glDrawArrays(GL_TRIANGLES, 0, 3);

		fbo.unbind();
		ts.sendTexture(fbo.m_glTextureRGBA8, width, height);
		fbo.blitTexture(); // blit custom fbo to default framebuffer

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	modelPoseReceiver.stop();

	fbo.destroy();
	ts.destroy();

	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}