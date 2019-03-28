#include "stdafx.h"

QOpenGLFunctions_4_5_Core* gl;
Shapes shapes;

const glm::vec3 TRANSLATION_IDENTITY(0);
const glm::vec3 SCALE_IDENTITY(1);
const glm::quat ROTATION_IDENTITY(1, 0, 0, 0);

void Shapes::init() {
	gl->glCreateBuffers(1, &vertex_buffer);
	gl->glNamedBufferData(vertex_buffer, quad_vertices.size() * sizeof(glm::vec2), quad_vertices.data(), GL_STATIC_DRAW);

	gl->glCreateBuffers(1, &index_buffer);
	gl->glNamedBufferData(index_buffer, quad_indices.size() * sizeof(unsigned int) * 3, quad_indices.data(), GL_STATIC_DRAW);
}

std::string string_replaced(const std::string& source, const std::string& from, const std::string& to) {
	std::string new_string;
	new_string.reserve(source.length());  // avoids a few memory allocations

	size_t lastPos = 0;
	size_t findPos;

	while (std::string::npos != (findPos = source.find(from, lastPos))) {
		new_string.append(source, lastPos, findPos - lastPos);
		new_string += to;
		lastPos = findPos + from.length();
	}

	// Care for the rest after last occurrence
	new_string += source.substr(lastPos);

	return new_string;
}

std::vector<std::string> split(const std::string& string, const char delimiter) {
	std::vector<std::string> elems;
	std::stringstream ss(string);

	std::string item;
	while (std::getline(ss, item, delimiter)) {
		elems.push_back(item);
	}
	return elems;
}

std::vector<std::string_view> split_new(const std::string& string, const char delimiter) {
	std::vector<std::string_view> elems;

	int last_position = 0;
	for (int i = 0; i < string.length(); i++) {
		if (string[i] == delimiter) {
			elems.emplace_back(string.c_str() + last_position, i - last_position);
			last_position = i;
		}
	}

	if (elems.empty()) {
		elems.emplace_back(string);
	}

	return elems;
}

bool is_number(const std::string& s) {
	return !s.empty() && std::find_if(s.begin(), s.end(), [](char c) { return !std::isdigit(c); }) == s.end();
}

GLuint compile_shader(const fs::path& vertex_shader, const fs::path& fragment_shader) {
	char buffer[512];
	GLint status;

	std::string vertex_source = read_text_file(vertex_shader.string());
	std::string fragment_source = read_text_file(fragment_shader.string());

	const GLuint vertex = gl->glCreateShader(GL_VERTEX_SHADER);
	const GLuint fragment = gl->glCreateShader(GL_FRAGMENT_SHADER);

	// Vertex Shader
	const char* source = vertex_source.c_str();
	gl->glShaderSource(vertex, 1, &source, nullptr);
	gl->glCompileShader(vertex);


	gl->glGetShaderiv(vertex, GL_COMPILE_STATUS, &status);
	gl->glGetShaderInfoLog(vertex, 512, nullptr, buffer);
	if (!status) {
		std::cout << buffer << std::endl;
	}

	// Fragment Shader
	source = fragment_source.c_str();
	gl->glShaderSource(fragment, 1, &source, nullptr);
	gl->glCompileShader(fragment);

	gl->glGetShaderiv(fragment, GL_COMPILE_STATUS, &status);
	gl->glGetShaderInfoLog(fragment, 512, nullptr, buffer);
	if (!status) {
		std::cout << buffer << std::endl;
	}

	// Link
	const GLuint shader = gl->glCreateProgram();
	gl->glAttachShader(shader, vertex);
	gl->glAttachShader(shader, fragment);
	gl->glLinkProgram(shader);

	gl->glGetProgramiv(shader, GL_LINK_STATUS, &status);
	if (!status) {
		gl->glGetProgramInfoLog(shader, 512, nullptr, buffer);
		std::cout << buffer << std::endl;
	}

	gl->glDeleteShader(vertex);
	gl->glDeleteShader(fragment);

	return shader;
}

std::string read_text_file(const std::string& path) {
	std::ifstream textfile(path.c_str());
	std::string line;
	std::string text;

	if (!textfile.is_open())
		return "";

	while (getline(textfile, line)) {
		text += line + "\n";
	}

	return text;
}

