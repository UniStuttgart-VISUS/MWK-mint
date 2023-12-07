// adapted from https://www.glfw.org/docs/latest/quick_guide.html

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define GLM_FORCE_CTOR_INIT
#define GLM_FORCE_EXPLICIT_CTOR
#include <glm/glm.hpp> // vec2, vec3, mat4, radians
#include <glm/ext.hpp> // perspective, translate, rotate
#include <glm/gtx/transform.hpp> // rotate in degrees around axis

#include <iostream>
#include <vector>
#include <fstream>
#include <chrono>
#include <thread>
#include <algorithm>

#include <CLI/CLI.hpp>

#include <interop.hpp>
glm::vec4 toGlm(const mint::vec4& v) {
	return glm::vec4{ v.x, v.y, v.z, v.w };
}
mint::vec4 toInterop(const glm::vec3& v) {
	return mint::vec4{ v.x, v.y, v.z, 0.0f };
}
glm::mat4 toGlm(const mint::mat4& m) {
	return glm::mat4{
		toGlm(m.data[0]),
		toGlm(m.data[1]),
		toGlm(m.data[2]),
		toGlm(m.data[3])
	};
}

struct Vertex
{
	float x, y, z;
	float r, g, b;
};

struct RenderVertices {
	GLuint vertex_array = 0, vertex_buffer = 0;
	GLuint element_array = 0;

	void init(const std::vector<Vertex>& vertices) {
		glGenVertexArrays(1, &vertex_array);
		glBindVertexArray(vertex_array);
		glGenBuffers(1, &vertex_buffer);
		glGenBuffers(1, &element_array);
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
		unbind();
	}

	void setElements(const std::vector<unsigned int>& elements) {
		bind();
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * elements.size(), elements.data(), GL_STATIC_DRAW);
		unbind();
	}

	void bind() {
		glBindVertexArray(vertex_array);
		glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, element_array);
	}
	void unbind() {
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	void destroy() {
		unbind();
		glDeleteVertexArrays(1, &vertex_array);
		glDeleteBuffers(1, &vertex_buffer);
		glDeleteBuffers(1, &element_array);
		vertex_array = 0;
		vertex_buffer = 0;
		element_array = 0;
	}
};

const glm::vec3 offset{ 1.0f, 0.7f, 0.3f };
const auto& o = offset;

const std::vector<Vertex> quadVertices =
{   // pos(x,y,z)   , col(r,g,b)
	Vertex{o.x + -1.0f,o.y + -0.5f,o.z + 0.0f, 1.f, 0.f, 0.f },
	Vertex{o.x + 1.0f,o.y + -0.5f,o.z + 0.0f, 0.f, 1.f, 0.f },
	Vertex{o.x + 1.0f,o.y + 0.5f,o.z + 0.0f, 0.f, 0.f, 1.f },
	Vertex{o.x + 1.0f,o.y + 0.5f,o.z + 0.0f, 0.f, 0.f, 1.f },
	Vertex{o.x + -1.0f,o.y + 0.5f,o.z + 0.0f, 0.f, 1.f, 0.f },
	Vertex{o.x + -1.0f,o.y + -0.5f,o.z + 0.0f, 1.f, 0.f, 0.f }
};

const std::vector<Vertex> bboxVertices =
{   // pos(x,y,z)   , col(r,g,b)
	Vertex{o.x + -1.0f,o.y + -0.7f,o.z + -0.3f, 0.1f, 0.f, 0.8f },
	Vertex{o.x + 1.0f,o.y + -0.7f,o.z + -0.3f, 0.1f, 0.f, 0.8f },
	Vertex{o.x + 1.0f,o.y + 0.7f,o.z + -0.3f, 0.1f, 0.f, 0.8f },
	Vertex{o.x + -1.0f,o.y + 0.7f,o.z + -0.3f, 0.1f, 0.f, 0.8f },
	Vertex{o.x + -1.0f,o.y + -0.7f,o.z + 0.3f, 0.1f, 0.f, 0.8f },
	Vertex{o.x + 1.0f,o.y + -0.7f,o.z + 0.3f, 0.1f, 0.f, 0.8f },
	Vertex{o.x + 1.0f,o.y + 0.7f,o.z + 0.3f, 0.1f, 0.f, 0.8f },
	Vertex{o.x + -1.0f,o.y + 0.7f,o.z + 0.3f, 0.1f, 0.f, 0.8f },
};
const std::vector<unsigned int> bboxElements =
{
	0, 1, 2, 3, 0, // bottom
	4, 5, 6, 7, 4, // top
	5, 1, 2, 6, 7, 3 // connect them
};

