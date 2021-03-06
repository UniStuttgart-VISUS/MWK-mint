﻿#include "interop.hpp"

#include <zmq.hpp>
#include "SpoutSender.h"
#include <nlohmann/json.hpp>

#include <iostream>

using json = nlohmann::json;

// we need to load some GL function names by hand if we don't use a loader library
// TODO: cross-link with GL application?
namespace {

#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif

#define MAKE_GL_CALL(FUNCTION_NAME, RETURN_TYPE, ...) \
	typedef RETURN_TYPE (APIENTRYP FUNCTION_NAME ## FuncPtr)(__VA_ARGS__); \
	FUNCTION_NAME ## FuncPtr m ## FUNCTION_NAME;

	//typedef void (APIENTRYP glDrawBuffersEXTFuncPtr)(GLsizei n, const GLenum *bufs);
	//glDrawBuffersEXTFuncPtr my_glDrawBuffersEXT;
	//#define glDrawBuffersEXT my_glDrawBuffersEXT
	typedef char GLchar;
	typedef int GLint;
	#define GL_FRAGMENT_SHADER                0x8B30
	#define GL_VERTEX_SHADER                  0x8B31
	#define GL_ARRAY_BUFFER                   0x8892
	#define GL_LINK_STATUS                    0x8B82
	#define GL_TEXTURE0                       0x84C0
	#define GL_ACTIVE_TEXTURE                 0x84E0
	MAKE_GL_CALL(glDrawBuffersEXT, void, GLsizei n, const GLenum *bufs)
	MAKE_GL_CALL(glCreateShader, GLuint, GLenum shaderType)
	MAKE_GL_CALL(glShaderSource, void, GLuint shader​, GLsizei count​, const GLchar **string​, const GLint *length)
	MAKE_GL_CALL(glCompileShader, void, GLuint shader)
	MAKE_GL_CALL(glCreateProgram, GLuint, void)
	MAKE_GL_CALL(glAttachShader, void, GLuint program​, GLuint shader​)
	MAKE_GL_CALL(glLinkProgram, void, GLuint program)
	MAKE_GL_CALL(glDeleteShader, void, GLuint shader)
	MAKE_GL_CALL(glGetUniformLocation, GLint, GLuint program​, const GLchar *name​)
	MAKE_GL_CALL(glGenVertexArrays, void, GLsizei n​, GLuint *arrays)
	MAKE_GL_CALL(glBindVertexArray, void, GLuint array​)
	//MAKE_GL_CALL(glBindBuffer, void, GLenum target​, GLuint buffer​)
	MAKE_GL_CALL(glDeleteProgram, void, GLuint program)
	MAKE_GL_CALL(glDeleteVertexArrays, void, GLsizei n​, const GLuint *arrays​)
	MAKE_GL_CALL(glGetProgramiv, void, GLuint program, GLenum pname, GLint *params)
	MAKE_GL_CALL(glGetProgramInfoLog, void, GLuint program, GLsizei maxLength, GLsizei *length, GLchar *infoLog)
	MAKE_GL_CALL(glUseProgram, void, GLuint shader)
	MAKE_GL_CALL(glDrawArrays, void, GLenum mode, GLint first, GLsizei count)
	MAKE_GL_CALL(glActiveTexture, void, GLenum texture)
	MAKE_GL_CALL(glBindTexture, void, GLenum target, GLuint texture)
	MAKE_GL_CALL(glUniform1i, void, GLint location, GLint v0)
	MAKE_GL_CALL(glUniform2i, void, GLint location, GLint v0, GLint v1)

	void loadGlExtensions()
	{
		Spout sp;
		sp.OpenSpout(); // loads GL functions

#define GET_GL_CALL(FUNCTION_NAME) \
		const auto FUNCTION_NAME ## fptr = wglGetProcAddress(#FUNCTION_NAME); \
		m ## FUNCTION_NAME = (FUNCTION_NAME ## FuncPtr)FUNCTION_NAME ## fptr;

		//const auto fptr = wglGetProcAddress("glDrawBuffers");
		//glDrawBuffersEXT = (glDrawBuffersEXTFuncPtr)fptr;
		GET_GL_CALL(glDrawBuffersEXT)
		GET_GL_CALL(glCreateShader)
		GET_GL_CALL(glShaderSource)
		GET_GL_CALL(glCompileShader)
		GET_GL_CALL(glCreateProgram)
		GET_GL_CALL(glAttachShader)
		GET_GL_CALL(glLinkProgram)
		GET_GL_CALL(glDeleteShader)
		GET_GL_CALL(glGetUniformLocation)
		GET_GL_CALL(glGenVertexArrays)
		GET_GL_CALL(glBindVertexArray)
		//GET_GL_CALL(glBindBuffer)
		GET_GL_CALL(glDeleteProgram)
		GET_GL_CALL(glDeleteVertexArrays)
		GET_GL_CALL(glGetProgramiv)
		GET_GL_CALL(glGetProgramInfoLog)
		GET_GL_CALL(glUseProgram)
		GET_GL_CALL(glDrawArrays)
		GET_GL_CALL(glActiveTexture)
		GET_GL_CALL(glBindTexture)
		GET_GL_CALL(glUniform1i)
		GET_GL_CALL(glUniform2i)
	}

	static zmq::context_t g_zmqContext{1};
}

static const void savePreviousFbo(interop::glFramebuffer* fbo)
{
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING_EXT, &fbo->m_previousFramebuffer[0]);
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING_EXT, &fbo->m_previousFramebuffer[1]);
	glGetIntegerv(GL_VIEWPORT, fbo->m_previousViewport);
};
static const void restorePreviousFbo(interop::glFramebuffer* fbo)
{
	glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, fbo->m_previousFramebuffer[0]);
	glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, fbo->m_previousFramebuffer[1]);
	glViewport(fbo->m_previousViewport[0], fbo->m_previousViewport[1], fbo->m_previousViewport[2], fbo->m_previousViewport[3]);

	fbo->m_previousFramebuffer[0] = 0;
	fbo->m_previousFramebuffer[1] = 0;
	for(int i = 0; i < 4; i++)
		fbo->m_previousViewport[i] = 0;
}

