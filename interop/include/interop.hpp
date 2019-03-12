
#pragma once

#include <string>

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

		std::string m_name = "";
		uint m_width = 0;
		uint m_height = 0;
		void* m_sender;
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

	// column major matrix built from column vectors: (v1, v2, v3, v4)
	struct mat4 {
		vec4 data[4];
	};

	struct CameraView {
		vec4 eyePos;
		vec4 lookAtPos;
		vec4 camUp;
	};

	struct CameraProjection {
		float fieldOfViewY_deg = 1.0f; // vertical field of view in degrees
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
	struct ModelConfiguration {
		vec4 translation;
		float scale = 1.0f;
		vec4 rotation_quaternion;

		mat4 modelMatrix;
	};

	// renderer receives this config from unity
	struct DatasetRenderConfiguration {
		StereoCameraConfiguration stereoCamera;
		ModelConfiguration modelTransform;
	};

	// renderer sends bounding box (in world space) for its geometry to unity
	// bounding box is used as reference for rendering and interaction with dataset
	struct BoundingBoxCorners {
		vec4 min;
		vec4 max;
	};
}