fs::path find_warcraft_directory() {
	QSettings settings;
	if (settings.contains("warcraftDirectory")) {
		return settings.value("warcraftDirectory").toString().toStdString();
	} else if (fs::exists("C:/Program Files (x86)/Warcraft III")) {
		return "C:/Program Files (x86)/Warcraft III";
	} else if (fs::exists("D:/Program Files (x86)/Warcraft III")) {
		return "D:/Program Files (x86)/Warcraft III";
	} else {
		return "";
	}
}

void load_modification_table(BinaryReader& reader, slk::SLK& base_data, slk::SLK& meta_data, const bool modification, bool optional_ints) {
	const uint32_t objects = reader.read<uint32_t>();
	for (size_t i = 0; i < objects; i++) {
		const std::string original_id = reader.read_string(4);
		const std::string modified_id = reader.read_string(4);

		if (modification) {
			base_data.copy_row(original_id, modified_id);
		}

		const uint32_t modifications = reader.read<uint32_t>();

		for (size_t j = 0; j < modifications; j++) {
			const std::string modification_id = reader.read_string(4);
			const uint32_t type = reader.read<uint32_t>();

			if (optional_ints) {
				// ToDo dont Skip optional ints
				reader.advance(8);
			}

			const std::string column_header = meta_data.data("field", modification_id);

			std::string data;
			switch (type) {
				case 0:
					data = std::to_string(reader.read<int>());
					break;
				case 1:
				case 2:
					data = std::to_string(reader.read<float>());
					break;
				case 3:
					data = reader.read_c_string();
					break;
				default: 
					std::cout << "Unknown data type " << type << " while loading modification table.";
			}
			reader.position += 4;
			if (modification) {
				base_data.set_shadow_data(column_header, modified_id, data);
			} else {
				base_data.set_shadow_data(column_header, original_id, data);
			}
		}
	}
}

QIcon ground_texture_to_icon(uint8_t* data, const int width, const int height) {
	QImage temp_image = QImage(data, width, height, QImage::Format::Format_ARGB32);
	const int size = height / 4;

	auto pix = QPixmap::fromImage(temp_image.copy(0, 0, size, size));

	QIcon icon;
	icon.addPixmap(pix, QIcon::Normal, QIcon::Off);

	QPainter painter(&pix);
	painter.fillRect(0, 0, size, size, QColor(255, 255, 0, 64));
	painter.end();

	icon.addPixmap(pix, QIcon::Normal, QIcon::On);

	return icon;
}

QIcon texture_to_icon(fs::path path) {
	auto tex = resource_manager.load<Texture>(path);
	QImage temp_image = QImage(tex->data.data(), tex->width, tex->height, QImage::Format::Format_ARGB32);
	auto pix = QPixmap::fromImage(temp_image);
	return QIcon(pix);
}
//#define TEST_MODE 1
void fromRotationTranslationScaleOrigin(glm::quat & q, glm::vec3 & v, glm::vec3 & s, glm::mat4 & out, glm::vec3 & pivot)
{
	// ho tom bambadil
	// tom bombadillo
	// Retera was here
	// (This code is copied from the holy bible)
	float x = q.x;
	float y = q.y;
	float z = q.z;
	float w = q.w;
	float x2 = x + x;
	float y2 = y + y;
	float z2 = z + z;
	float xx = x * x2;
	float xy = x * y2;
	float xz = x * z2;
	float yy = y * y2;
	float yz = y * z2;
	float zz = z * z2;
	float wx = w * x2;
	float wy = w * y2;
	float wz = w * z2;
	float sx = s.x;
	float sy = s.y;
	float sz = s.z;
#ifdef TEST_MODE
	out[0][0] = (1 - (yy + zz)) * sx; 
	out[1][0] = (xy + wz) * sx;
	out[2][0] = (xz - wy) * sx;
	out[3][0] = 0;
	out[0][1] = (xy - wz) * sy;
	out[1][1] = (1 - (xx + zz)) * sy;
	out[2][1] = (yz + wx) * sy;
	out[3][1] = 0;
	out[0][2] = (xz + wy) * sz;
	out[1][2] = (yz - wx) * sz;
	out[2][2] = (1 - (xx + yy)) * sz;
	out[3][2] = 0;
	out[0][3] = v.x + pivot.x - (out[0][0] * pivot.x + out[0][1] * pivot.y + out[0][2] * pivot.z);
	out[1][3] = v.y + pivot.y - (out[1][0] * pivot.x + out[1][1] * pivot.y + out[1][2] * pivot.z);
	out[2][3] = v.z + pivot.z - (out[2][0] * pivot.x + out[2][1] * pivot.y + out[2][2] * pivot.z);
	out[3][3] = 1;
#else
	out[0][0] = (1 - (yy + zz)) * sx;
	out[0][1] = (xy + wz) * sx;
	out[0][2] = (xz - wy) * sx;
	out[0][3] = 0;
	out[1][0] = (xy - wz) * sy;
	out[1][1] = (1 - (xx + zz)) * sy;
	out[1][2] = (yz + wx) * sy;
	out[1][3] = 0;
	out[2][0] = (xz + wy) * sz;
	out[2][1] = (yz - wx) * sz;
	out[2][2] = (1 - (xx + yy)) * sz;
	out[2][3] = 0;
	out[3][0] = v.x + pivot.x - (out[0][0] * pivot.x + out[1][0] * pivot.y + out[2][0] * pivot.z);
	out[3][1] = v.y + pivot.y - (out[0][1] * pivot.x + out[1][1] * pivot.y + out[2][1] * pivot.z);
	out[3][2] = v.z + pivot.z - (out[0][2] * pivot.x + out[1][2] * pivot.y + out[2][2] * pivot.z);
	out[3][3] = 1;
#endif
}
void quatMul(const glm::quat & left, const glm::quat & right, glm::quat dest)
{
	dest.x = left.x * right.w + left.w * right.x + left.y * right.z - left.z * right.y;
	dest.y = left.y * right.w + left.w * right.y + left.z * right.x - left.x * right.z;
	dest.z = left.z * right.w + left.w * right.z + left.x * right.y - left.y * right.x;
	dest.w = left.w * right.w - left.x * right.x - left.y * right.y - left.z * right.z;
}
;