// GL functions are presented by Spout!
void interop::glFramebuffer::init(uint width, uint height) {

	// im so sorry
	if (!mglDrawBuffersEXT)
		loadGlExtensions();

	m_width = width;
	m_height = height;

	if (!m_glFbo)
		glGenFramebuffersEXT(1, &m_glFbo);

	if(!m_glTextureRGBA8)
		glGenTextures(1, &m_glTextureRGBA8);
	glBindTexture(GL_TEXTURE_2D, m_glTextureRGBA8);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if(!m_glTextureDepth)
		glGenTextures(1, &m_glTextureDepth);
	glBindTexture(GL_TEXTURE_2D, m_glTextureDepth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, m_width, m_height, 0, GL_DEPTH_COMPONENT, GL_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);

	savePreviousFbo(this);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_glFbo);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, m_glTextureRGBA8, 0);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, m_glTextureDepth, 0);

	GLenum e = glCheckFramebufferStatusEXT(GL_DRAW_FRAMEBUFFER_EXT);
	if (e != GL_FRAMEBUFFER_COMPLETE_EXT)
		printf("There is a problem with the FBO\n");

	restorePreviousFbo(this);
}

void interop::glFramebuffer::destroy() {
	if(m_glFbo)
		glDeleteFramebuffersEXT(1, &m_glFbo);

	if(m_glTextureRGBA8)
		glDeleteTextures(1, &m_glTextureRGBA8);

	if(m_glTextureDepth)
		glDeleteTextures(1, &m_glTextureDepth);

	m_glFbo = 0;
	m_glTextureRGBA8 = 0;
	m_glTextureDepth = 0;
	m_width = 0;
	m_height = 0;
}

