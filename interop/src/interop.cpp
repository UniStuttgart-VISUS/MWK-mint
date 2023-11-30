#include "interop.hpp"

#include "SpoutReceiver.h"
#include "SpoutSender.h"
#include <nlohmann/json.hpp>
#include <zmq.hpp>

#include <iostream>

using json = nlohmann::json;

// we need to load some GL function names by hand if we don't use a loader
// library
// TODO: cross-link with GL application?
namespace {

#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif

#define MAKE_GL_CALL(FUNCTION_NAME, RETURN_TYPE, ...)                          \
  typedef RETURN_TYPE(APIENTRYP FUNCTION_NAME##FuncPtr)(__VA_ARGS__);          \
  FUNCTION_NAME##FuncPtr m##FUNCTION_NAME;

	// typedef void (APIENTRYP glDrawBuffersEXTFuncPtr)(GLsizei n, const GLenum
	// *bufs); glDrawBuffersEXTFuncPtr my_glDrawBuffersEXT; #define glDrawBuffersEXT
	// my_glDrawBuffersEXT
	typedef char GLchar;
	typedef int GLint;
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_VERTEX_SHADER 0x8B31
#define GL_ARRAY_BUFFER 0x8892
#define GL_LINK_STATUS 0x8B82
#define GL_TEXTURE0 0x84C0
#define GL_ACTIVE_TEXTURE 0x84E0
#define GL_DEPTH_COMPONENT32 0x81A7
	// MAKE_GL_CALL(glDrawBuffersEXT, void, GLsizei n, const GLenum *bufs)
	MAKE_GL_CALL(glCreateShader, GLuint, GLenum shaderType)
		MAKE_GL_CALL(glShaderSource, void, GLuint shader, GLsizei count,
			const GLchar** string, const GLint* length)
		MAKE_GL_CALL(glCompileShader, void, GLuint shader)
		MAKE_GL_CALL(glCreateProgram, GLuint, void)
		MAKE_GL_CALL(glAttachShader, void, GLuint program, GLuint shader)
		MAKE_GL_CALL(glLinkProgram, void, GLuint program)
		MAKE_GL_CALL(glDeleteShader, void, GLuint shader)
		MAKE_GL_CALL(glGetUniformLocation, GLint, GLuint program, const GLchar* name)
		MAKE_GL_CALL(glGenVertexArrays, void, GLsizei n, GLuint* arrays)
		MAKE_GL_CALL(glBindVertexArray, void, GLuint array)
		MAKE_GL_CALL(glBindBuffer, void, GLenum target, GLuint buffer)
		MAKE_GL_CALL(glDeleteProgram, void, GLuint program)
		MAKE_GL_CALL(glDeleteVertexArrays, void, GLsizei n, const GLuint* arrays)
		MAKE_GL_CALL(glGetProgramiv, void, GLuint program, GLenum pname, GLint* params)
		MAKE_GL_CALL(glGetProgramInfoLog, void, GLuint program, GLsizei maxLength,
			GLsizei* length, GLchar* infoLog)
		MAKE_GL_CALL(glUseProgram, void, GLuint shader)
		// MAKE_GL_CALL(glDrawArrays, void, GLenum mode, GLint first, GLsizei count)
		MAKE_GL_CALL(glActiveTexture, void, GLenum texture)
		// MAKE_GL_CALL(glBindTexture, void, GLenum target, GLuint texture)
		MAKE_GL_CALL(glUniform1i, void, GLint location, GLint v0)
		MAKE_GL_CALL(glUniform2i, void, GLint location, GLint v0, GLint v1)
		MAKE_GL_CALL(glCopyTexSubImage2D, void, GLenum, GLint, GLint, GLint, GLint,
			GLint, GLsizei, GLsizei)

		MAKE_GL_CALL(glGenFramebuffersEXT, void, GLsizei n, GLuint* ids)
		MAKE_GL_CALL(glBindFramebufferEXT, void, GLenum target, GLuint framebuffer)
		MAKE_GL_CALL(glFramebufferTexture2DEXT, void, GLenum target, GLenum attachment,
			GLenum textarget, GLuint texture, GLint level)
		MAKE_GL_CALL(glCheckFramebufferStatusEXT, GLenum, GLenum target)
		MAKE_GL_CALL(glDeleteFramebuffersEXT, void, GLsizei n, GLuint* framebuffers)
		MAKE_GL_CALL(glBlitFramebufferEXT, void, GLint srcX0, GLint srcY0, GLint srcX1,
			GLint srcY1, GLint dstX0, GLint dstY0, GLint dstX1, GLint dstY1,
			GLbitfield mask, GLenum filter)

		void loadGlExtensions() {
		static bool hasInit = false;
		if (hasInit) {
			return;
		}
		hasInit = true;

#define GET_GL_CALL(FUNCTION_NAME)                                             \
  const auto FUNCTION_NAME##fptr = wglGetProcAddress(#FUNCTION_NAME);          \
  m##FUNCTION_NAME = (FUNCTION_NAME##FuncPtr)FUNCTION_NAME##fptr;              \
  if (FUNCTION_NAME##fptr == nullptr)                                          \
    std::cout << "Fatal OpenGL Error: could not load function '"               \
              << "m" #FUNCTION_NAME "'" << std::endl;

		// const auto fptr = wglGetProcAddress("glDrawBuffers");
		// glDrawBuffersEXT = (glDrawBuffersEXTFuncPtr)fptr;
		// GET_GL_CALL(glDrawBuffersEXT)
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
			GET_GL_CALL(glBindBuffer)
			GET_GL_CALL(glDeleteProgram)
			GET_GL_CALL(glDeleteVertexArrays)
			GET_GL_CALL(glGetProgramiv)
			GET_GL_CALL(glGetProgramInfoLog)
			GET_GL_CALL(glUseProgram)
			// GET_GL_CALL(glDrawArrays)
			GET_GL_CALL(glActiveTexture)
			// GET_GL_CALL(glBindTexture)
			GET_GL_CALL(glUniform1i)
			GET_GL_CALL(glUniform2i)
			GET_GL_CALL(glCopyTexSubImage2D)

			GET_GL_CALL(glGenFramebuffersEXT)
			GET_GL_CALL(glBindFramebufferEXT)
			GET_GL_CALL(glFramebufferTexture2DEXT)
			GET_GL_CALL(glCheckFramebufferStatusEXT)
			GET_GL_CALL(glDeleteFramebuffersEXT)
			GET_GL_CALL(glBlitFramebufferEXT)
	}

	static zmq::context_t g_zmqContext{};

	static std::string mint_lib_identity{ "mint Minimal Interoperation Lib" };
} // namespace

static const void savePreviousFbo(interop::glFramebuffer* fbo) {
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING_EXT,
		&fbo->m_previousFramebuffer[0]);
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING_EXT,
		&fbo->m_previousFramebuffer[1]);
	glGetIntegerv(GL_VIEWPORT, fbo->m_previousViewport);
};
static const void restorePreviousFbo(interop::glFramebuffer* fbo) {
	mglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, fbo->m_previousFramebuffer[0]);
	mglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, fbo->m_previousFramebuffer[1]);
	glViewport(fbo->m_previousViewport[0], fbo->m_previousViewport[1],
		fbo->m_previousViewport[2], fbo->m_previousViewport[3]);

	fbo->m_previousFramebuffer[0] = 0;
	fbo->m_previousFramebuffer[1] = 0;
	for (int i = 0; i < 4; i++)
		fbo->m_previousViewport[i] = 0;
}