static const char* vertex_shader_source = R"(
	uniform mat4 MVP;
	attribute vec3 vCol;
	attribute vec3 vPos;
	varying vec3 color;
	void main()
	{
		gl_Position = MVP * vec4(vPos, 1.0);
		color = vCol;
	}\
)";

static const char* fragment_shader_source = R"(
	varying vec3 color;
	in vec4 gl_FragCoord;
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
				std::cout << "OpenGL Error: " << message << std::endl;
}

enum class TextureSenderMode {
	All = 0,
	Single,
	Stereo,
};
struct TextureSenders {
	TextureSenderMode mode;
	mint::StereoTextureSender stereotextureSender;
	mint::TextureSender lefttextureSender;
	mint::TextureSender righttextureSender;

	explicit TextureSenders(TextureSenderMode m) : mode{ m } {
		switch (mode)
		{
		case TextureSenderMode::All:
			stereotextureSender.init();
			lefttextureSender.init(mint::ImageType::LeftEye);
			righttextureSender.init(mint::ImageType::RightEye);
			break;
		case TextureSenderMode::Single:
			lefttextureSender.init(mint::ImageType::LeftEye);
			righttextureSender.init(mint::ImageType::RightEye);
			break;
		case TextureSenderMode::Stereo:
			stereotextureSender.init();
			break;
		default:
			break;
		}
	}

	void destroy() {
		switch (mode)
		{
		case TextureSenderMode::All:
			stereotextureSender.destroy();
			lefttextureSender.destroy();
			righttextureSender.destroy();
			break;
		case TextureSenderMode::Single:
			lefttextureSender.destroy();
			righttextureSender.destroy();
			break;
		case TextureSenderMode::Stereo:
			stereotextureSender.destroy();
			break;
		default:
			break;
		}
	}
};