void interop::glFramebuffer::resizeTexture(uint width, uint height) { 
	if (width == 0 || height == 0)
		return;

	if (m_width == width && m_height == height)
		return;

	this->init(width, height);
}

void interop::glFramebuffer::bind() {
	savePreviousFbo(this);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_glFbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	glViewport(0, 0, m_width, m_height);
}

void interop::glFramebuffer::unbind() {
	restorePreviousFbo(this);
}

void interop::glFramebuffer::blitTexture() {
	savePreviousFbo(this);

	glBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, m_previousFramebuffer[0]);
	glBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, m_glFbo);

	glBlitFramebufferEXT(
		0, 0,
		m_width, m_height,
		m_previousViewport[0],
		m_previousViewport[1],
		m_previousViewport[2],
		m_previousViewport[3],
		GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT,
		GL_NEAREST);

	restorePreviousFbo(this);
}


#define m_spout (static_cast<SpoutSender*>(m_sender.get()))
#define error { printf("there was an error"); return; }
interop::TextureSender::TextureSender() {
	m_sender = std::make_shared<SpoutSender>();
}

interop::TextureSender::~TextureSender() {
	m_sender = nullptr;
}

void interop::TextureSender::init(std::string name, uint width, uint height) {
	if (name.size() == 0 ||name.size() > 256)
		return;

	if (width == 0 || height == 0)
		return;

	if (!m_sender)
		return;

	m_name = name;
	m_name.resize(256, '\0'); // Spout doc says sender name MUST have 256 bytes

	m_width = width;
	m_height = height;

	m_spout->SetCPUmode(false);
	m_spout->SetMemoryShareMode(false);
	m_spout->SetDX9(false);
	m_spout->CreateSender(m_name.c_str(), m_width, m_height);
}

void interop::TextureSender::destroy() {
	if (!m_sender)
		return;

	m_spout->ReleaseSender();
	m_width = 0;
	m_height = 0;
	m_name = "";
}

bool interop::TextureSender::resizeTexture(uint width, uint height) {
	if (!m_sender)
		return false;

	if (width == 0 || height == 0)
		return false;

	if (m_width != width || m_height != height)
	{
		m_width = width;
		m_height = height;

		m_spout->UpdateSender(m_name.c_str(), m_width, m_height);

		return true;
	}
}

void interop::TextureSender::sendTexture(uint textureHandle, uint width, uint height) {
	if (textureHandle == 0)
		return;

	if (!resizeTexture(width, height))
		return;

	m_spout->SendTexture(textureHandle, GL_TEXTURE_2D, m_width, m_height);
}
#undef m_spout

void interop::TextureSender::sendTexture(glFramebuffer& fb) {
	this->sendTexture(fb.m_glTextureRGBA8, fb.m_width, fb.m_height);
}


interop::TexturePackageSender::TexturePackageSender() {}
interop::TexturePackageSender::~TexturePackageSender() {}

void interop::TexturePackageSender::init(std::string name, uint width, uint height) {
	m_name = name + "_hugetexture";
	m_width = width;
	m_height = height;

	m_hugeFbo.init();
	m_hugeTextureSender.init(m_name);
	this->initGLresources();
	
	this->makeHugeTexture(m_width, m_height);
}

void interop::TexturePackageSender::destroy() {
	m_hugeFbo.destroy();
	m_hugeTextureSender.destroy();
	this->destroyGLresources();
}

void interop::TexturePackageSender::makeHugeTexture(const uint originalWidth, const uint originalHeight) {
	m_hugeWidth = originalWidth * 2;
	m_hugeHeight = originalHeight * 2;

	m_hugeFbo.resizeTexture(m_hugeWidth, m_hugeHeight);
	m_hugeTextureSender.resizeTexture(m_hugeWidth, m_hugeHeight); // resizes sender texture
}

