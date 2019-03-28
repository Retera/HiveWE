#pragma once

class AnimatedMesh : public Resource {
public:
	struct MeshEntry {
		int vertices = 0;
		int indices = 0;
		int base_vertex = 0;
		int base_index = 0;

		int material_id = 0;
		bool visible = true;

		mdx::GeosetAnimation* geoset_anim; // can be nullptr, often
		// below vectors are per-instance:
		std::vector<float> geoset_anim_alphas;
		std::vector<glm::vec3> geoset_anim_colors;
		std::vector<float> layer_alphas;
	};

	std::shared_ptr<mdx::MDX> model;

	std::vector<MeshEntry> entries;
	bool has_mesh; // ToDo remove when added support for meshless

	std::map<std::string, Animation> animations;

	GLuint vertex_buffer;
	GLuint uv_buffer;
	GLuint normal_buffer;
	GLuint index_buffer;
	GLuint instance_buffer;
	GLuint retera_vertex_group_buffer;
	GLuint retera_node_buffer;
	GLuint retera_groups_buffer;
	GLuint retera_node_buffer_texture;
	GLuint retera_groups_buffer_texture;
	GLuint layer_alpha;
	GLuint geoset_color;

	std::vector<uint> vertex_groups;
	std::vector<glm::uvec4> group_indexing_lookups;

	fs::path path;

	std::vector<std::shared_ptr<GPUTexture>> textures;
	std::shared_ptr<mdx::MTLS> mtls;
	std::shared_ptr<Shader> shader; // ToDo only needed one for class
	std::vector<std::shared_ptr<SkeletalModelInstance>> render_jobs;
	std::vector<glm::mat4> render_matrices;
	std::vector<glm::mat4> instance_bone_matrices;

	static constexpr const char* name = "AnimatedMesh";


	explicit AnimatedMesh(const fs::path& path);
	virtual ~AnimatedMesh();

	void render_queue(const std::shared_ptr<SkeletalModelInstance>& mvp);
	void render();
};