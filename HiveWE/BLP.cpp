#include "stdafx.h"

namespace blp {
	uint8_t* BLP::load(const std::string& path) {
		BinaryReader reader(hierarchy.open_file(path));

		std::string version = reader.readString(4);

		if (version != "BLP1") {
			std::cout << "Could not load file as it is not a BLP1 file. Maybe it is BLP0 or BLP2." << std::endl;
		}

		uint32_t content = reader.read<uint32_t>();
		uint32_t alpha_bits = reader.read<uint32_t>();

		uint32_t width = reader.read<uint32_t>();
		uint32_t height = reader.read<uint32_t>();

		// Mipmaplocator
		uint32_t extra = reader.read<uint32_t>();
		uint32_t has_mipmaps = reader.read<uint32_t>();
		std::vector<uint32_t> mipmap_offsets = reader.readVector<uint32_t>(16);
		std::vector<uint32_t> mipmap_sizes = reader.readVector<uint32_t>(16);

		uint32_t jpegHeaderSize = reader.read<uint32_t>();
		std::vector<uint8_t> header = reader.readVector<uint8_t>(jpegHeaderSize);
		reader.position = mipmap_offsets[0];
		std::vector<uint8_t> mipmap = reader.readVector<uint8_t>(mipmap_sizes[0]);
		header.insert(header.end(), mipmap.begin(), mipmap.end());

		unsigned char* buffer;
		if (content == CONTENT_JPEG) {
			// Decode JPEG content
			tjhandle handle = tjInitDecompress();
			buffer = new unsigned char[width * height * tjPixelSize[TJPF_CMYK]];
			tjDecompress2(handle, &header[0], header.size(), buffer, width, 0, height, TJPF_CMYK, 0);
			tjDestroy(handle);

			// BGRA to RGBA
			for (size_t i = 0; i < width * height; i++) {
				unsigned char temp = buffer[i * 4];
				buffer[i * 4] = buffer[i * 4 + 2];
				buffer[i * 4 + 2] = temp;
			}
		}

		return buffer;
	}
}