void interop::TexturePackageSender::sendTexturePackage(glFramebuffer& fb_left, glFramebuffer& fb_right, const uint width, const uint height) {
	this->sendTexturePackage(fb_left.m_glTextureRGBA8, fb_right.m_glTextureRGBA8, fb_left.m_glTextureDepth, fb_right.m_glTextureDepth, width, height);
}

void interop::TexturePackageSender::sendTexturePackage(const uint color_left, const uint color_right, const uint depth_left, const uint depth_right, const uint width, const uint height) {
	if (m_width != width || m_height != height)
		this->makeHugeTexture(width, height);

	this->blitTextures(color_left, color_right, depth_left, depth_right, width, height);

	m_hugeTextureSender.sendTexture(m_hugeFbo);
}

void interop::TexturePackageSender::initGLresources() {
	const char* vertex_shader_source =
R"(
	#version 400

	in int gl_VertexID;

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
	}
)";

	const char* fragment_shader_source =
R"(
	#version 400

	in vec4 gl_FragCoord;

	uniform sampler2D left_color;
	uniform sampler2D right_color;
	uniform sampler2D left_depth;
	uniform sampler2D right_depth;
	uniform ivec2 texture_size;

	out vec4 FragColor;

	void main()
	{
		vec4 result = vec4(0.0f);
		ivec2 screen_coords = ivec2(gl_FragCoord.xy);

		bool side = bool(screen_coords.x < texture_size.x); // true => left side, false => right side
		bool color_or_depth = bool(screen_coords.y < texture_size.y); // true => color, false => depth
		ivec2 lookup_coord = ivec2(mod(gl_FragCoord.xy, texture_size.xy));

		if(color_or_depth) { // color
			result = (side) 
                ? texelFetch(left_color, lookup_coord, 0).rgba
                : texelFetch(right_color, lookup_coord, 0).rgba;
		} else { // depth
			float depth = (side) 
                ? texelFetch(left_depth, lookup_coord, 0).r
                : texelFetch(right_depth, lookup_coord, 0).r;

			uint deph_uint = floatBitsToUint(depth); // version 330
			vec4 depth_rgba8 = unpackUnorm4x8(deph_uint); // version 400

			result = depth_rgba8;
		}

		FragColor = result;
		//FragColor = vec4(mod(gl_FragCoord.xy, texture_size.xy)/(texture_size.xy), 0.0f, 1.0f);
	}
)";

	const uint vertex_shader = mglCreateShader(GL_VERTEX_SHADER);
	mglShaderSource(vertex_shader, 1, &vertex_shader_source, NULL);
	mglCompileShader(vertex_shader);
	const uint fragment_shader = mglCreateShader(GL_FRAGMENT_SHADER);
	mglShaderSource(fragment_shader, 1, &fragment_shader_source, NULL);
	mglCompileShader(fragment_shader);

	m_shader = mglCreateProgram();
	mglAttachShader(m_shader, vertex_shader);
	mglAttachShader(m_shader, fragment_shader);
	mglLinkProgram(m_shader);

	int success = 0;
	mglGetProgramiv(m_shader, GL_LINK_STATUS, &success);
	if (!success) {
		std::string infoLog;
		infoLog.resize(512);
		mglGetProgramInfoLog(m_shader, 512, NULL, const_cast<char*>(infoLog.c_str()));
		std::cout << infoLog << std::endl;
	}

	mglDeleteShader(vertex_shader);
	mglDeleteShader(fragment_shader);

	m_uniform_locations[0] = mglGetUniformLocation(m_shader, "texture_size");
	m_uniform_locations[1] = mglGetUniformLocation(m_shader, "left_color");
	m_uniform_locations[2] = mglGetUniformLocation(m_shader, "left_depth");
	m_uniform_locations[3] = mglGetUniformLocation(m_shader, "right_color");
	m_uniform_locations[4] = mglGetUniformLocation(m_shader, "right_depth");

	mglGenVertexArrays(1, &m_vao);
}

