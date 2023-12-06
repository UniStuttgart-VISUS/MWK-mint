
#pragma once

#include <memory>
#include <string>
#include <optional>
#include <atomic>
#include <thread>
#include <unordered_map>
#include <mutex>
#include <utility>

namespace interop {

	using uint = unsigned int;

	enum class Role {
		Steering,
		Rendering,
	};

	enum class DataProtocol {
		IPC,
		TCP,
	};

	enum class ImageProtocol {
		// according to spout docs pdf
		GPU = 0, // GPU texture sharing
		CPU = 1, // CPU texture sharing
		MemShare = 2, // memory sharing
	};

	enum class ImageType {
		LeftEye,
		RightEye,
		SingleStereo,
	};

	void init(Role r, DataProtocol dp = DataProtocol::IPC, ImageProtocol ip = ImageProtocol::GPU);

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

		int m_previousFramebuffer[2] = { 0 };
		int m_previousViewport[4] = { 0 };
	};

	struct TextureSender {
		TextureSender();
		~TextureSender();

		void init(std::string name, uint width = 1, uint height = 1);
		void init(ImageType type, std::string name = "", uint width = 1, uint height = 1);
		void destroy();
		bool resize(uint width, uint height);
		void send(uint texture, uint width, uint height);
		void send(glFramebuffer& fb);

		std::string m_name = "";
		uint m_width = 0;
		uint m_height = 0;
		std::shared_ptr<void> m_sender;
	};

	struct TextureReceiver {
		TextureReceiver();
		~TextureReceiver();

		void init(std::string name);
		void init(ImageType type, std::string name = "");

		void destroy();
		bool receive();

		std::string m_name = "";
		uint m_width = 0;
		uint m_height = 0;
		std::shared_ptr<void> m_receiver;
		uint m_texture_handle = 0;
		uint m_source_fbo = 0;
		uint m_target_fbo = 0;
	};

	// packs color and depth textures into one huge texture before sharing texture
	// data with other processes input widht/height are for the original size of the
	// input textures
	struct StereoTextureSender {
		StereoTextureSender();
		~StereoTextureSender();

		void init(std::string name = "", const uint width = 1, const uint height = 1);

		void destroy();
		void send(glFramebuffer& fb_left, glFramebuffer& fb_right,
			const uint width, const uint height,
			const uint meta_data = 0, const uint meta_data_2 = 0);
		void send(const uint color_left, const uint color_right,
			const uint depth_left, const uint depth_right,
			const uint width, const uint height,
			const uint meta_data = 0, const uint meta_data_2 = 0);

		std::string m_name = "";
		uint m_width = 0;
		uint m_height = 0;
		uint m_hugeWidth = 0;
		uint m_hugeHeight = 0;
		glFramebuffer
			m_hugeFbo; // holds huge texture for sending 2x color, 2x depth at once
		TextureSender m_hugeTextureSender;
		void makeHugeTexture(const uint originalWidth, const uint originalHeight);
		uint m_shader = 0;
		uint m_vao = 0;
		uint m_uniform_locations[7] = { 0 };
		void initGLresources();
		void destroyGLresources();
		void blitTextures(const uint color_left_texture,
			const uint color_right_texture,
			const uint depth_left_texture,
			const uint depth_right_texture, const uint width,
			const uint height, const uint meta_data, const uint meta_data_2 = 0);

		static_assert(sizeof(uint) == 4, "unigned int expected to be 4 bytes");
	};

	enum class Endpoint {
		Bind,
		Connect,
	};

	struct DataSender {
		DataSender();
		explicit DataSender(std::string const& address);
		~DataSender();

		void start(Endpoint socket_type = Endpoint::Bind);
		void stop();

		bool send_raw(std::string const& v, std::string const& filterName);

		template <typename DataType>
		bool send(DataType const& v);

		template <typename DataType>
		bool send(DataType const& v, std::string const& filterName, std::optional<std::pair<std::string/*name*/, std::string/*value*/>> const& maybe_extra = std::nullopt);

		std::shared_ptr<void> m_sender;
		std::string m_address;
	};

	struct DataReceiver {
		DataReceiver();
		explicit DataReceiver(std::string const& address);
		~DataReceiver();

		bool start(const std::string& filterName = "", Endpoint socket_type = Endpoint::Connect);
		void stop();

		template <typename Datatype> bool receive(Datatype& v);
		template <typename Datatype> bool receive(Datatype& v, const std::string& filterName);
		template <typename Datatype> bool receive(Datatype& v, const std::string& filterName, std::optional<std::pair<std::string/*name*/, std::string/*value*/>>& maybe_extra);
		std::optional<std::string> receiveCopy(const std::string& filterName = "");

		//std::shared_ptr<void> m_receiver;
		std::string m_filterName;
		std::string m_address;

		std::thread m_worker;
		std::atomic_flag m_worker_signal = ATOMIC_FLAG_INIT;
		std::atomic<bool> m_worker_result{ false };

		std::unordered_map<std::string, std::string> m_messages;
		std::mutex m_mutex;
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
	vec4 operator+=(vec4& lhs, vec4 const& rhs);
	vec4 operator-(vec4 const& lhs, vec4 const& rhs);
	vec4 operator*(vec4 const& lhs, vec4 const& rhs);
	vec4 operator*(vec4 const& v, const float s);
	vec4 operator*(const float s, const vec4 v);
	bool operator==(const vec4& l, const vec4& r);
	bool operator!=(const vec4& l, const vec4& r);

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

	struct StereoCameraView {
		CameraView leftEyeView;
		CameraView rightEyeView;
	};

	struct StereoCameraViewRelative {
		CameraView leftEyeView;
		CameraView rightEyeView;
	};

	struct CameraProjection {
		float fieldOfViewY_rad = 1.0f; // vertical field of view in radians
		float nearClipPlane = 0.1f;    // distance of near clipping plane
		float farClipPlane = 1.0f;

		float aspect = 1.0f; // aspect ratio, width divided by height
		uint pixelWidth =
			1; // width of camera area in pixels, i.e. framebuffer texture size
		uint pixelHeight = 1;
	};

	struct CameraConfiguration {
		CameraView viewParameters;
		CameraProjection projectionParameters;

		// the matrices hold the respective low-level camera transformations as read
		// from unity the matrices may be from the VR SDK and thus better suited for
		// VR stereo rendering than the explicit camera parameters above
		mat4 viewMatrix;
		mat4 projectionMatrix;
	};

	struct StereoCameraConfiguration {
		float stereoConvergence =
			1.0f; // distance to point where left and right eye converge
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

		vec4 diagonal() const {
			return max - min;
		}

		// returns vector V such that 'V + x' moves vector x to centered bbox
		// coordinates
		vec4 getCenteringVector() const {
			return -1.0f * (min + ((max - min) * 0.5f));
		}

		// returns transformation matrix to center bounding box (and data set) at
		// (0,0,0) the convention between Unity and our renderer is that the data set
		// is centered at (and can be rotated around) zero
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

			return BoundingBoxCorners{ this->min + transform.data[3],
									  this->max + transform.data[3] };
		}
	};

	// converter functions to fill inteorp struct from Json string received by
	// DataReceiver
