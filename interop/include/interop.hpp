
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

}