struct Addresses {
	std::string send;
	std::string receive;
};

struct DataProtocol {
	std::vector<Addresses> steering_rendering;
};

static const std::vector<DataProtocol> protocol_addresses = {
	// IPC
	DataProtocol{{
		{"ipc:///tmp/mint_steering_send", "ipc:///tmp/mint_steering_receive"}/*steering send/receive*/,
		{"ipc:///tmp/mint_steering_receive", "ipc:///tmp/mint_steering_send"}/*rendering send/receive*/,
	}},
	// TCP
	DataProtocol{{
		{"tcp://127.0.0.1:12345", "tcp://localhost:12346"}/*steering send/receive*/,
		{"tcp://127.0.0.1:12346", "tcp://localhost:12345"}/*rendering send/receive*/,
	}},
};

static const std::string texture_sharing_address = "/mint/texturesharing";

static Addresses session_addresses;
static interop::ImageProtocol session_texture_sharing;

std::string to_string(interop::ImageType side) {
	std::string ret;

	switch (side) {
	case interop::ImageType::LeftEye:
		ret = "Left";
		break;
	case interop::ImageType::RightEye:
		ret = "Right";
		break;
	case interop::ImageType::SingleStereo:
		ret = "SingleStereo";
		break;
	default:
		break;
	}

	return ret;
}

void interop::init(Role r, DataProtocol dp, ImageProtocol ip) {
	loadGlExtensions();

	session_addresses = protocol_addresses[static_cast<unsigned int>(dp)].steering_rendering[static_cast<unsigned int>(r)];
	session_texture_sharing = ip;
}

// GL functions are presented by Spout!
void interop::glFramebuffer::init(uint width, uint height) {
	loadGlExtensions();

	m_width = width;
	m_height = height;

	if (!m_glFbo)
		mglGenFramebuffersEXT(1, &m_glFbo);

	if (!m_glTextureRGBA8)
		glGenTextures(1, &m_glTextureRGBA8);
	glBindTexture(GL_TEXTURE_2D, m_glTextureRGBA8);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA,
		GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	if (!m_glTextureDepth)
		glGenTextures(1, &m_glTextureDepth);
	glBindTexture(GL_TEXTURE_2D, m_glTextureDepth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, m_width, m_height, 0,
		GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindTexture(GL_TEXTURE_2D, 0);

	savePreviousFbo(this);
	mglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_glFbo);
	mglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
		GL_TEXTURE_2D, m_glTextureRGBA8, 0);
	mglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
		GL_TEXTURE_2D, m_glTextureDepth, 0);

	GLenum e = mglCheckFramebufferStatusEXT(GL_DRAW_FRAMEBUFFER_EXT);
	if (e != GL_FRAMEBUFFER_COMPLETE_EXT)
		printf("There is a problem with the FBO\n");

	restorePreviousFbo(this);
}