#define make_dataGet(DataTypeName)                                             \
  template <> bool DataReceiver::receive<DataTypeName>(\
	DataTypeName &v, \
	const std::string &filterName, \
 	std::optional<std::pair<std::string,std::string>>& maybe_extra );\
	\
	template <> bool DataReceiver::receive<DataTypeName>(\
		DataTypeName& v, \
		const std::string& filterName \
);

	make_dataGet(BoundingBoxCorners);
	make_dataGet(DatasetRenderConfiguration);
	make_dataGet(ModelPose);
	make_dataGet(StereoCameraConfiguration);
	make_dataGet(CameraConfiguration);
	make_dataGet(CameraProjection);
	make_dataGet(StereoCameraView);
	make_dataGet(StereoCameraViewRelative);
	make_dataGet(CameraView);
	make_dataGet(mat4);
	make_dataGet(vec4);

	make_dataGet(bool);
	make_dataGet(int);
	make_dataGet(unsigned int);
	make_dataGet(float);
	make_dataGet(double);
#undef make_dataGet

#define make_send(DataTypeName)                                            \
  template <>                                                                  \
  bool interop::DataSender::send<DataTypeName>(                            \
	DataTypeName const &v, \
	std::string const &filterName, \
 	std::optional<std::pair<std::string,std::string>> const& maybe_extra \
);

	make_send(BoundingBoxCorners);
	make_send(DatasetRenderConfiguration);
	make_send(ModelPose);
	make_send(StereoCameraConfiguration);
	make_send(CameraConfiguration);
	make_send(CameraProjection);
	make_send(StereoCameraView);
	make_send(StereoCameraViewRelative);
	make_send(CameraView);
	make_send(mat4);
	make_send(vec4);
#undef make_send

	template <typename DataType>
	std::string to_data_name(DataType const& v);

} // namespace interop

namespace mint = interop;

