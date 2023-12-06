
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define GLM_FORCE_CTOR_INIT
#define GLM_FORCE_EXPLICIT_CTOR
#include <glm/glm.hpp> // vec2, vec3, mat4, radians
#include <glm/ext.hpp> // perspective, translate, rotate
#include <glm/gtx/transform.hpp> // rotate in degrees around axis

#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <ctime>
#include <fstream>

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

const char* vertex_shader_source =
R"(
	#version 430 core

	//in int gl_VertexID;

	uniform sampler2D texture_in;

	uniform int latency_measure_active;
	uniform int latency_measure_current_frame_id;
	uniform int latency_measure_current_index;

	layout (std430, binding=2) buffer LatencySsboDataBlock
	{ 
	  uvec4 data[]; // x = render loop frame id, y = incoming texture frame id
	} ssbo_data;

	const vec4 unitQuad[4] =
	vec4[](
		vec4(-1.0f, 1.0f, 0.0f, 1.0f),
		vec4(-1.0f,-1.0f, 0.0f, 1.0f),
		vec4( 1.0f, 1.0f, 0.0f, 1.0f),
		vec4( 1.0f,-1.0f, 0.0f, 1.0f)
	);
	
	void main()
	{
	    gl_Position = unitQuad[gl_VertexID];

		if(gl_VertexID == 0 && latency_measure_active > 0) {
			// reverses: FragColor = unpackUnorm4x8(uint(meta_data))
			vec4 steering_id_raw = texelFetch(texture_in, ivec2(0,0), 0);
			vec4 renderer_fps_raw = texelFetch(texture_in, ivec2(1,0), 0);
			uint texture_frame_id = packUnorm4x8(steering_id_raw);
			uint texture_renderer_fps = packUnorm4x8(renderer_fps_raw);
			ssbo_data.data[latency_measure_current_index] = uvec4(
				latency_measure_current_frame_id,
				texture_frame_id,
				texture_renderer_fps,
				0);
		}
	}
)";

const char* fragment_shader_source =
R"(
	#version 430 core

	in vec4 gl_FragCoord;

	out vec4 FragColor;

	uniform sampler2D texture_in;
	uniform ivec2 texture_size;

	void main()
	{
		FragColor = texture(texture_in, vec2(gl_FragCoord.xy) / vec2(texture_size.xy));
	}
)";