void interop::TexturePackageSender::destroyGLresources() {
	mglDeleteProgram(m_shader);
	mglDeleteVertexArrays(1, &m_vao);
}

void interop::TexturePackageSender::blitTextures(const uint color_left_texture, const uint color_right_texture, const uint depth_left_texture, const uint depth_right_texture, const uint width, const uint height) {
	GLint previous_active_texture;
	glGetIntegerv(GL_ACTIVE_TEXTURE, &previous_active_texture);

	m_hugeFbo.bind(); // also sets viewport
	mglUseProgram(m_shader);
	mglBindVertexArray(m_vao);
	// TODO: set + restore buffer reset values
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	const auto bindTexture = [&](const uint texture_uniform_location, const uint texture_handle, const uint binding_point) {
		mglActiveTexture(GL_TEXTURE0 + binding_point); // activate the texture unit first before binding texture
		mglBindTexture(GL_TEXTURE_2D, texture_handle);
		mglUniform1i(texture_uniform_location, binding_point); // set it manually
	};

	mglUniform2i(m_uniform_locations[0], static_cast<int>(width), static_cast<int>(height));
	bindTexture(m_uniform_locations[1], color_left_texture, 0);
	bindTexture(m_uniform_locations[2], depth_left_texture, 1);
	bindTexture(m_uniform_locations[3], color_right_texture, 2);
	bindTexture(m_uniform_locations[4], depth_right_texture, 3);

	mglDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	mglBindVertexArray(0);
	m_hugeFbo.unbind();
	mglActiveTexture(previous_active_texture); // activate the texture unit first before binding texture
}

#define m_socket (static_cast<zmq::socket_t*>(m_sender.get()))
interop::DataSender::DataSender() {
}
interop::DataSender::~DataSender() {
}

void interop::DataSender::start(const std::string& networkAddress, std::string const& filterName) {
	m_filterName = filterName;

	m_sender = std::make_shared<zmq::socket_t>(g_zmqContext, zmq::socket_type::pub);
	auto& socket = *m_socket;

	//if (socket.connected())
	//	return;

	try {
		//std::string identity{"InteropLib"};
		//socket.setsockopt(ZMQ_IDENTITY, identity.data(), identity.size());
		socket.bind(networkAddress);
	}
	catch (std::exception& e){
		std::cout << "InteropLib: binding zmq socket failed: " << e.what() << std::endl;
		return;
	}
}
void interop::DataSender::stop() {
	if (m_socket->connected())
		m_socket->close();
}

bool interop::DataSender::sendData(std::string const& v) {
	return this->sendData(m_filterName, v);
}
bool interop::DataSender::sendData(std::string const& filterName, std::string const& v) {
	auto& socket = *m_socket;

	zmq::message_t address_msg{filterName.data(), filterName.size()};
	zmq::message_t data_msg{v.data(), v.size()};
	
	//std::cout << "ZMQ Sender: " << filterName << " / " << v << std::endl;
	bool sendingResult = false;
	const auto print = [](bool b) -> std::string { return (b ? ("true") : ("false"));  };
	try {
		bool adr = socket.send(address_msg, ZMQ_SNDMORE);
		bool msg = socket.send(data_msg);

		//std::cout << "ZMQ Sender: adr=" << print(adr) << ", msg=" << print(msg) << ", " << v << std::endl;
		sendingResult = adr && msg;
	}
	catch (std::exception& e){
		std::cout << "InteropLib: ZMQ sending failed: " << e.what() << std::endl;
	}
	return sendingResult;
}

//template <typename DataType>
//void interop::DataSender::sendData(std::string const& filterName, DataType const& v);
#undef m_socket

