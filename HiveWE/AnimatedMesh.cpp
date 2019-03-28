#include "stdafx.h"

AnimatedMesh::AnimatedMesh(const fs::path & path)
{
	shader = resource_manager.load<Shader>({ "Data/Shaders/animated_mesh_instanced.vs", "Data/Shaders/animated_mesh_instanced.fs" });

	if (path.extension() == ".mdx" || path.extension() == ".MDX") {
		BinaryReader reader = hierarchy.open_file(path);
		this->path = path;

		size_t vertices = 0;
		size_t indices = 0;
		size_t uvs = 0;
		size_t matrices = 0;

		model = std::make_shared<mdx::MDX>(reader);

		has_mesh = model->has_chunk<mdx::GEOS>();
		if (has_mesh) {
			// Calculate required space
			for (auto&& i : model->chunk<mdx::GEOS>()->geosets) {
				vertices += i.vertices.size();
				indices += i.faces.size();
				//uvs += i.texture_coordinate_sets.size() * i.texture_coordinate_sets.front().coordinates.size();
				for (auto&& vertex_group_id : i.vertex_groups) {
					vertex_groups.push_back(vertex_group_id + matrices);
				}
				matrices += i.matrix_groups.size();
			}

			// Allocate space
			gl->glCreateBuffers(1, &vertex_buffer);
			gl->glNamedBufferData(vertex_buffer, vertices * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW); // TODO static draw

			gl->glCreateBuffers(1, &uv_buffer);
			gl->glNamedBufferData(uv_buffer, vertices * sizeof(glm::vec2), nullptr, GL_DYNAMIC_DRAW); // TODO static draw

			gl->glCreateBuffers(1, &instance_buffer);

			gl->glCreateBuffers(1, &layer_alpha);
			gl->glCreateBuffers(1, &geoset_color);

			//gl->glCreateBuffers(1, &retera_vertex_group_buffer);

			//gl->glGenBuffers(1, &retera_node_buffer);
			gl->glCreateBuffers(1, &retera_node_buffer);
			// We need a buffer texture of this one
			//gl->glActiveTexture(GL_TEXTURE15);
			gl->glCreateTextures(GL_TEXTURE_BUFFER, 1, &retera_node_buffer_texture);
			//gl->glTextureBuffer(retera_node_buffer_texture, GL_RGBA32F, retera_node_buffer);

			// Vertex groups that are not per vertex, shader does indexing with group ID
			gl->glCreateBuffers(1, &retera_groups_buffer);
			for (auto&& i : model->chunk<mdx::GEOS>()->geosets) {
				int geoset_matrix_offset = 0;
				for (auto&& matrix_size : i.matrix_groups) {
					// show size is capped at 3 because that's what I made the shader do.
					// Not a correct behavior, just a simpler one.
					int show_size = matrix_size > 3 ? 3 : matrix_size;
					glm::uvec4 matrixData(show_size,0,0,0);
					for (int index = 0; index < show_size; index++) {
						matrixData[index + 1] = i.matrix_indices[geoset_matrix_offset + index] + 1;
					}
					group_indexing_lookups.push_back(matrixData);
					geoset_matrix_offset += matrix_size;
				}
			}
			//gl->glNamedBufferData(retera_groups_buffer, matrices * sizeof(glm::uvec4), group_indexing_lookups.data(), GL_STATIC_DRAW); // TODO static draw
			//gl->glActiveTexture(GL_TEXTURE14);
			gl->glCreateTextures(GL_TEXTURE_BUFFER, 1, &retera_groups_buffer_texture);


			
			// We need a buffer texture of this one
			//gl->glCreateTextures(GL_TEXTURE14, 1, &retera_groups_buffer_texture);
			// TODO the GLenum must change from GL_RGBA32UI to something like 8UI, type must be uint8 instead of uint32,
			// but I dunno how to tell OpenGL how to do that because I dunno OpenGL's deep secrets
			//gl->glTextureBuffer(retera_groups_buffer_texture, GL_RGBA32I, retera_groups_buffer);

			// Per-vertex group IDs
			gl->glCreateBuffers(1, &retera_vertex_group_buffer);
			gl->glNamedBufferData(retera_vertex_group_buffer, vertex_groups.size() * sizeof(uint), vertex_groups.data(), GL_STATIC_DRAW); // TODO static draw



			gl->glCreateBuffers(1, &index_buffer);
			gl->glNamedBufferData(index_buffer, indices * sizeof(uint16_t), nullptr, GL_DYNAMIC_DRAW);

			// Buffer Data
			int base_vertex = 0;
			int base_index = 0;
			for (auto&& i : model->chunk<mdx::GEOS>()->geosets) {
				MeshEntry entry;
				entry.vertices = i.vertices.size();
				entry.base_vertex = base_vertex;

				entry.indices = i.faces.size();
				entry.base_index = base_index;

				entry.material_id = i.material_id;
				entry.geoset_anim = nullptr;

				entries.push_back(entry);

				gl->glNamedBufferSubData(vertex_buffer, base_vertex * sizeof(glm::vec3), entry.vertices * sizeof(glm::vec3), i.vertices.data());
				gl->glNamedBufferSubData(uv_buffer, base_vertex * sizeof(glm::vec2), entry.vertices * sizeof(glm::vec2), i.texture_coordinate_sets.front().coordinates.data());
				gl->glNamedBufferSubData(index_buffer, base_index * sizeof(uint16_t), entry.indices * sizeof(uint16_t), i.faces.data());

				base_vertex += entry.vertices;
				base_index += entry.indices;
			}
		}

		if (model->has_chunk<mdx::SEQS>()) {
			for (auto&& i : model->chunk<mdx::SEQS>()->sequences) {
				Animation animation;
				animation.interval_start = i.interval_start;
				animation.interval_end = i.interval_end;
				animation.movespeed = i.movespeed;
				animation.flags = i.flags;
				animation.rarity = i.rarity;
				animation.sync_point = i.sync_point;
				animation.extent = i.extent;

				std::transform(i.name.begin(), i.name.end(), i.name.begin(), ::tolower);

				animations.emplace(i.name, animation);
			}
		}

		if (model->has_chunk<mdx::GEOA>()) {
			for (auto&& i : model->chunk<mdx::GEOA>()->animations) {
				if (i.geoset_id >= 0 && i.geoset_id <= entries.size()) {
					entries[i.geoset_id].geoset_anim = &i;
				}
			}
		}

		if (model->has_chunk<mdx::TEXS>()) {
			for (auto&& i : model->chunk<mdx::TEXS>()->textures) {
				if (i.replaceable_id != 0) {
					if (mdx::replacable_id_to_texture.find(i.replaceable_id) == mdx::replacable_id_to_texture.end()) {
						std::cout << "Unknown replacable ID found\n";
					}
					textures.push_back(resource_manager.load<GPUTexture>(mdx::replacable_id_to_texture[i.replaceable_id]));
				}
				else {
					textures.push_back(resource_manager.load<GPUTexture>(i.file_name));
					// ToDo Same texture on different model with different flags?
					gl->glTextureParameteri(textures.back()->id, GL_TEXTURE_WRAP_S, i.flags & 1 ? GL_REPEAT : GL_CLAMP_TO_EDGE);
					gl->glTextureParameteri(textures.back()->id, GL_TEXTURE_WRAP_T, i.flags & 1 ? GL_REPEAT : GL_CLAMP_TO_EDGE);
				}
			}
		}

		mtls = model->chunk<mdx::MTLS>();
	}
}