int main(int argc, char** argv)
{
	CLI::App app("mint steering");
	app.set_config("--config", "mint.config", "Config file containing CLI options in TOML format", false);
	app.allow_config_extras(true);

	mint::DataProtocol zmq_protocol = mint::DataProtocol::TCP;
	std::map<std::string, mint::DataProtocol> map_zmq = { {"ipc", mint::DataProtocol::IPC}, {"tcp", mint::DataProtocol::TCP} };
	app.add_option("--zmq", zmq_protocol, "ZeroMQ protocol to use for data channels. Options: ipc, tcp")
		->transform(CLI::CheckedTransformer(map_zmq, CLI::ignore_case));

	mint::ImageProtocol spout_protocol = mint::ImageProtocol::GPU;
	std::map<std::string, mint::ImageProtocol> map_spout = { {"gpu", mint::ImageProtocol::GPU}, {"cpu", mint::ImageProtocol::CPU}, {"memshare", mint::ImageProtocol::MemShare} };
	app.add_option("--spout", spout_protocol, "Spout protocol to use for texture sharing. Options: ram (shared memory), vram")
		->transform(CLI::CheckedTransformer(map_spout, CLI::ignore_case));

	std::filesystem::path latency_measure_output_file = "";
	app.add_option("-f,--latency-file", latency_measure_output_file, "Output file for latency measurements");

	float latency_measure_duration_sec = 0.0f;
	app.add_option("--latency-measure", latency_measure_duration_sec, "How long to measure frame latency, in seconds");

	float latency_measure_delay_until_start_sec = 0.0f;
	app.add_option("--latency-startup", latency_measure_delay_until_start_sec, "If positive, delay in seconds after startup until latency measurements begin");

	std::vector<int> image_size = { 800, 600 };
	auto* image_size_used = app.add_option("-i,--image-size", image_size, "Size of shared image: -i width height")->expected(2);

	bool vsync = false;
	app.add_flag("--vsync", vsync, "Whether to activate vsync for this process");

	CLI11_PARSE(app, argc, argv);
	// cli data available only after parsing!
	auto latency_measure_duration_ms = latency_measure_duration_sec * 1000.0f;
	auto latency_measure_delay_until_start_ms = latency_measure_delay_until_start_sec * 1000.0f;
	if (image_size_used->count() > 0)
		std::cout << "Texture Size: " << image_size[0] << "x" << image_size[1] << std::endl;

	GLFWwindow* window;
	GLuint vertex_shader, fragment_shader, program;
	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit())
		exit(EXIT_FAILURE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	int initialWidth = 640;
	int initialHeight = 480;
	std::string window_name = "mint steering";
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

	// https://www.khronos.org/opengl/wiki/Shader_Compilation
	auto check_shader_compilation = [](auto& name, auto& shader) {
		GLint isCompiled = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &isCompiled);
		if (isCompiled == GL_FALSE)
		{
			GLint maxLength = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &maxLength);

			std::vector<GLchar> errorLog(maxLength);
			glGetShaderInfoLog(shader, maxLength, &maxLength, &errorLog[0]);

			std::cout << "Shader Compilation Error " << name << ": " << std::string{ (char*)errorLog.data() } << std::endl;

			glDeleteShader(shader);

			std::exit(EXIT_FAILURE);
		}
		};

	vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
	glCompileShader(vertex_shader);
	check_shader_compilation("Vertex", vertex_shader);
	fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
	glCompileShader(fragment_shader);
	check_shader_compilation("Fragment", fragment_shader);
	program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	using SsboType = glm::uvec4;
	assert(SsboType{}.x == 0 && SsboType{}.y == 0);

	struct LatencyMeasurements {
		unsigned int frame_id = 0;
		unsigned int texture_frame_id = 0;
		float steering_wall_clock_ms = 0.f;
		float steering_fps_ms = 0.f;
		float rendering_fps_ms = 0.f;
	};

	GLuint block_index = 0;
	block_index = glGetProgramResourceIndex(program, GL_SHADER_STORAGE_BLOCK, "LatencySsboDataBlock");
	GLuint ssbo_binding_point_index = 2;
	glShaderStorageBlockBinding(program, block_index, ssbo_binding_point_index);

	GLuint latency_ssbo = 0;
	glGenBuffers(1, &latency_ssbo);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, ssbo_binding_point_index, latency_ssbo);

	std::vector<LatencyMeasurements> latency_measurements;
	std::vector<SsboType> latency_ssbo_values;
	size_t latency_ssbo_memory_size = 0;
	unsigned int latency_measurements_index = 0;
	glBufferData(GL_SHADER_STORAGE_BUFFER, latency_ssbo_memory_size, latency_ssbo_values.data(), GL_DYNAMIC_READ);

	auto set_latency_measurement_buffers = [&](unsigned int samples) {
		unsigned int latency_sample_count = samples;
		latency_measurements.resize(latency_sample_count, LatencyMeasurements{});
		latency_ssbo_values.resize(latency_sample_count, SsboType{});
		latency_ssbo_memory_size = latency_ssbo_values.size() * sizeof(SsboType);
		std::cout << "SSBO: requesting buffer size " << latency_ssbo_memory_size / 1024 << " KB, " << latency_ssbo_memory_size / 1024 / 1024 << " MB" << std::endl;
		glBufferData(GL_SHADER_STORAGE_BUFFER, latency_ssbo_memory_size, latency_ssbo_values.data(), GL_DYNAMIC_READ);
		};

	auto read_ssbo_data_from_gpu = [&]() {
		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		void* p = glMapNamedBuffer(latency_ssbo, GL_READ_ONLY);
		memcpy(latency_ssbo_values.data(), p, latency_ssbo_memory_size);
		glUnmapNamedBuffer(latency_ssbo);
		};

	GLuint uniform_texture_location = glGetUniformLocation(program, "texture_in"); // sampler2D
	GLuint uniform_texture_size_location = glGetUniformLocation(program, "texture_size"); // ivec2
	GLuint uniform_latency_measure_active = glGetUniformLocation(program, "latency_measure_active"); // int 
	GLuint uniform_latency_measure_frame_id = glGetUniformLocation(program, "latency_measure_current_frame_id"); // int 
	GLuint uniform_latency_measure_index = glGetUniformLocation(program, "latency_measure_current_index"); // int 

	bool latency_measure_active = false;

	std::vector<Vertex> quadVertices;
	RenderVertices quad;
	quad.init(quadVertices);
	quad.bind();
	//registerVertexAttributes();
	quad.unbind();

	mint::init(mint::Role::Steering, zmq_protocol, spout_protocol);

	mint::glFramebuffer fbo;
	fbo.init();

	mint::TextureReceiver texture_receiver;
	texture_receiver.init(mint::ImageType::SingleStereo);

	mint::DataSender data_sender;
	data_sender.start();

	auto cameraProjection = mint::CameraProjection(); // "CameraProjection"
	auto stereoCameraView = mint::StereoCameraViewRelative(); // "StereoCameraViewRelative"

	mint::DataReceiver data_receiver;
	data_receiver.start();

	auto bboxCorners = mint::BoundingBoxCorners{
		mint::vec4{0.0f, 0.0f, 0.0f , 1.0f},
		mint::vec4{1.0f, 1.0f, 1.0f , 1.0f},
	};

	const auto cam_up = glm::vec3{ 0.0f, 1.0f, 0.0f };

	mint::CameraView defaultCameraView;
	defaultCameraView.eyePos = toInterop(glm::vec3{ 0.0f, 0.0f, 3.0f });
	defaultCameraView.lookAtPos = toInterop(glm::vec3{ 0.0f });
	defaultCameraView.camUpDir = toInterop(cam_up);

	auto get_camera_view = [&](mint::BoundingBoxCorners& bbox) -> mint::CameraView {
		auto diagonal = bbox.diagonal();
		auto center = diagonal * 0.5f + bbox.min;

		mint::CameraView view;

		view.eyePos = center + diagonal * 0.5f;
		view.lookAtPos = center;
		view.camUpDir = toInterop(cam_up);

		return view;
		};

	float fovy = 90.f;
	float aspect_ratio = initialWidth / (float)initialHeight;
	float near_p = 0.1f;
	float far_p = 10.0f;

	auto projection = glm::perspective(fovy, aspect_ratio, near_p, far_p);

	int width = image_size[0], height = image_size[1];

	mint::uint frame_id = 0;

	using namespace std::chrono_literals;
	using FpMilliseconds = std::chrono::duration<float, std::chrono::milliseconds::period>;
	auto program_start_time = std::chrono::high_resolution_clock::now();
	auto current_time = std::chrono::high_resolution_clock::now();
	auto latency_measure_started_time = std::chrono::high_resolution_clock::now();

	std::string current_time_string = std::to_string(current_time.time_since_epoch().count());
	std::cout << "steering program_start_time: " << current_time_string << std::endl;

	std::vector<float> frame_durations(60, 0.0f);
	float frame_durations_size = static_cast<float>(frame_durations.size());
	unsigned int frame_timing_index = 0;
	float last_frame_ms = 0.0f;

	auto fps_average = [&]() {
		return std::accumulate(frame_durations.begin(), frame_durations.end(), 0.0f) / frame_durations_size;
		};

	bool has_bbox = false;

	mint::DataSender close_sender("tcp://127.0.0.1:12349");
	close_sender.start(mint::Endpoint::Bind);

	while (!glfwWindowShouldClose(window))
	{
		frame_id++;

		auto last_time = current_time;
		current_time = std::chrono::high_resolution_clock::now();
		auto last_frame_duration = FpMilliseconds(current_time - last_time).count();
		frame_durations[frame_timing_index] = last_frame_duration;
		last_frame_ms = last_frame_duration;
		current_time_string = std::to_string(current_time.time_since_epoch().count());

		if (frame_timing_index == 0) {
			float frame_ms = fps_average();
			std::string fps_info = " | " + std::to_string(frame_ms) + " ms/f | " + std::to_string(1000.0f / frame_ms) + " fps";
			std::string title = window_name + fps_info;
			glfwSetWindowTitle(window, title.c_str());
		}
		frame_timing_index = (frame_timing_index + 1) % frame_durations.size();

		glfwGetFramebufferSize(window, &width, &height);

		aspect_ratio = width / (float)height;
		// default framebuffer
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		cameraProjection.fieldOfViewY_rad = fovy;
		cameraProjection.nearClipPlane = near_p;
		cameraProjection.farClipPlane = far_p;
		cameraProjection.aspect = aspect_ratio;
		cameraProjection.pixelWidth = width;
		cameraProjection.pixelHeight = height;

		mint::BoundingBoxCorners newBbox;
		auto timestamp = std::make_optional(std::pair<std::string, std::string>{"timestamp", "0"});
		if (has_bbox = data_receiver.receive(newBbox, mint::to_data_name(newBbox), timestamp)) {
			if (newBbox.min != bboxCorners.min || newBbox.max != bboxCorners.max) {
				defaultCameraView = get_camera_view(newBbox);
				bboxCorners = newBbox;
				std::cout << "New Bbox: ("
					<< bboxCorners.min.x << ", "
					<< bboxCorners.min.y << ", "
					<< bboxCorners.min.z << "; "
					<< bboxCorners.max.x << ", "
					<< bboxCorners.max.y << ", "
					<< bboxCorners.max.z << ") "
					<< std::endl;

				if (timestamp.has_value()) {
					auto rendering_bbox_send_frame_start_ns = timestamp.value().second;
					auto steering_bbox_receive_frame_start_ns = current_time_string;

					std::cout << "steering bbox receive frame start: " << steering_bbox_receive_frame_start_ns << std::endl;
					std::cout << "rendering bbox send frame start:   " << rendering_bbox_send_frame_start_ns << std::endl;
					std::cout << "bbox send/receive frame start diff: "
						<< static_cast<double>(std::stoull(steering_bbox_receive_frame_start_ns)
							- std::stoull(rendering_bbox_send_frame_start_ns)) / 1000000.0
						<< " ms"
						<< std::endl;
				}
			}
		}

		// https://stackoverflow.com/questions/70617006/how-to-rotate-an-object-around-a-point-with-glm-opengl-c
		auto rotate_around = [&](float rad, const glm::vec3& point, const glm::vec3& axis) -> glm::mat4
			{
				auto t1 = glm::translate(glm::mat4(1), -point);
				auto r = glm::rotate(glm::mat4(1), rad, axis);
				auto t2 = glm::translate(glm::mat4(1), point);
				return t2 * r * t1;
			};

		defaultCameraView.eyePos = toInterop(glm::vec3(rotate_around(glfwGetTime() * 0.001f, glm::vec3(toGlm(defaultCameraView.lookAtPos)), glm::vec3(toGlm(defaultCameraView.camUpDir))) * toGlm(defaultCameraView.eyePos)));

		stereoCameraView.leftEyeView = defaultCameraView;
		stereoCameraView.leftEyeView.eyePos.w = glm::uintBitsToFloat(frame_id);
		stereoCameraView.rightEyeView = defaultCameraView;
		// actually draw different left/right cameras
		stereoCameraView.rightEyeView.eyePos += 0.2 * bboxCorners.diagonal();

		data_sender.send(stereoCameraView);
		data_sender.send(cameraProjection);

		texture_receiver.receive();
		auto texture_handle = texture_receiver.m_texture_handle;

		if (width != fbo.m_width || height != fbo.m_height) {
			fbo.resizeTexture(width, height);
		}

		// render into custom framebuffer
		fbo.bind();
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glUseProgram(program);

		unsigned int binding_point = 0;
		glActiveTexture(GL_TEXTURE0 + binding_point);
		glBindTexture(GL_TEXTURE_2D, texture_handle);
		glUniform1i(uniform_texture_location, binding_point);

		// start latency measurements
		if (latency_measure_delay_until_start_sec > 0.0f && has_bbox && !latency_measure_active && FpMilliseconds(current_time - program_start_time).count() > latency_measure_delay_until_start_ms && latency_measurements_index == 0) {
			std::cout << "start latency measurements" << std::endl;
			latency_measure_active = true;
			float frame_ms = fps_average();
			//we expect to roughly maintain our current fps during the latency measurements
			auto frames_upper_bound = static_cast<unsigned int>(latency_measure_duration_ms / (frame_ms * 0.45f));
			std::cout << "LATENCY: upper frame bound: " << frames_upper_bound << ", measure duration ms: " << latency_measure_duration_ms << ", frame ms: " << frame_ms << std::endl;

			set_latency_measurement_buffers(frames_upper_bound);
			auto latency_measure_started_time = current_time;
		}
		// stop latency measurements
		if (latency_measure_active && (FpMilliseconds(current_time - latency_measure_started_time).count() > latency_measure_duration_ms || latency_measurements_index >= latency_measurements.size())) {
			latency_measure_active = false;

			read_ssbo_data_from_gpu();

			std::string result = "# FrameID,GpuFrameID,FrameTimeDeltaMS,WallClockMS,RenderingFrameDeltaMS\n";

			if (latency_measurements_index > latency_measurements.size()) {
				std::cout << "LATENCY: aborted due to too man entries" << std::endl;
			}
			if (latency_measurements_index < latency_measurements.size()) {
				std::cout << "LATENCY: buffer not filled entirely, difference: " << latency_measurements.size() - latency_measurements_index << std::endl;
				latency_measurements.resize(latency_measurements_index);
				latency_ssbo_values.resize(latency_measurements_index);
			}

			auto cpu_iter = latency_measurements.begin();
			for (auto pair : latency_ssbo_values) {
				auto& cpu = *cpu_iter;

				if (cpu.frame_id != pair.x) {
					std::cout << "LATENCY ID PAIR ERROR: CPU=" << cpu.frame_id << ", GPU=" << pair.x << std::endl;
					std::exit(EXIT_FAILURE);
				}

				cpu.texture_frame_id = pair.y;
				cpu.rendering_fps_ms = glm::uintBitsToFloat(pair.z);
				cpu_iter++;

				result +=
					std::to_string(cpu.frame_id) + ","
					+ std::to_string(cpu.texture_frame_id) + ","
					+ std::to_string(cpu.steering_fps_ms) + ","
					+ std::to_string(cpu.steering_wall_clock_ms) + ","
					+ std::to_string(cpu.rendering_fps_ms) + "\n";
				//std::cout << "FrameId=" << cpu.frame_id << ", TexId=" << cpu.texture_frame_id << ", deltaMS= " << cpu.steering_fps_ms <<std::endl;

			}

			std::cout << "writing measurements into " << latency_measure_output_file.string() << std::endl;
			if (latency_measure_output_file.empty()) {
				std::time_t time = std::time({});
				char timeString[std::size("yyyy-mm-dd-hh-mm")];
				std::strftime(std::data(timeString), std::size(timeString), "%F-%H-%M", std::localtime(&time));
				latency_measure_output_file = "mint_steering_frame_latency_measurements_" + std::string{ timeString } + ".txt";
			}

			std::ofstream file{ latency_measure_output_file };
			if (!file.good()) {
				std::cout << "\n" << result << "\n" << std::endl;
				std::cout << "ERROR: COULD NOT OPEN FILE " << latency_measure_output_file.string() << std::endl;
				std::exit(EXIT_FAILURE);
			}
			file << result;

			int should_close = 1;
			close_sender.send(should_close, "mintclose");
			data_sender.send(should_close, "mintclose");
			std::cout << "sending remote close" << std::endl;
			glfwSetWindowShouldClose(window, true);
		}

		// do latency measurements
		auto latency_ssbo_index = 0;
		if (latency_measure_active && latency_measurements_index < latency_measurements.size()) {
			latency_ssbo_index = latency_measurements_index;
			latency_measurements_index++;
			latency_measurements[latency_ssbo_index].frame_id = frame_id;
			latency_measurements[latency_ssbo_index].texture_frame_id = 42;
			latency_measurements[latency_ssbo_index].steering_fps_ms = last_frame_ms;
			latency_measurements[latency_ssbo_index].steering_wall_clock_ms = FpMilliseconds(current_time - program_start_time).count();
			latency_measurements[latency_ssbo_index].rendering_fps_ms = -1.0f;
		}

		glUniform1i(uniform_latency_measure_active, latency_measure_active ? 1 : 0);
		glUniform1i(uniform_latency_measure_frame_id, static_cast<int>(frame_id));
		glUniform1i(uniform_latency_measure_index, static_cast<int>(latency_ssbo_index));
		glUniform2i(uniform_texture_size_location, width, height);

		//std::this_thread::sleep_for(1s);

		quad.bind();
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		quad.unbind();

		fbo.unbind();
		fbo.blitTexture();

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	data_sender.stop();
	data_receiver.stop();
	close_sender.stop();

	fbo.destroy();
	texture_receiver.destroy();

	quad.destroy();

	std::cout << "mint steering exit \naverage frame ms: " << fps_average() << std::endl;

	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}