namespace {
	void runThread_DataReceiver(interop::DataReceiver* dr, const std::string& networkAddress, const std::string& filterName) {
		if (dr == nullptr)
			return;

		auto& mutex = dr->m_mutex;
		auto& threadRunning = dr->m_threadRunning;
		auto& newDataFlag = dr->m_newDataFlag;
		newDataFlag.test_and_set(); // sets true
		std::string& msgData = dr->m_msgData;
		std::string filter = filterName;

		zmq::socket_t socket{g_zmqContext, ZMQ_SUB};

		try {
			//std::string identity{"InteropLib"};
			//socket.setsockopt(ZMQ_IDENTITY, identity.data(), identity.size());
			//socket.setsockopt(ZMQ_CONFLATE, 1); // keep only most recent message, drop old ones
			socket.setsockopt(ZMQ_SUBSCRIBE, filter.data(), filter.size()); // only receive messages with prefix given by filterName
			socket.setsockopt(ZMQ_RCVTIMEO, 100 /*ms*/); // timeout after not receiving messages for tenth of a second
			socket.connect(networkAddress);
		}
		catch (std::exception& e){
			std::cout << "InteropLib: connecting zmq socket failed: " << e.what() << std::endl;
			return;
		}
		threadRunning.store(true);

		zmq::message_t address_msg;
		zmq::message_t content_msg;

		while (threadRunning) {

			int more = 0;
			int filterEmpty = filter.empty();

			if (!filterEmpty)
			{
				if (!socket.recv(&address_msg))
					continue;
				else
					more = socket.getsockopt<int>(ZMQ_RCVMORE);
			}

			if (filterEmpty || more)
			{
				if (!socket.recv(&content_msg))
					continue;
			}
			else
				continue;

			const bool log = false;
			if (log)
			{
				std::cout << "zmq received address: " << std::string(static_cast<char*>(address_msg.data()), address_msg.size()) << std::endl;
				std::cout << "zmq received content: " << std::string(static_cast<char*>(content_msg.data()), content_msg.size()) << std::endl;
			}

			// store message content for user to retrieve+parse
			std::lock_guard<std::mutex> lock{mutex};
			msgData.assign(static_cast<char*>(content_msg.data()), content_msg.size());
			newDataFlag.clear(std::memory_order_release); // Sets false. All writes in the current thread are visible in other threads that acquire the same atomic variable
		}

		socket.close();
	}
}

interop::DataReceiver::~DataReceiver() {
	stop();
}

void interop::DataReceiver::start(const std::string& networkAddress, const std::string& filterName) {
	if (m_threadRunning)
		return;

	m_thread = std::thread(runThread_DataReceiver, this, networkAddress, filterName);
}

void interop::DataReceiver::stop() {
	if (m_threadRunning) {
		m_threadRunning.store(false);
		m_thread.join();
	}
}

bool interop::DataReceiver::getDataCopy() {
	if (! m_newDataFlag.test_and_set(std::memory_order_acquire)) { // Sets true, returns previous value. All writes in other threads that release the same atomic variable are visible in the current thread.
		std::lock_guard<std::mutex> lock{m_mutex};
		m_msgDataCopy.assign(m_msgData);
		return true;
	}

	return false;
}

//template <typename Datatype>
//Datatype interop::DataReceiver::getData() {
//	this->getDataCopy();
//	auto& byteData = m_msgDataCopy;
//}

// -------------------------------------------------
// --- JSON converters for our interop data types
// -------------------------------------------------

// Add json converter functions as follows: 
//
// void to_json(json& j, const YourOwnType& v) {
// 	j = json{
// 		writeVal(member1),
// 		...
// 		writeVal(memberN)
// 	};
// }
// void from_json(const json& j, YourOwnType& v) {
// 	readVal(member1);
// 	...
// 	readVal(memberN);
// }
namespace interop {

#define writeVal(name) {# name, v. ## name}
#define readVal(name) j.at(# name).get_to(v. ## name);
#define readObj(name) from_json(j.at(# name), v.## name)
	void to_json(json& j, const vec4& v) {
		j = json{
			writeVal(x),
			writeVal(y),
			writeVal(z),
			writeVal(w)
		};
	}
	void from_json(const json& j, vec4& v) {
		readVal(x);
		readVal(y);
		readVal(z);
		readVal(w);
	}