void interop::glFramebuffer::destroy() {
	if (m_glFbo)
		mglDeleteFramebuffersEXT(1, &m_glFbo);

	if (m_glTextureRGBA8)
		glDeleteTextures(1, &m_glTextureRGBA8);

	if (m_glTextureDepth)
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
	mglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_glFbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
	glViewport(0, 0, m_width, m_height);
}

void interop::glFramebuffer::unbind() { restorePreviousFbo(this); }

void interop::glFramebuffer::blitTexture() {
	savePreviousFbo(this);

	mglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, m_previousFramebuffer[0]);
	mglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, m_glFbo);

	mglBlitFramebufferEXT(0, 0, m_width, m_height, m_previousViewport[0],
		m_previousViewport[1], m_previousViewport[2],
		m_previousViewport[3],
		GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

	restorePreviousFbo(this);
}

#define m_spout (static_cast<SpoutSender *>(m_sender.get()))
#define error                                                                  \
  {                                                                            \
    printf("there was an error");                                              \
    return;                                                                    \
  }
interop::TextureSender::TextureSender() {
	m_sender = std::make_shared<SpoutSender>();
}

interop::TextureSender::~TextureSender() { m_sender = nullptr; }

void interop::TextureSender::init(ImageType type, std::string name, uint width, uint height) {
	this->init(name + to_string(type), width, height);
}

void interop::TextureSender::init(std::string name, uint width, uint height) {
	if (name.size() == 0 || name.size() > 256)
		return;

	if (width == 0 || height == 0)
		return;

	if (!m_sender)
		return;

	m_name = texture_sharing_address + "/" + name;

	m_name.resize(256, '\0'); // Spout doc says sender name MUST have 256 bytes

	m_width = width;
	m_height = height;

	m_spout->SetVerticalSync(0);
	m_spout->SetDX9(false);

	std::vector<std::string> map = {
		"GPU",
		"CPU",
		"Memory Share",
	};

	auto mode = static_cast<int>(session_texture_sharing);
	if (!m_spout->SetShareMode(mode))
		std::cout << "mint: setting spout texture sharing mode to " << map[mode] << std::endl;

	// DXGI_FORMAT_R8G8B8A8_UNORM; // default DX11 format - compatible with DX9
	// (28)
	unsigned int format = 28;
	m_spout->CreateSender(m_name.c_str(), m_width, m_height, format);
}

void interop::TextureSender::destroy() {
	if (!m_sender)
		return;

	m_spout->ReleaseSender();
	m_width = 0;
	m_height = 0;
	m_name = "";
}

bool interop::TextureSender::resize(uint width, uint height) {
	if (!m_sender)
		return false;

	if (width == 0 || height == 0)
		return false;

	if (m_width != width || m_height != height) {
		m_width = width;
		m_height = height;

		m_spout->UpdateSender(m_name.c_str(), m_width, m_height);
	}

	return true;
}

void interop::TextureSender::send(uint textureHandle, uint width,
	uint height) {
	if (textureHandle == 0)
		return;

	if (!resize(width, height))
		return;

	m_spout->SendTexture(textureHandle, GL_TEXTURE_2D, m_width, m_height);
}
#undef m_spout

void interop::TextureSender::send(glFramebuffer& fb) {
	this->send(fb.m_glTextureRGBA8, fb.m_width, fb.m_height);
}

#define m_spout (static_cast<SpoutReceiver *>(m_receiver.get()))
interop::TextureReceiver::TextureReceiver() {
	m_receiver = std::make_shared<SpoutReceiver>();
}

interop::TextureReceiver::~TextureReceiver() { m_receiver = nullptr; }

void interop::TextureReceiver::init(ImageType type, std::string name) {
	this->init(name + to_string(type));
}

void interop::TextureReceiver::init(std::string name) {
	if (name.size() == 0 || name.size() > 256)
		return;

	m_name = texture_sharing_address + "/" + name;
	m_name.resize(256, '\0'); // Spout doc says sender name MUST have 256 bytes

	m_spout->SetVerticalSync(0);
	m_spout->SetDX9(false);

	std::vector<std::string> map = {
		"GPU",
		"CPU",
		"Memory Share",
	};

	auto mode = static_cast<int>(session_texture_sharing);
	if (!m_spout->SetShareMode(mode))
		std::cout << "SPOUT: failed setting spout texture sharing mode to " << map[mode] << std::endl;
	bool active = false;
	unsigned int w = 1, h = 1;

	if (!m_spout->CreateReceiver(const_cast<char*>(m_name.c_str()), w, h, active))
		std::cout << "SPOUT: failed creating receiver " << m_name << std::endl;
}

