#include "interop.hpp"

#include <zmq.hpp>
#include "SpoutSender.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

int func()
{
	Spout sp;
	zmq::context_t c;
	json j;
	return sp.GetVerticalSync();
}


// we need to load some GL function names by hand if we don't use a loader library
// TODO: cross-link with GL application?
namespace {
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif

	typedef void (APIENTRYP glDrawBuffersFuncPtr)(GLsizei n, const GLenum *bufs);
	glDrawBuffersFuncPtr my_glDrawBuffers;
	#define glDrawBuffersEXT my_glDrawBuffers

	void loadGlExtensions()
	{
		Spout sp;
		sp.OpenSpout(); // loads GL functions
		const auto fptr = wglGetProcAddress("glDrawBuffers");
		glDrawBuffersEXT = (glDrawBuffersFuncPtr)fptr;
	}
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
	if (!glDrawBuffersEXT)
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
	//GLuint attachments[1] = { GL_COLOR_ATTACHMENT0_EXT };
	//glDrawBuffersEXT(1, attachments);
	glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
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
#undef glDrawBuffersEXT


#define m_spout (static_cast<SpoutSender*>(m_sender))
#define error { printf("there was an error"); return; }
interop::TextureSender::TextureSender() {
	auto spoutPtr = new SpoutSender;
	m_sender = static_cast<void*>(spoutPtr);
}

interop::TextureSender::~TextureSender() {
	delete m_spout;
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

void interop::TextureSender::sendTexture(uint textureHandle, uint width, uint height) {
	if (width == 0 || height == 0)
		return;

	if (!m_sender)
		return;

	if (textureHandle == 0)
		return;

	if (m_width != width || m_height != height)
	{
		m_width = width;
		m_height = height;

		m_spout->UpdateSender(m_name.c_str(), m_width, m_height);
	}

	m_spout->SendTexture(textureHandle, GL_TEXTURE_2D, m_width, m_height);
}