	// mat4 is somewhat special regarding the json format coming from Unity
	void to_json(json& j, const mat4& v) {
		j = json{
			{"e00" , v.data[0].x}, {"e01" , v.data[1].x}, {"e02" , v.data[2].x}, {"e03" , v.data[3].x},
			{"e10" , v.data[0].y}, {"e11" , v.data[1].y}, {"e12" , v.data[2].y}, {"e13" , v.data[3].y},
			{"e20" , v.data[0].z}, {"e21" , v.data[1].z}, {"e22" , v.data[2].z}, {"e23" , v.data[3].z}, 
			{"e30" , v.data[0].w}, {"e31" , v.data[1].w}, {"e32" , v.data[2].w}, {"e33" , v.data[3].w}
		};
	}
#define readMat(name,value) j.at(name).get_to(## value);
	void from_json(const json& j, mat4& v) {
		readMat("e00", v.data[0].x); readMat("e01", v.data[1].x); readMat("e02", v.data[2].x); readMat("e03", v.data[3].x);
		readMat("e10", v.data[0].y); readMat("e11", v.data[1].y); readMat("e12", v.data[2].y); readMat("e13", v.data[3].y);
		readMat("e20", v.data[0].z); readMat("e21", v.data[1].z); readMat("e22", v.data[2].z); readMat("e23", v.data[3].z); 
		readMat("e30", v.data[0].w); readMat("e31", v.data[1].w); readMat("e32", v.data[2].w); readMat("e33", v.data[3].w);
	}

	void to_json(json& j, const CameraView& v) {
		j = json{
			writeVal(eyePos),
			writeVal(lookAtPos),
			writeVal(camUpDir)
		};
	}
	void from_json(const json& j, CameraView& v) {
		readObj(eyePos);
		readObj(lookAtPos);
		readObj(camUpDir);
	}

	void to_json(json& j, const StereoCameraView& v) {
		j = json{
			writeVal(leftEyeView),
			writeVal(rightEyeView)
		};
	}
	void from_json(const json& j, StereoCameraView& v) {
		readObj(leftEyeView);
		readObj(rightEyeView);
	}

	void to_json(json& j, const CameraProjection& v) {
		j = json{
			writeVal(fieldOfViewY_rad),
			writeVal(nearClipPlane),
			writeVal(farClipPlane),
			writeVal(aspect),
			writeVal(pixelWidth),
			writeVal(pixelHeight)
		};
	}
	void from_json(const json& j, CameraProjection& v) {
		readVal(fieldOfViewY_rad);
		readVal(nearClipPlane);
		readVal(farClipPlane);
		readVal(aspect);
		readVal(pixelWidth);
		readVal(pixelHeight);
	}

	void to_json(json& j, const CameraConfiguration& v) {
		j = json{
			writeVal(viewParameters),
			writeVal(projectionParameters),
			writeVal(viewMatrix),
			writeVal(projectionMatrix)
		};
	}
	void from_json(const json& j, CameraConfiguration& v) {
		readObj(viewParameters);
		readObj(projectionParameters);
		readObj(viewMatrix);
		readObj(projectionMatrix);
	}

	void to_json(json& j, const StereoCameraConfiguration& v) {
		j = json{
			writeVal(stereoConvergence),
			writeVal(stereoSeparation),
			writeVal(cameraLeftEye),
			writeVal(cameraRightEye)
		};
	}
	void from_json(const json& j, StereoCameraConfiguration& v) {
		readVal(stereoConvergence);
		readVal(stereoSeparation);
		readObj(cameraLeftEye);
		readObj(cameraRightEye);
	}