void interop::TextureReceiver::destroy() {
	if (!m_receiver)
		return;

	m_spout->ReleaseReceiver();
}

void interop::TextureReceiver::receive() {
	uint width = m_width, height = m_height;

	const auto make_texture = [&]() {
		glBindTexture(GL_TEXTURE_2D, m_texture_handle);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA,
			GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glBindTexture(GL_TEXTURE_2D, 0);
		};

	glFramebuffer fbo_backup;
	savePreviousFbo(&fbo_backup);

	if (!m_texture_handle) {
		glGenTextures(1, &m_texture_handle);
		make_texture();
	}

	if (!m_source_fbo) {
		mglGenFramebuffersEXT(1, &m_source_fbo);
		mglGenFramebuffersEXT(1, &m_target_fbo);
	}

	//bool connected = false;
	//if (!m_spout->CheckReceiver(const_cast<char*>(m_name.c_str()), width,
	//	height, connected) && session_texture_sharing == mint::ImageProtocol::GPU) {
	//	std::cout << "SPOUT receive texture failed check, connected? " << (connected ? "yes" : "no") << std::endl;
	//	std::cout << "SPOUT: name: " << m_name << ", width: " << width << ", height: " << height << std::endl;
	//	return;
	//}

	if (!m_spout->ReceiveTexture(const_cast<char*>(m_name.c_str()), width,
		height
		, m_texture_handle, GL_TEXTURE_2D, true /*invert*/, 0
	)) {
		std::cout << "SPOUT failed to receive texture" << std::endl;
		return;
	}

	if (m_width != width || m_height != height) {
		m_width = width;
		m_height = height;
		make_texture();
	}

	//std::cout << "SPOUT: reports texture: with: " << m_width << ", height: " << m_height << ", handle: " << m_texture_handle << ", name: " << m_name.c_str() << std::endl;

	//if (!m_spout->BindSharedTexture())
	//	return;

	//int shared_texture_id = 0;
	//glGetIntegerv(GL_TEXTURE_BINDING_2D, &shared_texture_id);
	mglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, m_source_fbo);
	mglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
		GL_TEXTURE_2D, m_texture_handle, 0);

	GLenum e = mglCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (e != GL_FRAMEBUFFER_COMPLETE_EXT)
		printf("There is a problem with the FBO\n");

	//mglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo_backup.m_previousFramebuffer[0]);
	//mglFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
	//	GL_TEXTURE_2D, m_texture_handle, 0);
	e = mglCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (e != GL_FRAMEBUFFER_COMPLETE_EXT)
		printf("There is a problem with the FBO\n");

	mglBindFramebufferEXT(GL_DRAW_FRAMEBUFFER_EXT, fbo_backup.m_previousFramebuffer[0]);
	mglBindFramebufferEXT(GL_READ_FRAMEBUFFER_EXT, m_source_fbo);

	// flip y axis of dx texture
	mglBlitFramebufferEXT(0, m_height, m_width, 0, 0, 0, m_width, m_height,
		GL_COLOR_BUFFER_BIT, //| GL_DEPTH_BUFFER_BIT,
		GL_NEAREST);

	mglBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glBindTexture(GL_TEXTURE_2D, 0);
	//m_spout->UnBindSharedTexture();

	restorePreviousFbo(&fbo_backup);
}
#undef m_spout

interop::StereoTextureSender::StereoTextureSender() {}
interop::StereoTextureSender::~StereoTextureSender() {}

void interop::StereoTextureSender::init(std::string name, uint width, uint height) {

	m_width = width;
	m_height = height;

	m_hugeFbo.init();
	m_hugeTextureSender.init(interop::ImageType::SingleStereo, name, width, height);
	this->initGLresources();

	this->makeHugeTexture(m_width, m_height);
}

void interop::StereoTextureSender::destroy() {
	m_hugeFbo.destroy();
	m_hugeTextureSender.destroy();
	this->destroyGLresources();
}

void interop::StereoTextureSender::makeHugeTexture(const uint originalWidth,
	const uint originalHeight) {
	m_hugeWidth = originalWidth * 2;
	m_hugeHeight = originalHeight * 2;

	m_hugeFbo.resizeTexture(m_hugeWidth, m_hugeHeight);
	m_hugeTextureSender.resize(m_hugeWidth,
		m_hugeHeight); // resizes sender texture
}

void interop::StereoTextureSender::send(glFramebuffer& fb_left,
	glFramebuffer& fb_right,
	const uint width,
	const uint height,
	const uint meta_data) {
	this->send(fb_left.m_glTextureRGBA8, fb_right.m_glTextureRGBA8,
		fb_left.m_glTextureDepth, fb_right.m_glTextureDepth,
		width, height, meta_data);
}