float lerp(float a, float b, float t) {
	return a + t * (b - a);
}
float hermite(float a, float aOutTan, float bInTan, float b, float t) {
	float factorTimes2 = t * t;
	float factor1 = factorTimes2 * (2 * t - 3) + 1;
	float factor2 = factorTimes2 * (t - 2) + t;
	float factor3 = factorTimes2 * (t - 1);
	float factor4 = factorTimes2 * (3 - 2 * t);
	return (a * factor1) + (aOutTan * factor2) + (bInTan * factor3) + (b * factor4);
}
float bezier(float a, float aOutTan, float bInTan, float b, float t) {
	float invt = 1 - t;
	float factorSquared = t * t;
	float inverseFactorSquared = invt * invt;
	float factor1 = inverseFactorSquared * invt;
	float factor2 = 3 * t * inverseFactorSquared;
	float factor3 = 3 * factorSquared * invt;
	float factor4 = factorSquared * t;
	return (a * factor1) + (aOutTan * factor2) + (bInTan * factor3) + (b * factor4);
}

template<typename T>
inline void interpolate(T & out, const T * start, const T * outTan, const T * inTan, const T * end, float t, int interpolationType)
{
	out = *start;
}
template <>
void interpolate<float>(float& out, const float* start, const float* outTan, const float* inTan, const float* end, float t, int interpolationType) {
	switch (interpolationType) {
	case 0:
		out = *start;
		return;
	case 1: // LINEAR
		out = lerp(*start, *end, t);
		return;
	case 2: // HERMITE
		out = hermite(*start, *outTan, *inTan, *end, t);
		return;
	case 3: // BEZIER
		out = bezier(*start, *outTan, *inTan, *end, t);
		return;
	}
}
template <>
void interpolate<glm::vec3>(glm::vec3& out, const glm::vec3* start, const glm::vec3* outTan, const glm::vec3* inTan, const glm::vec3* end, float t, int interpolationType) {
	switch (interpolationType) {
	case 0:
		out = *start;
		return;
	case 1: // LINEAR
		out.x = lerp(start->x, end->x, t);
		out.y = lerp(start->y, end->y, t);
		out.z = lerp(start->z, end->z, t);
		return;
	case 2: // HERMITE
		out.x = hermite(start->x, outTan->x, inTan->x, end->x, t);
		out.y = hermite(start->y, outTan->y, inTan->y, end->y, t);
		out.z = hermite(start->z, outTan->z, inTan->z, end->z, t);
		return;
	case 3: // BEZIER
		out.x = bezier(start->x, outTan->x, inTan->x, end->x, t);
		out.y = bezier(start->y, outTan->y, inTan->y, end->y, t);
		out.z = bezier(start->z, outTan->z, inTan->z, end->z, t);
		return;
	}
}
template <>
void interpolate<glm::quat>(glm::quat& out, const glm::quat* start, const glm::quat* outTan, const glm::quat* inTan, const glm::quat* end, float t, int interpolationType) {
	switch (interpolationType) {
	case 0:
		out = *start;
		return;
	case 1: // LINEAR
		//slerp(out, *start, *end, t);
		out = glm::slerp(*start, *end, t);
		return;
	case 2: // HERMITE
		// GLM uses both {x, y, z, w} and {w, x, y, z} convention, in different places, sometimes.
		// Their squad is {w, x, y, z} but we are elsewhere using {x, y, z, w}, so we will
		// continue using the copy of the Matrix Eater "ghostwolfSquad" for now.
		//out = glm::squad(*start, *outTan, *inTan, *end, t);
		ghostwolfSquad(out, start, outTan, inTan, end, t);
		return;
	case 3: // BEZIER
		// GLM uses both {x, y, z, w} and {w, x, y, z} convention, in different places, sometimes.
		// Their squad is {w, x, y, z} but we are elsewhere using {x, y, z, w}, so we will
		// continue using the copy of the Matrix Eater "ghostwolfSquad" for now.
		//out = glm::squad(*start, *outTan, *inTan, *end, t);
		ghostwolfSquad(out, start, outTan, inTan, end, t);
		return;
	default:
		break;
	}
}
template <>
void interpolate<uint32_t>(uint32_t& out, const uint32_t* start, const uint32_t* outTan, const uint32_t* inTan, const uint32_t* end, float t, int interpolationType) {
	out = *start;
}