	void to_json(json& j, const BoundingBoxCorners& v) {
		j = json{
			writeVal(min),
			writeVal(max)
		};
	}
	void from_json(const json& j, BoundingBoxCorners& v) {
		readObj(min);
		readObj(max);
	}

	void to_json(json& j, const ModelPose& v) {
		j = json{
			writeVal(translation),
			writeVal(scale),
			writeVal(rotation_axis_angle_rad),
			writeVal(modelMatrix)
		};
	}
	void from_json(const json& j, ModelPose& v) {
		readObj(translation);
		readObj(scale);
		readObj(rotation_axis_angle_rad);
		readObj(modelMatrix);
	}

	void to_json(json& j, const DatasetRenderConfiguration& v) {
		j = json{
			writeVal(stereoCamera),
			writeVal(modelTransform)
		};
	}
	void from_json(const json& j, DatasetRenderConfiguration& v) {
		readObj(stereoCamera);
		readObj(modelTransform);
	}
}

#define make_dataGet(DataTypeName) \
template <> \
bool interop::DataReceiver::getData<DataTypeName>(DataTypeName& v) { \
	if(this->getDataCopy()) \
	{ \
		auto& byteData = m_msgDataCopy; \
		if(byteData.size()) { \
			json j = json::parse(byteData); \
			v = j.get<DataTypeName>(); \
			return true; \
		} \
	} \
 \
	return false; \
}

// when adding to those macros, don't forget to also define 'to_json' and 'from_json' functions above for the new data type
make_dataGet(interop::BoundingBoxCorners)
make_dataGet(interop::DatasetRenderConfiguration)
make_dataGet(interop::ModelPose)
make_dataGet(interop::StereoCameraConfiguration)
make_dataGet(interop::CameraConfiguration)
make_dataGet(interop::CameraProjection)
make_dataGet(interop::StereoCameraView)
make_dataGet(interop::CameraView)
make_dataGet(interop::mat4)
make_dataGet(interop::vec4)


#define make_sendData(DataTypeName) \
template <> \
bool interop::DataSender::sendData<DataTypeName>(std::string const& filterName, DataTypeName const& v) { \
	const json j = v; \
	const std::string jsonString = j.dump(); \
	return this->sendData(filterName, jsonString); \
}

// when adding to those macros, don't forget to also define 'to_json' and 'from_json' functions above for the new data type
make_sendData(interop::BoundingBoxCorners)
make_sendData(interop::DatasetRenderConfiguration)
make_sendData(interop::ModelPose)
make_sendData(interop::StereoCameraConfiguration)
make_sendData(interop::CameraConfiguration)
make_sendData(interop::CameraProjection)
make_sendData(interop::StereoCameraView)
make_sendData(interop::CameraView)
make_sendData(interop::mat4)
make_sendData(interop::vec4)
#undef make_sendData

// interop::vec4 arithmetic operators
interop::vec4 interop::operator+(interop::vec4 const& lhs, interop::vec4 const& rhs) {
	return vec4{
		lhs.x + rhs.x,
		lhs.y + rhs.y,
		lhs.z + rhs.z,
		lhs.w + rhs.w,
	};
}
interop::vec4 interop::operator-(interop::vec4 const& lhs, interop::vec4 const& rhs) {
	return vec4{
		lhs.x - rhs.x,
		lhs.y - rhs.y,
		lhs.z - rhs.z,
		lhs.w - rhs.w,
	};
}
interop::vec4 interop::operator*(interop::vec4 const& lhs, interop::vec4 const& rhs) {
	return vec4{
		lhs.x * rhs.x,
		lhs.y * rhs.y,
		lhs.z * rhs.z,
		lhs.w * rhs.w,
	};
}
interop::vec4 interop::operator*(interop::vec4 const& v, const float s) {
	return vec4{
		v.x * s,
		v.y * s,
		v.z * s,
		v.w * s,
	};
}
interop::vec4 interop::operator*(const float s, const interop::vec4 v) {
	return v * s;
}