int main(int argc, char** argv)
{
	CLI::App app("mint rendering");
	app.set_config("--config", "mint.config", "Config file containing CLI options in TOML format", false);
	app.allow_config_extras(true);

	mint::DataProtocol zmq_protocol = mint::DataProtocol::TCP;
	std::map<std::string, mint::DataProtocol> map_zmq = { {"ipc", mint::DataProtocol::IPC}, {"tcp", mint::DataProtocol::TCP} };
	app.add_option("--zmq", zmq_protocol, "ZeroMQ protocol to use for data channels. Options: ipc, tcp")
		->transform(CLI::CheckedTransformer(map_zmq, CLI::ignore_case));

	mint::ImageProtocol spout_protocol = mint::ImageProtocol::GPU;
	std::map<std::string, mint::ImageProtocol> map_spout = { {"gpu", mint::ImageProtocol::GPU}, {"cpu", mint::ImageProtocol::CPU}, {"memshare", mint::ImageProtocol::MemShare} };
	app.add_option("--spout", spout_protocol, "Spout protocol to use for texture sharing.")
		->transform(CLI::CheckedTransformer(map_spout, CLI::ignore_case));

	float rendering_fps_target_ms = 0.0;
	auto* fps_opt_opt = app.add_option("-r,--render-ms", rendering_fps_target_ms, "Frame time in miliseconds to target via render loop delay");

	std::vector<int> image_size = { 800, 600 };
	auto* image_size_used = app.add_option("-i,--image-size", image_size, "Size of shared image: -i width height")->expected(2);

	bool vsync = false;
	app.add_flag("--vsync", vsync, "Whether to activate vsync for this process");

	TextureSenderMode texture_sender_mode = TextureSenderMode::All;
	std::map<std::string, TextureSenderMode> map_txtrsend = { {"all", TextureSenderMode::All}, {"single", TextureSenderMode::Single}, {"stereo", TextureSenderMode::Stereo} };
	app.add_option("--texture-send", texture_sender_mode, "Whether to send single left+right textures, stereo texture, or all")
		->transform(CLI::CheckedTransformer(map_txtrsend, CLI::ignore_case));

	auto txtr_to_string = [](TextureSenderMode mode) -> std::string {
		switch (mode)
		{
		case TextureSenderMode::All:
			return "single + stereo";
			break;
		case TextureSenderMode::Single:
			return "single textures";
			break;
		case TextureSenderMode::Stereo:
			return "stereo texture";
			break;
		default:
			break;
		}
		};

	CLI11_PARSE(app, argc, argv);

	std::cout << "rendering with fps target: " << rendering_fps_target_ms << " ms" << std::endl;
	std::cout << "rendering sends texture: " << txtr_to_string(texture_sender_mode) << std::endl;

	GLFWwindow* window;
	GLuint vertex_shader, fragment_shader, program;
	GLint mvp_location, vpos_location, vcol_location;
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		exit(EXIT_FAILURE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	int initialWidth = 640;
	int initialHeight = 480;
	std::string window_name = "mint rendering";
	window = glfwCreateWindow(initialWidth, initialHeight, window_name.c_str(), NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	glfwSetKeyCallback(window, glfw_key_callback);
	glfwMakeContextCurrent(window);
	glfwSwapInterval(vsync ? 1 : 0);
	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(opengl_debug_message_callback, nullptr);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glClearDepth(1.0f);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glDisable(GL_CULL_FACE);

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

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	mvp_location = glGetUniformLocation(program, "MVP");
	vpos_location = glGetAttribLocation(program, "vPos");
	vcol_location = glGetAttribLocation(program, "vCol");

	const auto registerVertexAttributes = [&]() {
		glEnableVertexAttribArray(vpos_location);
		glVertexAttribPointer(vpos_location, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		glEnableVertexAttribArray(vcol_location);
		glVertexAttribPointer(vcol_location, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(float) * 3));
		};

	RenderVertices quad;
	quad.init(quadVertices);
	quad.bind();
	registerVertexAttributes();
	quad.unbind();

	RenderVertices bbox;
	bbox.init(bboxVertices);
	bbox.bind();
	registerVertexAttributes();
	bbox.unbind();
	bbox.setElements(bboxElements);

	mint::init(mint::Role::Rendering, zmq_protocol, spout_protocol);

	mint::glFramebuffer fbo_left;
	fbo_left.init();
	mint::glFramebuffer fbo_right;
	fbo_right.init();

	mint::DataReceiver data_receiver;
	data_receiver.start();

	//mint::DataReceiver cameraProjectionReceiver;
	//cameraProjectionReceiver.start("CameraProjection");
	auto cameraProjection = mint::CameraProjection();

	//mint::DataReceiver stereoCameraViewReceiver_relative;
	//stereoCameraViewReceiver_relative.start("StereoCameraViewRelative");
	auto stereoCameraView = mint::StereoCameraViewRelative();

	mint::DataSender data_sender;
	data_sender.start();

	auto bboxCorners = mint::BoundingBoxCorners{
		mint::vec4{0.0f, 0.0f, 0.0f , 1.0f},
		mint::vec4{2.0f * offset.x, 2.0f * offset.y, 2.0f * offset.z, 1.0f}
	};

	TextureSenders textureSender{ texture_sender_mode };

	mint::CameraView defaultCameraView;
	defaultCameraView.eyePos = toInterop(glm::vec3{ 0.0f, 0.0f, 3.0f });
	defaultCameraView.lookAtPos = toInterop(glm::vec3{ 0.0f });
	defaultCameraView.camUpDir = toInterop(glm::vec3{ 0.0f, 1.0f, 0.0f });
	auto view = glm::lookAt(glm::vec3(toGlm(defaultCameraView.eyePos)), glm::vec3(toGlm(defaultCameraView.lookAtPos)), glm::vec3(toGlm(defaultCameraView.camUpDir)));
	float ratio = initialWidth / (float)initialHeight;
	auto projection = glm::perspective(90.0f, ratio, 0.1f, 10.0f);

	stereoCameraView.leftEyeView = defaultCameraView;
	stereoCameraView.rightEyeView = defaultCameraView;

	int fbo_width = 0, fbo_height = 0;
	glfwGetFramebufferSize(window, &fbo_width, &fbo_height);

	using namespace std::chrono_literals;
	using FpMilliseconds = std::chrono::duration<float, std::chrono::milliseconds::period>;
	auto program_start_time = std::chrono::high_resolution_clock::now();
	auto current_time = std::chrono::high_resolution_clock::now();


	std::string current_time_string = std::to_string(current_time.time_since_epoch().count());
	std::cout << "rendering program_start_time: " << current_time_string << std::endl;

	std::vector<float> frame_durations(60, 0.0f);
	float frame_durations_size = static_cast<float>(frame_durations.size());
	unsigned int frame_timing_index = 0;
	float last_fps_wait = 0.0f;
	float last_frame_ms = 0.0f;

	auto fps_average = [&]() {
		return std::accumulate(frame_durations.begin(), frame_durations.end(), 0.0f) / frame_durations_size;
		};

	while (!glfwWindowShouldClose(window))
	{
		int should_close = 0;
		if (data_receiver.receive(should_close, "mintclose") && should_close) {
			std::cout << "received remote close" << std::endl;
			glfwSetWindowShouldClose(window, true);
		}

		auto last_time = current_time;
		current_time = std::chrono::high_resolution_clock::now();
		auto last_frame_duration = FpMilliseconds(current_time - last_time).count();
		frame_durations[frame_timing_index] = last_frame_duration;
		last_frame_ms = last_frame_duration;
		current_time_string = std::to_string(current_time.time_since_epoch().count());

		if (rendering_fps_target_ms > 0.0f) {
			auto diff = rendering_fps_target_ms - last_frame_duration;
			last_fps_wait += diff * 0.1;
			//std::cout << "diff " << diff << std::endl;
			std::this_thread::sleep_for(std::chrono::duration<float, std::milli>(last_fps_wait));
		}

		if (frame_timing_index == 0) {
			float frame_ms = fps_average();
			std::string fps_info = " | " + std::to_string(frame_ms) + " ms/f | " + std::to_string(1000.0f / frame_ms) + " fps";
			std::string title = window_name + fps_info;
			glfwSetWindowTitle(window, title.c_str());
		}
		frame_timing_index = (frame_timing_index + 1) % frame_durations.size();

		ratio = fbo_width / (float)fbo_height;
		// default framebuffer
		glViewport(0, 0, fbo_width, fbo_height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		data_sender.send(bboxCorners, mint::to_data_name(bboxCorners), std::pair{ std::string("timestamp"), current_time_string });

		const auto getModelMatrix = [](mint::ModelPose& mp) -> glm::mat4 {
			const glm::vec4 modelTranslate = toGlm(mp.translation);
			const glm::vec4 modelScale = toGlm(mp.scale);
			const glm::vec4 modelRotation = toGlm(mp.rotation_axis_angle_rad);

			return
				glm::translate(glm::vec3{ modelTranslate })
				* glm::rotate(modelRotation.w, glm::vec3{ modelRotation })
				* glm::scale(glm::vec3{ modelScale });
			};

		bool received_data = false;

		bool hasNewWindowSize = false;
		if (received_data |= data_receiver.receive<mint::CameraProjection>(cameraProjection)) {
			projection = glm::perspective(
				cameraProjection.fieldOfViewY_rad,
				cameraProjection.aspect,
				cameraProjection.nearClipPlane,
				cameraProjection.farClipPlane);

			hasNewWindowSize =
				(fbo_width != cameraProjection.pixelWidth)
				|| (fbo_height != cameraProjection.pixelHeight);
			if (hasNewWindowSize && image_size_used->count() == 0) {
				fbo_width = cameraProjection.pixelWidth;
				fbo_height = cameraProjection.pixelHeight;
				glfwSetWindowSize(window, fbo_width, fbo_height);
				std::cout << "texture size from projection: " << fbo_width << "x" << fbo_height << std::endl;
			}
		}

		received_data |= data_receiver.receive<mint::StereoCameraViewRelative>(stereoCameraView);

		const auto render = [&](mint::CameraView& camView, mint::glFramebuffer& fbo, mint::TextureSender* ts)
			{
				if (hasNewWindowSize || fbo_width != fbo.m_width || fbo_height != fbo.m_height) {
					fbo.resizeTexture(fbo_width, fbo_height);
				}

				// render into custom framebuffer
				fbo.bind();
				glViewport(0, 0, fbo_width, fbo_height);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				//const auto cameraModel = getModelMatrix(cameraPose);
				view = glm::lookAt(
					glm::vec3(toGlm(camView.eyePos)),
					glm::vec3(toGlm(camView.lookAtPos)),
					glm::vec3(toGlm(camView.camUpDir)));

				const auto mvp = projection * view;

				glUseProgram(program);
				glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*)glm::value_ptr(mvp));

				quad.bind();
				glDrawArrays(GL_TRIANGLES, 0, 6);
				quad.unbind();

				bbox.bind();
				glDrawElements(GL_LINE_STRIP, bboxElements.size(), GL_UNSIGNED_INT, (void*)0);
				bbox.unbind();

				fbo.unbind();
				if (ts) {
					ts->send(fbo.m_glTextureRGBA8, fbo_width, fbo_height);
					fbo.blitTexture(); // blit custom fbo to default framebuffer
				}
			};

		mint::TextureSender* left_sender_ptr = nullptr;
		mint::TextureSender* right_sender_ptr = nullptr;
		switch (texture_sender_mode)
		{
		case TextureSenderMode::All:
			[[fallthrough]];
		case TextureSenderMode::Single:
			left_sender_ptr = &textureSender.lefttextureSender;
			right_sender_ptr = &textureSender.righttextureSender;
			break;
		case TextureSenderMode::Stereo:
			left_sender_ptr = nullptr;
			right_sender_ptr = nullptr;
			break;
		default:
			break;
		}
		render(stereoCameraView.leftEyeView, fbo_left, left_sender_ptr);
		render(stereoCameraView.rightEyeView, fbo_right, right_sender_ptr);

		// embedd frame id from steering in texture
		mint::uint steering_frame_id = 0;
		steering_frame_id = glm::floatBitsToUint(stereoCameraView.leftEyeView.eyePos.w);
		mint::uint rendering_last_frame_ms = 0;
		rendering_last_frame_ms = glm::floatBitsToUint(last_frame_ms);

		switch (texture_sender_mode)
		{
		case TextureSenderMode::All:
			[[fallthrough]];
		case TextureSenderMode::Stereo:
			textureSender.stereotextureSender.send(fbo_left, fbo_right, fbo_width, fbo_height, steering_frame_id, rendering_last_frame_ms);
			break;
		case TextureSenderMode::Single:
			break;
		default:
			break;
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	data_receiver.stop();
	//cameraProjectionReceiver.stop();
	//stereoCameraViewReceiver_relative.stop();
	data_sender.stop();

	fbo_left.destroy();
	fbo_right.destroy();
	textureSender.destroy();

	quad.destroy();
	bbox.destroy();

	std::cout << "mint rendering exit \naverage frame ms: " << fps_average() << std::endl;

	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}