void interop::StereoTextureSender::send(
	const uint color_left, const uint color_right, const uint depth_left,
	const uint depth_right, const uint width, const uint height,
	const uint meta_data) {
	if (m_width != width || m_height != height)
		this->makeHugeTexture(width, height);

	this->blitTextures(color_left, color_right, depth_left, depth_right, width,
		height, meta_data);

	m_hugeTextureSender.send(m_hugeFbo);
}

void interop::StereoTextureSender::initGLresources() {
	const char* vertex_shader_source =
		R"(
	#version 400

	//in int gl_VertexID;

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
	uniform int meta_data;

	out vec4 FragColor;

	void main()
	{
		vec4 result = vec4(0.0f);
		ivec2 screen_coords = ivec2(gl_FragCoord.xy);

		if(screen_coords == ivec2(0,0)) {
			FragColor = unpackUnorm4x8(uint(meta_data));
			return;
		}

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
			//vec4 depth_rgba8 = unpackUnorm4x8(deph_uint); // version 400
			vec4 depth_rgba8 = vec4(
				float((deph_uint >> 0 ) & 0x000000FF),           // r: 0..7
				float((deph_uint >> 8 ) & 0x000000FF),           // g: 8..15
				float((deph_uint >> 16) & 0x000000FF),           // b: 16..23
				float((deph_uint >> 24) & 0x000000FF) ) / 255.0; // a: 24..31

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
		mglGetProgramInfoLog(m_shader, 512, NULL,
			const_cast<char*>(infoLog.c_str()));
		std::cout << infoLog << std::endl;
	}

	mglDeleteShader(vertex_shader);
	mglDeleteShader(fragment_shader);

	m_uniform_locations[0] = mglGetUniformLocation(m_shader, "texture_size");
	m_uniform_locations[1] = mglGetUniformLocation(m_shader, "left_color");
	m_uniform_locations[2] = mglGetUniformLocation(m_shader, "left_depth");
	m_uniform_locations[3] = mglGetUniformLocation(m_shader, "right_color");
	m_uniform_locations[4] = mglGetUniformLocation(m_shader, "right_depth");
	m_uniform_locations[5] = mglGetUniformLocation(m_shader, "meta_data");

	mglGenVertexArrays(1, &m_vao);
}

void interop::StereoTextureSender::destroyGLresources() {
	mglDeleteProgram(m_shader);
	mglDeleteVertexArrays(1, &m_vao);
}

void interop::StereoTextureSender::blitTextures(
	const uint color_left_texture, const uint color_right_texture,
	const uint depth_left_texture, const uint depth_right_texture,
	const uint width, const uint height, const uint meta_data) {
	GLint previous_active_texture;
	glGetIntegerv(GL_ACTIVE_TEXTURE, &previous_active_texture);

	m_hugeFbo.bind(); // also sets viewport
	mglUseProgram(m_shader);
	mglBindVertexArray(m_vao);
	// TODO: set + restore buffer reset values
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	const auto bindTexture = [&](const uint texture_uniform_location,
		const uint texture_handle,
		const uint binding_point) {
			mglActiveTexture(GL_TEXTURE0 +
				binding_point); // activate the texture unit first before
			// binding texture
			glBindTexture(GL_TEXTURE_2D, texture_handle);
			mglUniform1i(texture_uniform_location, binding_point); // set it manually
		};

	mglUniform2i(m_uniform_locations[0], static_cast<int>(width),
		static_cast<int>(height));
	bindTexture(m_uniform_locations[1], color_left_texture, 0);
	bindTexture(m_uniform_locations[2], depth_left_texture, 1);
	bindTexture(m_uniform_locations[3], color_right_texture, 2);
	bindTexture(m_uniform_locations[4], depth_right_texture, 3);
	mglUniform1i(m_uniform_locations[5], static_cast<int>(meta_data));

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	mglBindVertexArray(0);
	m_hugeFbo.unbind();
	mglActiveTexture(previous_active_texture); // activate the texture unit first
	// before binding texture
}

#define m_socket (*static_cast<zmq::socket_t *>(m_sender.get()))
interop::DataSender::DataSender() {
	m_address = session_addresses.send;
}
interop::DataSender::DataSender(std::string const& address) {
	m_address = address;
}
interop::DataSender::~DataSender() {}

void interop::DataSender::start(Endpoint socket_type) {

	const auto& networkAddress = m_address;

	m_sender =
		std::make_shared<zmq::socket_t>(g_zmqContext, zmq::socket_type::pub);
	auto& socket = m_socket;

	// if (socket.connected())
	//	return;

	try {
		socket.setsockopt(ZMQ_IDENTITY, mint_lib_identity.data(), mint_lib_identity.size());

		switch (socket_type) {
		case interop::Endpoint::Bind:
			socket.bind(networkAddress);
			break;
		case interop::Endpoint::Connect:
			socket.connect(networkAddress);
			break;
		}
	}
	catch (std::exception& e) {
		std::cout << "InteropLib: binding zmq socket failed: " << e.what() << " - "
			<< networkAddress << std::endl;
		return;
	}
}
void interop::DataSender::stop() {
	if (m_socket.connected())
		m_socket.close();
}

