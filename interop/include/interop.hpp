
#pragma once

#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <memory>

namespace interop {

using uint = unsigned int;

	struct glFramebuffer {
		void init(uint width = 1, uint height = 1);
		void destroy();

		void bind();
		void unbind();
		void resizeTexture(uint width, uint height);
		void blitTexture();

		uint m_glFbo = 0;
		uint m_glTextureRGBA8 = 0;
		uint m_glTextureDepth = 0;
		int m_width = 0, m_height = 0;

		int m_previousFramebuffer[2] = {0};
		int m_previousViewport[4] = {0};
	};

	struct TextureSender {
		TextureSender();
		~TextureSender();

		void init(std::string name, uint width = 1, uint height = 1);
		void destroy();
		void sendTexture(uint texture, uint width, uint height);
		void sendTexture(glFramebuffer& fb);

		std::string m_name = "";
		uint m_width = 0;
		uint m_height = 0;
		std::shared_ptr<void> m_sender;
	};

	struct DataSender {
		DataSender();
		~DataSender();

		void start(const std::string& networkAddress, const std::string& filterName = ""); // address following zmq conventions, e.g. "tcp://localhost:1234"
		void stop();

		bool sendData(std::string const& v);
		bool sendData(std::string const& filterName, std::string const& v);

		template <typename DataType>
		bool sendData(std::string const& filterName, DataType const& v);

		std::shared_ptr<void> m_sender;
		std::string m_filterName;
	};

	struct DataReceiver {
		~DataReceiver();

		void start(const std::string& networkAddress, const std::string& filterName); // address following zmq conventions, e.g. "tcp://localhost:1234"
		void stop();

		template <typename Datatype>
		bool getData(Datatype& v);

		std::string m_msgData = "";
		std::string m_msgDataCopy = "";
		bool getDataCopy();
		std::thread m_thread;
		std::mutex m_mutex;
		std::atomic<bool> m_threadRunning = false;
		std::atomic_flag m_newDataFlag;
	};


	// all vectors, matrices and quaternions follow OpenGL and GLM conventions
	// in the sense that the data can directly be passed to GL and GLM functions

	// column vector, as in glm
	struct vec4 {
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
		float w = 0.0f;
	};
	vec4 operator+(vec4 const& lhs, vec4 const& rhs);
	vec4 operator-(vec4 const& lhs, vec4 const& rhs);
	vec4 operator*(vec4 const& lhs, vec4 const& rhs);
	vec4 operator*(vec4 const& v, const float s);
	vec4 operator*(const float s, const vec4 v);

	// column major matrix built from column vectors: (v1, v2, v3, v4)
	struct mat4 {
		vec4 data[4] = {
			vec4{1.0f, 0.0f, 0.0f, 0.0f},
			vec4{0.0f, 1.0f, 0.0f, 0.0f},
			vec4{0.0f, 0.0f, 1.0f, 0.0f},
			vec4{0.0f, 0.0f, 0.0f, 1.0f},
		};
	};

	struct CameraView {
		vec4 eyePos;
		vec4 lookAtPos;
		vec4 camUpDir;
	};

	struct CameraProjection {
		float fieldOfViewY_rad = 1.0f; // vertical field of view in radians 
		float nearClipPlane = 0.1f; // distance of near clipping plane
		float farClipPlane = 1.0f;

		float aspect = 1.0f; // aspect ratio, width divided by height
		uint pixelWidth = 1; // width of camera area in pixels, i.e. framebuffer texture size
		uint pixelHeight = 1;
	};

	struct CameraConfiguration {
		CameraView viewParameters;
		CameraProjection projectionParameters;

		// the matrices hold the respective low-level camera transformations as read from unity
		// the matrices may be from the VR SDK and thus better suited for VR stereo rendering than the explicit camera parameters above
		mat4 viewMatrix;
		mat4 projectionMatrix;
	};

	struct StereoCameraConfiguration {
		float stereoConvergence = 1.0f; // distance to point where left and right eye converge
		float stereoSeparation = 1.0f; // distance between virtual eyes

		CameraConfiguration cameraLeftEye;
		CameraConfiguration cameraRightEye;
	};

	// to be applied as model matrix (or equivalent) to the dataset before rendering
	struct ModelPose {
		vec4 translation;
		vec4 scale;
		vec4 rotation_axis_angle_rad; // angle in radians, as expected by GLM

		mat4 modelMatrix;
	};

	// renderer receives this config from unity
	struct DatasetRenderConfiguration {
		StereoCameraConfiguration stereoCamera;
		ModelPose modelTransform;
	};

	// axis-aligned bounding box of rendered data 
	// renderer sends bounding box (in world space) of its geometry to unity
	// bounding box is used as reference for rendering and interaction with dataset
	struct BoundingBoxCorners {
		vec4 min;
		vec4 max;

		// returns vector V such that 'V + x' moves vector x to centered bbox coordinates
		vec4 getCenteringVector() const {
			return -1.0f * (min + ((max - min) * 0.5f));
		}

		// returns transformation matrix to center bounding box (and data set) at (0,0,0)
		// the convention between Unity and our renderer is that the data set is centered at (and can be rotated around) zero
		mat4 getCenteringTransform() const {
			mat4 translate; // identity

			translate.data[3] = this->getCenteringVector();
			translate.data[3].w = 1.0f;

			return translate;
		}

		// Unity expects a bounding box centered at zero
		BoundingBoxCorners getCenteredBoundingBox() const {
			auto transform = this->getCenteringTransform();
			transform.data[3].w = 0.0f;

			return BoundingBoxCorners{
				this->min + transform.data[3],
				this->max + transform.data[3]};
		}
	};

// converter functions to fill inteorp struct from Json string received by DataReceiver
#define make_dataGet(DataTypeName) \
template <> \
bool DataReceiver::getData<DataTypeName>(DataTypeName& v);

make_dataGet(BoundingBoxCorners)
make_dataGet(DatasetRenderConfiguration)
make_dataGet(ModelPose)
make_dataGet(StereoCameraConfiguration)
make_dataGet(CameraConfiguration)
make_dataGet(CameraProjection)
make_dataGet(CameraView)
make_dataGet(mat4)
make_dataGet(vec4)
#undef make_dataGet


#define make_sendData(DataTypeName) \
template <> \
bool interop::DataSender::sendData<DataTypeName>(std::string const& filterName, DataTypeName const& v);

make_sendData(BoundingBoxCorners)
make_sendData(DatasetRenderConfiguration)
make_sendData(ModelPose)
make_sendData(StereoCameraConfiguration)
make_sendData(CameraConfiguration)
make_sendData(CameraProjection)
make_sendData(CameraView)
make_sendData(mat4)
make_sendData(vec4)
#undef make_sendData

}