void slerp(glm::quat& out, const glm::quat& startingValue, const glm::quat& endingValue, float interpolationFactor) {
	float ax = startingValue.x, ay = startingValue.y, az = startingValue.z, aw = startingValue.w;
	float bx = endingValue.x, by = endingValue.y, bz = endingValue.z, bw = endingValue.w;
	float omega;
	float cosom;
	float sinom, scale0, scale1;
	// calc cosine
	cosom = ax * bx + ay * by + az * bz + aw * bw;
	// adjust signs (if necessary)
	if (cosom < 0) {
		cosom = -cosom;
		bx = -bx;
		by = -by;
		bz = -bz;
		bw = -bw;
	}
	// calculate coefficients
	if ((1.0 - cosom) > 0.000001) {
		//standard case (slerp)
		omega = acos(cosom);
		sinom = sin(omega);
		scale0 = sin((1.0 - interpolationFactor) * omega) / sinom;
		scale1 = sin(interpolationFactor * omega) / sinom;
	}
	else {
		// "from" and "to" quaternions are very close
		// ... so we can do a linear interpolation
		scale0 = 1.0 - interpolationFactor;
		scale1 = interpolationFactor;
	}

	out.x = scale0 * ax + scale1 * bx;
	out.y = scale0 * ay + scale1 * by;
	out.z = scale0 * az + scale1 * bz;
	out.w = scale0 * aw + scale1 * bw;

	// Super slow and generally not needed.
	// quat.normalize(out, out);
}
void ghostwolfSquad(glm::quat& out, const glm::quat* a, const glm::quat* aOutTan, const glm::quat* bInTan, const glm::quat* b, float t) {
	glm::quat temp1;
	glm::quat temp2;
	slerp(temp1, *a, *b, t);
	slerp(temp2, *aOutTan, *bInTan, t);
	slerp(out, temp1, temp2, 2 * t*(1 - t));
}

float clampValue(float a, float min, float max) {
	if (a < min) {
		return min;
	}
	else if (a > max) {
		return max;
	}
	return a;
}

glm::quat safeQuatLookAt(
	glm::vec3 const& lookFrom,
	glm::vec3 const& lookTo,
	glm::vec3 const& up,
	glm::vec3 const& alternativeUp)
{
	glm::vec3  direction = lookTo - lookFrom;
	float      directionLength = glm::length(direction);

	// Check if the direction is valid; Also deals with NaN
	if (!(directionLength > 0.0001))
		return glm::quat(1, 0, 0, 0); // Just return identity

	// Normalize direction
	direction /= directionLength;

	// Is the normal up (nearly) parallel to direction?
	if (glm::abs(glm::dot(direction, up)) > .9999f) {
		// Use alternative up
		return glm::quatLookAt(direction, alternativeUp);
	}
	else {
		return glm::quatLookAt(direction, up);
	}
}