AnimatedMesh::~AnimatedMesh()
{
	gl->glDeleteBuffers(1, &vertex_buffer);
	gl->glDeleteBuffers(1, &uv_buffer);
	gl->glDeleteBuffers(1, &instance_buffer);
	gl->glDeleteBuffers(1, &index_buffer);
}

void AnimatedMesh::render_queue(const std::shared_ptr<SkeletalModelInstance> & mvp)
{
	render_jobs.push_back(mvp);
	render_matrices.push_back(mvp->matrix);

	if (render_jobs.size() == 1) {
		map->animatedMeshes.push_back(this);
	}
}

void AnimatedMesh::render()
{
	if (!has_mesh) {
		render_jobs.clear();
		render_matrices.clear();
		instance_bone_matrices.clear();
		return;
	}
	if (render_jobs.size() == 0) {
		return;
	}

	// Make the bone texture
	int bone_count = 0;
	if (model->has_chunk<mdx::BONE>()) {
		bone_count = model->chunk<mdx::BONE>()->bones.size();
		// Hypothetically we should know the number of render jobs
		// at all times. So this is horrible code, really, because I
		// dynamically create the bone texture on every frame, which
		// is horrible
		//gl->glTextureStorage2D(bone_texture, 1, GL_RGBA8, model->chunk<mdx::BONE>()->bones.size() * 4, render_jobs.size());

		//gl->glTextureParameteri(bone_texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		//gl->glTextureParameteri(bone_texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		//gl->glTextureParameteri(bone_texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		//gl->glTextureParameteri(bone_texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}

	gl->glEnableVertexAttribArray(0);
	gl->glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
	gl->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	gl->glEnableVertexAttribArray(1);
	gl->glBindBuffer(GL_ARRAY_BUFFER, uv_buffer);
	gl->glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	gl->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, index_buffer);

	// ToDo support "Doodads\\Ruins\\Water\\BubbleGeyser\\BubbleGeyser.mdx"

	shader->use();
	gl->glNamedBufferData(instance_buffer, render_jobs.size() * sizeof(glm::mat4), render_matrices.data(), GL_STATIC_DRAW);

	// Since a mat4 is 4 vec4's
	gl->glEnableVertexAttribArray(2);
	gl->glBindBuffer(GL_ARRAY_BUFFER, instance_buffer);
	for (int i = 0; i < 4; i++) {
		gl->glEnableVertexAttribArray(2 + i);
		gl->glVertexAttribPointer(2 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), reinterpret_cast<const void*>(sizeof(glm::vec4) * i));
		gl->glVertexAttribDivisor(2 + i, 1);
	}

	gl->glEnableVertexAttribArray(6);
	gl->glBindBuffer(GL_ARRAY_BUFFER, retera_vertex_group_buffer);
	gl->glVertexAttribIPointer(6, 1, GL_UNSIGNED_INT, sizeof(uint), nullptr);
	gl->glVertexAttribDivisor(6, 0);

	gl->glUniformMatrix4fv(7, 1, false, &camera->projection_view[0][0]);

	for (int i = 0; i < render_jobs.size(); i++) {
		instance_bone_matrices.push_back(glm::mat4(0.0f)); // Our shader code currently wants loading "matrix 0" (per instance) to load a totally empty one
		for (int j = 0; j < bone_count; j++) { //render_jobs[0]->renderNodes.size()
			instance_bone_matrices.push_back(render_jobs[i]->renderNodes[j]->worldMatrix);
		}
	}
	//gl->glNamedBufferData(retera_node_buffer, instance_bone_matrices.size() * sizeof(glm::mat4), instance_bone_matrices.data(), GL_DYNAMIC_DRAW);
	//gl->glBindBuffer(GL_ARRAY_BUFFER, retera_node_buffer);

	/*gl->glActiveTexture(GL_TEXTURE14);
	gl->glBindTexture(GL_TEXTURE_BUFFER, retera_groups_buffer_texture);
	gl->glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, retera_groups_buffer);
	gl->glUniform1i(gl->glGetUniformLocation(shader->program, "u_groupIndexing"), 14);

	gl->glActiveTexture(GL_TEXTURE15);
	gl->glBindTexture(GL_TEXTURE_BUFFER, retera_node_buffer_texture);
	gl->glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, retera_node_buffer);
	gl->glUniform1i(gl->glGetUniformLocation(shader->program, "u_nodeMatrices"), 15);*/

	gl->glActiveTexture(GL_TEXTURE15);
	gl->glBindBuffer(GL_TEXTURE_BUFFER, retera_node_buffer);
	gl->glBufferData(GL_TEXTURE_BUFFER, instance_bone_matrices.size() * sizeof(glm::mat4), instance_bone_matrices.data(), GL_DYNAMIC_DRAW);
	gl->glBindBuffer(GL_TEXTURE_BUFFER, 0);

	gl->glBindTexture(GL_TEXTURE_BUFFER, retera_node_buffer_texture);
	gl->glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, retera_node_buffer);

	gl->glActiveTexture(GL_TEXTURE14);
	gl->glBindBuffer(GL_TEXTURE_BUFFER, retera_groups_buffer);
	gl->glBufferData(GL_TEXTURE_BUFFER, group_indexing_lookups.size() * sizeof(glm::uvec4), group_indexing_lookups.data(), GL_DYNAMIC_DRAW);
	gl->glBindBuffer(GL_TEXTURE_BUFFER, 0);

	gl->glBindTexture(GL_TEXTURE_BUFFER, retera_groups_buffer_texture);
	gl->glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32UI, retera_groups_buffer);

	gl->glUniform1i(gl->glGetUniformLocation(shader->program, "u_groupIndexing"), 14);
	gl->glUniform1i(gl->glGetUniformLocation(shader->program, "u_nodeMatrices"), 15);

	//gl->glUniform1i(8, retera_groups_buffer_texture);
	//gl->glUniform1i(9, retera_node_buffer_texture);

	gl->glUniform1i(10, bone_count + 1);
	int entry_index = -1;
	for (auto&& i : entries) {
		entry_index++;
		if (!i.visible) {
			continue;
		}
		// create vector of per-instance color and visibility
		i.geoset_anim_alphas.clear();
		i.geoset_anim_colors.clear();
		for (int k = 0; k < render_jobs.size(); k++) {
			glm::vec3 geoset_color(1.0f);
			float geoset_anim_visibility = 1.0f;
			if (i.geoset_anim && render_jobs[k]->sequence_index >= 0) {
				i.geoset_anim->getColor(geoset_color, render_jobs[k]->current_frame, *render_jobs[k]);
				i.geoset_anim->getVisibility(geoset_anim_visibility, render_jobs[k]->current_frame, *render_jobs[k]);
			}
			i.geoset_anim_alphas.push_back(geoset_anim_visibility);
			i.geoset_anim_colors.push_back(geoset_color);
		}
		for (auto&& j : mtls->materials[i.material_id].layers) {
			gl->glBindTextureUnit(0, textures[j.texture_id]->id);

			// create vector of per-instance color and visibility
			i.layer_alphas.clear();
			for (int k = 0; k < render_jobs.size(); k++) {
				float layer_visibility = 1.0f;
				if (render_jobs[k]->sequence_index >= 0) {
					j.getVisibility(layer_visibility, render_jobs[k]->current_frame, *render_jobs[k]);
				}
				i.layer_alphas.push_back(layer_visibility * i.geoset_anim_alphas[k]);
			}

			gl->glEnableVertexAttribArray(11);
			gl->glBindBuffer(GL_ARRAY_BUFFER, layer_alpha);
			gl->glNamedBufferData(layer_alpha, i.layer_alphas.size() * sizeof(float), i.layer_alphas.data(), GL_DYNAMIC_DRAW);
			gl->glVertexAttribPointer(11, 1, GL_FLOAT, GL_FALSE, 0, nullptr);
			gl->glVertexAttribDivisor(11, 1);

			gl->glEnableVertexAttribArray(12);
			gl->glBindBuffer(GL_ARRAY_BUFFER, geoset_color);
			gl->glNamedBufferData(geoset_color, i.geoset_anim_colors.size() * sizeof(glm::vec3), i.geoset_anim_colors.data(), GL_DYNAMIC_DRAW);
			gl->glVertexAttribPointer(12, 3, GL_FLOAT, GL_FALSE, 0, nullptr);
			gl->glVertexAttribDivisor(12, 1);

			gl->glEnable(GL_BLEND);
			gl->glUniform1f(3, -1.f);
			switch (j.blend_mode) {
			case 0:
				gl->glDisable(GL_BLEND);
				break;
			case 1:
				gl->glDisable(GL_BLEND);
				gl->glUniform1f(3, 0.75f); // Alpha test
				break;
			case 2:
				gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
				break;
			case 3:
				gl->glBlendFunc(GL_ONE, GL_ONE);
				break;
			case 4:
				gl->glBlendFunc(GL_SRC_ALPHA, GL_ONE);
				break;
			case 5:
				gl->glBlendFunc(GL_ZERO, GL_SRC_COLOR);
				break;
			case 6:
				gl->glBlendFunc(GL_DST_COLOR, GL_SRC_COLOR);
				break;
			}

			gl->glDrawElementsInstancedBaseVertex(GL_TRIANGLES, i.indices, GL_UNSIGNED_SHORT, reinterpret_cast<void*>(i.base_index * sizeof(uint16_t)), render_jobs.size(), i.base_vertex);
		}
	}
	gl->glDisableVertexAttribArray(2);

	for (int i = 0; i < 4; i++) {
		gl->glVertexAttribDivisor(2 + i, 0); // ToDo use multiple vao
	}

	gl->glDisableVertexAttribArray(0);
	gl->glDisableVertexAttribArray(1);
	gl->glDisableVertexAttribArray(6);
	gl->glEnable(GL_BLEND);

	render_jobs.clear();
	render_matrices.clear();
	instance_bone_matrices.clear();
}