bool interop::DataSender::send_raw(
	std::string const& v,
	std::string const& filterName)
{
	auto& socket = m_socket;

	zmq::message_t address_msg{ filterName.data(), filterName.size() };
	zmq::message_t data_msg{ v.data(), v.size() };

	// std::cout << "ZMQ Sender: " << filterName << " / " << v << std::endl;
	bool sendingResult = false;
	const auto print = [](bool b) -> std::string {
		return (b ? ("true") : ("false"));
		};
	try {
		bool adr = socket.send(address_msg, ZMQ_SNDMORE);
		bool msg = socket.send(data_msg);

		// std::cout << "ZMQ Sender: adr=" << print(adr) << ", msg=" << print(msg)
		// << ", " << v << std::endl;
		sendingResult = adr && msg;
	}
	catch (std::exception& e) {
		std::cout << "InteropLib: ZMQ sending failed: " << e.what() << std::endl;
	}
	return sendingResult;
}
#undef m_socket

interop::DataReceiver::DataReceiver() {
	m_address = session_addresses.receive;
}
interop::DataReceiver::DataReceiver(std::string const& address) {
	m_address = address;
}

interop::DataReceiver::~DataReceiver() { stop(); }

//#define m_socket (*static_cast<zmq::socket_t *>(m_receiver.get()))
bool interop::DataReceiver::start(const std::string& filterName,
	Endpoint socket_type) {

	auto networkAddress = m_address;

	m_filterName = filterName;

	m_worker_signal.test_and_set();

	// we need this async receiver worker to churn through all messages
	// and have the latest one ready when somebody asks for it
	// because the ZMQ_CONFLATE option (keeping only latest message) 
	// does not work with PUB/SUB sockets
	m_worker = std::thread{ [&, filterName, networkAddress, socket_type]() {
		zmq::socket_t socket(g_zmqContext, zmq::socket_type::sub);

	  try {
		socket.setsockopt(ZMQ_IDENTITY, mint_lib_identity.data(), mint_lib_identity.size());

		//socket.setsockopt(ZMQ_CONFLATE, 1); // keep only most recent message, dont work with SUB pattern

		socket.setsockopt(
			ZMQ_SUBSCRIBE, filterName.data(),
			filterName.size()); // only receive messages with prefix given by filterName

		socket.setsockopt(ZMQ_RCVTIMEO,5/*ms*/); // timeout after not receiving messages

		switch (socket_type) {
		case interop::Endpoint::Bind:
		  socket.bind(networkAddress);
		  break;
		case interop::Endpoint::Connect:
		  socket.connect(networkAddress);
		  break;
		}
	  }
   catch (std::exception& e) {
  std::cout << "InteropLib: connecting zmq socket failed: " << e.what()
			<< std::endl;
  m_worker_result = false;
  return;
}

zmq::message_t address_msg;
zmq::message_t content_msg;
zmq::recv_result_t result;

while (m_worker_signal.test_and_set()) {

	if (!socket.recv(address_msg))// , zmq::recv_flags::dontwait))
		continue;

	if (!address_msg.more() || !socket.recv(content_msg))// , zmq::recv_flags::dontwait))
		continue;

	const bool log = false;
	if (log) {
	  std::cout << "zmq received address: "
				<< address_msg.to_string_view()
				<< std::endl;
	  std::cout << "zmq received content: "
				<< content_msg.to_string_view()
				<< std::endl;
	}

	{
		std::lock_guard<std::mutex> lock(m_mutex);
		m_messages[address_msg.to_string()] = content_msg.to_string();
	}
}

socket.close();
m_worker_result = true;
} };

	return true;
}

void interop::DataReceiver::stop() {
	m_worker_signal.clear();
	m_worker.join();
}

std::optional<std::string> interop::DataReceiver::receiveCopy(const std::string& filterName) {
	auto f = filterName.empty() ? m_filterName : filterName;

	std::optional<std::string> r = std::nullopt;
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		auto found = m_messages.find(f);

		if (found != m_messages.end()) {
			r = std::make_optional(found->second);
		}
	}

	return r;
}
#undef m_socket

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

