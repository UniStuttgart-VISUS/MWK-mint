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

	zmq::context_t g_zmqContext{1};
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

void interop::TextureSender::sendTexture(glFramebuffer& fb) {
	this->sendTexture(fb.m_glTextureRGBA8, fb.m_width, fb.m_height);
}

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
			std::string identity{"InteropLib"};
			socket.setsockopt(ZMQ_IDENTITY, identity.data(), identity.size());
			//socket.setsockopt(ZMQ_CONFLATE, 1); // keep only most recent message, drop old ones
			socket.setsockopt(ZMQ_SUBSCRIBE, filter.data(), filter.size()); // only receive messages with prefix given by filterName
			socket.setsockopt(ZMQ_RCVTIMEO, 1000 /*ms*/); // timeout after not receiving messages for 1 second
			socket.connect(networkAddress);
		}
		catch (std::exception& e){
			std::cout << "InteropLib: connecting zmq socket failed: " << e.what() << std::endl;
			return;
		}
		threadRunning.store(true);

		while (threadRunning) {

			zmq::message_t address_msg;
			zmq::message_t content_msg;

			if (!filter.empty())
				socket.recv(&address_msg);
			socket.recv(&content_msg);

			if ((filter.size() && !address_msg.size())
				|| (filter.size() && filter.compare(0, address_msg.size(), static_cast<const char*>(address_msg.data()), address_msg.size()) != 0))
			{
				//std::cout << "filter: \"" << filter << "\"" << std::endl;
				//std::cout << "recevd: \"" << std::string{static_cast<const char*>(address_msg.data()), address_msg.size()}  << "\"" << std::endl;
				continue;
			}

			// store message content for user to retrieve+parse
			if(content_msg.size() > 0) {
				//std::cout << "zmq received address: " << std::string(static_cast<char*>(address_msg.data()), address_msg.size()) << std::endl;
				//std::cout << "zmq received content: " << std::string(static_cast<char*>(content_msg.data()), content_msg.size()) << std::endl;

				std::lock_guard<std::mutex> lock{mutex};
				msgData.assign(static_cast<char*>(content_msg.data()), content_msg.size());
				newDataFlag.clear(std::memory_order_release); // Sets false. All writes in the current thread are visible in other threads that acquire the same atomic variable
			}
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
#define write(name) {# name, v. ## name}
#define read(name) j.at(# name).get_to(v. ## name);

// Add json converter functions as follows: 
//
// void to_json(json& j, const YourOwnType& v) {
// 	j = json{
// 		write(member1),
// 		...
// 		write(memberN)
// 	};
// }
// void from_json(const json& j, YourOwnType& v) {
// 	read(member1);
// 	...
// 	read(memberN);
// }
namespace interop {
	void to_json(json& j, const vec4& v) {
		j = json{
			write(x),
			write(y),
			write(z),
			write(w)
		};
	}
	void from_json(const json& j, vec4& v) {
		read(x);
		read(y);
		read(z);
		read(w);
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
#define rm(name,value) j.at(# name).get_to(## value);
	void from_json(const json& j, mat4& v) {
		rm("e00", v.data[0].x); rm("e01", v.data[1].x); rm("e02", v.data[2].x); rm("e03", v.data[3].x);
		rm("e10", v.data[0].y); rm("e11", v.data[1].y); rm("e12", v.data[2].y); rm("e13", v.data[3].y);
		rm("e20", v.data[0].z); rm("e21", v.data[1].z); rm("e22", v.data[2].z); rm("e23", v.data[3].z); 
		rm("e30", v.data[0].w); rm("e31", v.data[1].w); rm("e32", v.data[2].w); rm("e33", v.data[3].w);
	}

	void to_json(json& j, const CameraView& v) {
		j = json{
			write(eyePos),
			write(lookAtPos),
			write(camUp)
		};
	}
	void from_json(const json& j, CameraView& v) {
		read(eyePos);
		read(lookAtPos);
		read(camUp);
	}

	void to_json(json& j, const CameraProjection& v) {
		j = json{
			write(fieldOfViewY_deg),
			write(nearClipPlane),
			write(farClipPlane),
			write(aspect),
			write(pixelWidth),
			write(pixelHeight)
		};
	}
	void from_json(const json& j, CameraProjection& v) {
		read(fieldOfViewY_deg);
		read(nearClipPlane);
		read(farClipPlane);
		read(aspect);
		read(pixelWidth);
		read(pixelHeight);
	}

	void to_json(json& j, const CameraConfiguration& v) {
		j = json{
			write(viewParameters),
			write(projectionParameters),
			write(viewMatrix),
			write(projectionMatrix)
		};
	}
	void from_json(const json& j, CameraConfiguration& v) {
		read(viewParameters);
		read(projectionParameters);
		read(viewMatrix);
		read(projectionMatrix);
	}

	void to_json(json& j, const StereoCameraConfiguration& v) {
		j = json{
			write(stereoConvergence),
			write(stereoSeparation),
			write(cameraLeftEye),
			write(cameraRightEye)
		};
	}
	void from_json(const json& j, StereoCameraConfiguration& v) {
		read(stereoConvergence);
		read(stereoSeparation);
		read(cameraLeftEye);
		read(cameraRightEye);
	}

	void to_json(json& j, const BoundingBoxCorners& v) {
		j = json{
			write(min),
			write(max)
		};
	}
	void from_json(const json& j, BoundingBoxCorners& v) {
		read(min);
		read(max);
	}

	void to_json(json& j, const ModelPose& v) {
		j = json{
			write(translation),
			write(scale),
			write(rotation_quaternion),
			write(modelMatrix)
		};
	}
	void from_json(const json& j, ModelPose& v) {
		read(translation);
		read(scale);
		read(rotation_quaternion);
		read(modelMatrix);
	}

	void to_json(json& j, const DatasetRenderConfiguration& v) {
		j = json{
			write(stereoCamera),
			write(modelTransform)
		};
	}
	void from_json(const json& j, DatasetRenderConfiguration& v) {
		read(stereoCamera);
		read(modelTransform);
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

make_dataGet(interop::BoundingBoxCorners)
make_dataGet(interop::DatasetRenderConfiguration)
make_dataGet(interop::ModelPose)
make_dataGet(interop::StereoCameraConfiguration)
make_dataGet(interop::CameraConfiguration)
make_dataGet(interop::CameraProjection)
make_dataGet(interop::CameraView)
make_dataGet(interop::mat4)
make_dataGet(interop::vec4)