#define writeVal(name)                                                         \
  { #name, v.##name }
#define readVal(name) j.at(#name).get_to(v.##name);
#define readObj(name) from_json(j.at(#name), v.##name)
	void to_json(json& j, const vec4& v) {
		j = json{ writeVal(x), writeVal(y), writeVal(z), writeVal(w) };
	}
	void from_json(const json& j, vec4& v) {
		readVal(x);
		readVal(y);
		readVal(z);
		readVal(w);
	}

	// mat4 is somewhat special regarding the json format coming from Unity
	void to_json(json& j, const mat4& v) {
		j = json{ {"e00", v.data[0].x}, {"e01", v.data[1].x}, {"e02", v.data[2].x},
				 {"e03", v.data[3].x}, {"e10", v.data[0].y}, {"e11", v.data[1].y},
				 {"e12", v.data[2].y}, {"e13", v.data[3].y}, {"e20", v.data[0].z},
				 {"e21", v.data[1].z}, {"e22", v.data[2].z}, {"e23", v.data[3].z},
				 {"e30", v.data[0].w}, {"e31", v.data[1].w}, {"e32", v.data[2].w},
				 {"e33", v.data[3].w} };
	}
#define readMat(name, value) j.at(name).get_to(##value);
	void from_json(const json& j, mat4& v) {
		readMat("e00", v.data[0].x);
		readMat("e01", v.data[1].x);
		readMat("e02", v.data[2].x);
		readMat("e03", v.data[3].x);
		readMat("e10", v.data[0].y);
		readMat("e11", v.data[1].y);
		readMat("e12", v.data[2].y);
		readMat("e13", v.data[3].y);
		readMat("e20", v.data[0].z);
		readMat("e21", v.data[1].z);
		readMat("e22", v.data[2].z);
		readMat("e23", v.data[3].z);
		readMat("e30", v.data[0].w);
		readMat("e31", v.data[1].w);
		readMat("e32", v.data[2].w);
		readMat("e33", v.data[3].w);
	}

	void to_json(json& j, const CameraView& v) {
		j = json{ writeVal(eyePos), writeVal(lookAtPos), writeVal(camUpDir) };
	}
	void from_json(const json& j, CameraView& v) {
		readObj(eyePos);
		readObj(lookAtPos);
		readObj(camUpDir);
	}

	void to_json(json& j, const StereoCameraView& v) {
		j = json{ writeVal(leftEyeView), writeVal(rightEyeView) };
	}
	void from_json(const json& j, StereoCameraView& v) {
		readObj(leftEyeView);
		readObj(rightEyeView);
	}

	void to_json(json& j, const StereoCameraViewRelative& v) {
		j = json{ writeVal(leftEyeView), writeVal(rightEyeView) };
	}
	void from_json(const json& j, StereoCameraViewRelative& v) {
		readObj(leftEyeView);
		readObj(rightEyeView);
	}

	void to_json(json& j, const CameraProjection& v) {
		j = json{ writeVal(fieldOfViewY_rad), writeVal(nearClipPlane),
				 writeVal(farClipPlane),     writeVal(aspect),
				 writeVal(pixelWidth),       writeVal(pixelHeight) };
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
		j = json{ writeVal(viewParameters), writeVal(projectionParameters),
				 writeVal(viewMatrix), writeVal(projectionMatrix) };
	}
	void from_json(const json& j, CameraConfiguration& v) {
		readObj(viewParameters);
		readObj(projectionParameters);
		readObj(viewMatrix);
		readObj(projectionMatrix);
	}

	void to_json(json& j, const StereoCameraConfiguration& v) {
		j = json{ writeVal(stereoConvergence), writeVal(stereoSeparation),
				 writeVal(cameraLeftEye), writeVal(cameraRightEye) };
	}
	void from_json(const json& j, StereoCameraConfiguration& v) {
		readVal(stereoConvergence);
		readVal(stereoSeparation);
		readObj(cameraLeftEye);
		readObj(cameraRightEye);
	}

	void to_json(json& j, const BoundingBoxCorners& v) {
		j = json{ writeVal(min), writeVal(max) };
	}
	void from_json(const json& j, BoundingBoxCorners& v) {
		readObj(min);
		readObj(max);
	}

	void to_json(json& j, const ModelPose& v) {
		j = json{ writeVal(translation), writeVal(scale),
				 writeVal(rotation_axis_angle_rad), writeVal(modelMatrix) };
	}
	void from_json(const json& j, ModelPose& v) {
		readObj(translation);
		readObj(scale);
		readObj(rotation_axis_angle_rad);
		readObj(modelMatrix);
	}

	void to_json(json& j, const DatasetRenderConfiguration& v) {
		j = json{ writeVal(stereoCamera), writeVal(modelTransform) };
	}
	void from_json(const json& j, DatasetRenderConfiguration& v) {
		readObj(stereoCamera);
		readObj(modelTransform);
	}
} // namespace interop

#define make_dataGet(DataTypeName)                                 \
  template <>                                                                  \
  bool interop::DataReceiver::receive<DataTypeName>(DataTypeName &v, const std::string &filterName) {        \
    if (auto data = this->receiveCopy(filterName); data.has_value()) {         \
      auto &byteData = data.value();                                          \
      if (byteData.size()) {                                                   \
        json j = json::parse(byteData);                                        \
        v = j.get<DataTypeName>();                                             \
        return true;                                                           \
      }                                                                        \
    }                                                                          \
                                                                               \
    return false;                                                              \
  }

#define make_name(DataTypeName) \
    template <> \
    std::string interop::to_data_name(DataTypeName const& v) { return std::string{#DataTypeName}; }
make_name(interop::BoundingBoxCorners);
make_name(interop::CameraProjection);
make_name(interop::StereoCameraView);
make_name(interop::StereoCameraViewRelative);
#undef make_name

#define make_named_receive(DataTypeName) \
  template <>                                                         \
  bool interop::DataReceiver::receive<DataTypeName>(DataTypeName &v) {\
    return this->receive(v, interop::to_data_name(v));\
  }

make_named_receive(interop::BoundingBoxCorners);
make_named_receive(interop::CameraProjection);
make_named_receive(interop::StereoCameraView);
make_named_receive(interop::StereoCameraViewRelative);
#undef make_named_receive

// when adding to those macros, don't forget to also define 'to_json' and
// 'from_json' functions above for the new data type
make_dataGet(interop::BoundingBoxCorners);
make_dataGet(interop::DatasetRenderConfiguration);
make_dataGet(interop::ModelPose);
make_dataGet(interop::StereoCameraConfiguration);
make_dataGet(interop::CameraConfiguration);
make_dataGet(interop::CameraProjection);
make_dataGet(interop::StereoCameraView);
make_dataGet(interop::StereoCameraViewRelative);
make_dataGet(interop::CameraView);
make_dataGet(interop::mat4);
make_dataGet(interop::vec4);

#define make_scalar_get(DataTypeName)                                          \
  template <>                                                                  \
  bool interop::DataReceiver::receive<DataTypeName>(DataTypeName &v, const std::string &filterName) {        \
    if (auto data = this->receiveCopy(filterName); data.has_value()) {                   \
      auto &byteData = data.value();                                           \
      if (byteData.size()) {                                                   \
        json j = json::parse(byteData);                                        \
        v = j["value"];                                                        \
        return true;                                                           \
      }                                                                        \
    }                                                                          \
                                                                               \
    return false;                                                              \
  }

make_scalar_get(bool);
make_scalar_get(int);
make_scalar_get(unsigned int);
make_scalar_get(float);
make_scalar_get(double);

#define make_send(DataTypeName)                                            \
  template <>                                                                  \
  bool interop::DataSender::send<DataTypeName>(                            \
      DataTypeName const &v, std::string const &filterName) {                  \
    const json j = v;                                                          \
    const std::string jsonString = j.dump();                                   \
    return this->send_raw(jsonString, filterName);                             \
  }

// when adding to those macros, don't forget to also define 'to_json' and
// 'from_json' functions above for the new data type
make_send(interop::BoundingBoxCorners);
make_send(interop::DatasetRenderConfiguration);
make_send(interop::ModelPose);
make_send(interop::StereoCameraConfiguration);
make_send(interop::CameraConfiguration);
make_send(interop::CameraProjection);
make_send(interop::StereoCameraView);
make_send(interop::StereoCameraViewRelative);
make_send(interop::CameraView);
make_send(interop::mat4);
make_send(interop::vec4);
#undef make_send

#define make_named_send(DataTypeName)                                            \
  template <>                                                                  \
  bool interop::DataSender::send<DataTypeName>(                            \
      DataTypeName const &v) { return this->send(v, interop::to_data_name(v)); }
make_named_send(interop::BoundingBoxCorners);
make_named_send(interop::CameraProjection);
make_named_send(interop::StereoCameraView);
make_named_send(interop::StereoCameraViewRelative);
#undef make_send

#define make_send_scalar(DataTypeName)                                            \
  template <>                                                                  \
  bool interop::DataSender::send<DataTypeName>(                            \
      DataTypeName const &v, std::string const &filterName) {                  \
    json j;\
    j["value"] = v;                                                          \
    const std::string jsonString = j.dump();                                   \
    return this->send_raw(jsonString, filterName);                             \
  }
make_send_scalar(bool);
make_send_scalar(int);
make_send_scalar(unsigned int);
make_send_scalar(float);
make_send_scalar(double);
#undef make_send_scalar


// interop::vec4 arithmetic operators
interop::vec4 interop::operator+(interop::vec4 const& lhs,
	interop::vec4 const& rhs) {
	return vec4{
		lhs.x + rhs.x,
		lhs.y + rhs.y,
		lhs.z + rhs.z,
		lhs.w + rhs.w,
	};
}
interop::vec4 interop::operator+=(vec4& lhs, vec4 const& rhs) {
	lhs.x += rhs.x;
	lhs.y += rhs.y;
	lhs.z += rhs.z;
	lhs.w += rhs.w;
	return lhs;
}
interop::vec4 interop::operator-(interop::vec4 const& lhs,
	interop::vec4 const& rhs) {
	return vec4{
		lhs.x - rhs.x,
		lhs.y - rhs.y,
		lhs.z - rhs.z,
		lhs.w - rhs.w,
	};
}
interop::vec4 interop::operator*(interop::vec4 const& lhs,
	interop::vec4 const& rhs) {
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
bool interop::operator==(const vec4& l, const vec4& r) {
	return
		l.x == r.x &&
		l.y == r.y &&
		l.z == r.z &&
		l.w == r.w;
}
bool interop::operator!=(const vec4& l, const vec4& r) {
	return !(l == r);
}

