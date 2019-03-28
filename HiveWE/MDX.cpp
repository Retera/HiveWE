#include "stdafx.h"

namespace mdx {
	std::map<int, std::string> replacable_id_to_texture{
		{ 1, "ReplaceableTextures/TeamColor/TeamColor00.blp" },
		{ 2, "ReplaceableTextures/TeamGlow/TeamGlow00.blp" },
		{ 11, "ReplaceableTextures/Cliff/Cliff0.blp" },
		{ 31, "ReplaceableTextures/LordaeronTree/LordaeronFallTree.blp" },
		{ 32, "ReplaceableTextures/AshenvaleTree/AshenTree.blp" },
		{ 33, "ReplaceableTextures/BarrensTree/BarrensTree.blp" },
		{ 34, "ReplaceableTextures/NorthrendTree/NorthTree.blp" },
		{ 35, "ReplaceableTextures/Mushroom/MushroomTree.blp" },
		{ 36, "ReplaceableTextures/RuinsTree/RuinsTree.blp" },
		{ 37, "ReplaceableTextures/OutlandMushroomTree/MushroomTree.blp" }
	};

	void AnimatedData::load_tracks(BinaryReader& reader) {
		TrackTag tag = static_cast<TrackTag>(reader.read<int32_t>());

		switch (tag) {
		case TrackTag::KMTF:
		case TrackTag::KLAS:
		case TrackTag::KLAE:
		case TrackTag::KRTX:
		case TrackTag::KCRL:
			tracks.emplace(tag, std::make_unique<TrackHeader<uint32_t>>(reader));
			break;
		case TrackTag::KMTA:
		case TrackTag::KGAO:
		case TrackTag::KLAI:
		case TrackTag::KLBI:
		case TrackTag::KLAV:
		case TrackTag::KATV:
		case TrackTag::KPEE:
		case TrackTag::KPEG:
		case TrackTag::KPLN:
		case TrackTag::KPLT:
		case TrackTag::KPEL:
		case TrackTag::KPES:
		case TrackTag::KPEV:
		case TrackTag::KP2S:
		case TrackTag::KP2R:
		case TrackTag::KP2L:
		case TrackTag::KP2G:
		case TrackTag::KP2E:
		case TrackTag::KP2N:
		case TrackTag::KP2W:
		case TrackTag::KP2V:
		case TrackTag::KRHA:
		case TrackTag::KRHB:
		case TrackTag::KRAL:
		case TrackTag::KRVS:
			tracks.emplace(tag, std::make_unique<TrackHeader<float>>(reader));
			break;
		case TrackTag::KTAT:
		case TrackTag::KTAS:
		case TrackTag::KGAC:
		case TrackTag::KLAC:
		case TrackTag::KLBC:
		case TrackTag::KTTR:
		case TrackTag::KCTR:
		case TrackTag::KRCO:
		case TrackTag::KGTR:
		case TrackTag::KGSC:
			tracks.emplace(tag, std::make_unique<TrackHeader<glm::vec3>>(reader));
			break;
		case TrackTag::KTAR:
		case TrackTag::KGRT:
			tracks.emplace(tag, std::make_unique<TrackHeader<glm::quat>>(reader));
			break;
		default:
			std::cout << "Invalid Track Tag " << static_cast<int>(tag) << "\n";
		}
	}



	Extent::Extent(BinaryReader& reader) {
		bounds_radius = reader.read<float>();
		minimum = reader.read<glm::vec3>();
		maximum = reader.read<glm::vec3>();
	} 

	TextureCoordinateSet::TextureCoordinateSet(BinaryReader& reader) {
		reader.advance(4);
		const uint32_t texture_coordinates_count = reader.read<uint32_t>();
		coordinates = reader.read_vector<glm::vec2>(texture_coordinates_count);
	}

	Layer::Layer(BinaryReader& reader) {
		const int reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();
		blend_mode = reader.read<uint32_t>();
		shading_flags = reader.read<uint32_t>();
		texture_id = reader.read<uint32_t>();
		texture_animation_id = reader.read<uint32_t>();
		coord_id = reader.read<uint32_t>();
		alpha = reader.read<float>();


		while (reader.position < reader_pos + size) {
			animated_data.load_tracks(reader);
		}
		// Skip tags
		//reader.advance(size - 28);
	}

	void Layer::getVisibility(float & out, int frame, SkeletalModelInstance & instance)
	{
		if (animated_data.has_track(TrackTag::KMTA)) {
			std::shared_ptr<TrackHeader<float>> header = animated_data.track<float>(TrackTag::KMTA);
			header->matrixEaterInterpolate(out, frame, instance, alpha);
		}
		else {
			out = alpha;
		}
	}

	Texture::Texture(BinaryReader& reader) {
		replaceable_id = reader.read<uint32_t>();
		file_name = reader.read_string(260);
		flags = reader.read<uint32_t>();
	}

	Node::Node(BinaryReader& reader) {
		const int reader_pos = reader.position;
		const uint32_t inclusive_size = reader.read<uint32_t>();
		name = reader.read_string(80);
		object_id = reader.read<uint32_t>();
		parent_id = reader.read<uint32_t>();
		flags = reader.read<uint32_t>();

		while (reader.position < reader_pos + inclusive_size) {
			animated_data.load_tracks(reader);
		}
	}
	void NodeContainer::getVisibility(float& out, SkeletalModelInstance & instance)
	{
		out = 1.0f;
	}

	Sequence::Sequence(BinaryReader& reader) {
		name = reader.read_string(80);
		interval_start = reader.read<uint32_t>();
		interval_end = reader.read<uint32_t>();
		movespeed = reader.read<float>();
		flags = reader.read<uint32_t>();
		rarity = reader.read<float>();
		sync_point = reader.read<uint32_t>();
		extent = Extent(reader);
	}

	SEQS::SEQS(BinaryReader& reader) {
		const uint32_t size = reader.read<uint32_t>();
		for (size_t i = 0; i < size / 132; i++) {
			sequences.emplace_back(Sequence(reader));
		}
	}

	GLBS::GLBS(BinaryReader & reader)
	{
		const uint32_t size = reader.read<uint32_t>();
		for (size_t i = 0; i < size/4; i++) {
			global_sequences.push_back(reader.read<uint32_t>());
		}
	}

	GEOS::GEOS(BinaryReader& reader) {
		const uint32_t size = reader.read<uint32_t>();
		uint32_t total_size = 0;

		while (total_size < size) {
			total_size += reader.read<uint32_t>();

			Geoset geoset;
			reader.advance(4);
			const uint32_t vertex_count = reader.read<uint32_t>();
			geoset.vertices = reader.read_vector<glm::vec3>(vertex_count);
			reader.advance(4);
			const uint32_t normal_count = reader.read<uint32_t>();
			geoset.normals = reader.read_vector<glm::vec3>(normal_count);
			reader.advance(4);
			const uint32_t face_type_groups_count = reader.read<uint32_t>();
			geoset.face_type_groups = reader.read_vector<uint32_t>(face_type_groups_count);
			reader.advance(4);
			const uint32_t face_groups_count = reader.read<uint32_t>();
			geoset.face_groups = reader.read_vector<uint32_t>(face_groups_count);
			reader.advance(4);
			const uint32_t faces_count = reader.read<uint32_t>();
			geoset.faces = reader.read_vector<uint16_t>(faces_count);
			reader.advance(4);
			const uint32_t vertex_groups_count = reader.read<uint32_t>();
			geoset.vertex_groups = reader.read_vector<uint8_t>(vertex_groups_count);
			reader.advance(4);
			const uint32_t matrix_group_count = reader.read<uint32_t>();
			geoset.matrix_groups = reader.read_vector<uint32_t>(matrix_group_count);
			reader.advance(4);
			const uint32_t matrix_indices_count = reader.read<uint32_t>();
			geoset.matrix_indices = reader.read_vector<uint32_t>(matrix_indices_count);
			geoset.material_id = reader.read<uint32_t>();
			geoset.selection_group = reader.read<uint32_t>();
			geoset.selection_flags = reader.read<uint32_t>();

			geoset.extent = Extent(reader);
			const uint32_t extents_count = reader.read<uint32_t>();
			for (size_t i = 0; i < extents_count; i++) {
				geoset.extents.emplace_back(Extent(reader));
			}
			reader.advance(4);
			const uint32_t texture_coordinate_sets_count = reader.read<uint32_t>();
			for (size_t i = 0; i < texture_coordinate_sets_count; i++) {
				geoset.texture_coordinate_sets.emplace_back(TextureCoordinateSet(reader));
			}

			geosets.push_back(geoset);
		}
	}

	GEOA::GEOA(BinaryReader& reader) {
		uint32_t remaining_size = reader.read<uint32_t>();

		while (remaining_size > 0) {
			const int reader_pos = reader.position;
			const uint32_t inclusive_size = reader.read<uint32_t>();
			remaining_size -= inclusive_size;

			GeosetAnimation animation;
			animation.alpha = reader.read<float>();
			animation.flags = reader.read<uint32_t>();
			animation.color = reader.read<glm::vec3>();
			animation.geoset_id = reader.read<uint32_t>();

			while (reader.position < reader_pos + inclusive_size) {
				animation.animated_data.load_tracks(reader);
			}

			animations.push_back(std::move(animation));
		}
	}

	TEXS::TEXS(BinaryReader& reader) {
		const uint32_t size = reader.read<uint32_t>();

		for (size_t i = 0; i < size / 268; i++) {
			textures.emplace_back(Texture(reader));
		}
	}

	MTLS::MTLS(BinaryReader& reader) {
		const uint32_t size = reader.read<uint32_t>();
		uint32_t total_size = 0;

		while (total_size < size) {
			total_size += reader.read<uint32_t>();

			Material material;
			material.priority_plane = reader.read<uint32_t>();
			material.flags = reader.read<uint32_t>();
			reader.advance(4);
			const uint32_t layers_count = reader.read<uint32_t>();

			for (size_t i = 0; i < layers_count; i++) {
				material.layers.emplace_back(Layer(reader));
			}

			materials.push_back(material);
		}
	}

	HELP::HELP(BinaryReader & reader)
	{
		const int reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			NodeContainer nodeContainer;
			nodeContainer.node = Node(reader);
			bones.push_back(std::move(nodeContainer));
		}
	}

	LITE::LITE(BinaryReader& reader) {
		const int reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			Light light;
			const int node_reader_pos = reader.position;
			const uint32_t inclusive_size = reader.read<uint32_t>();
			light.node = Node(reader);
			light.type = reader.read<uint32_t>();
			light.attenuation_start = reader.read<float>();
			light.attenuation_end = reader.read<float>();
			light.color = reader.read<glm::vec3>();
			light.intensity = reader.read<float>();
			light.ambient_color = reader.read<glm::vec3>();
			light.ambient_intensity = reader.read<float>();
			while (reader.position < node_reader_pos + inclusive_size) {
				light.node.animated_data.load_tracks(reader);
			}
			lights.push_back(std::move(light));
		}
	}

	BONE::BONE(BinaryReader& reader) {
		const int reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			Bone bone;
			bone.node = Node(reader);
			bone.geoset_id = reader.read<uint32_t>();
			bone.geoset_animation_id = reader.read<uint32_t>();
			bones.push_back(std::move(bone));
		}
	}

	ATCH::ATCH(BinaryReader & reader)
	{
		const int reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			Attachment attachment;
			const int node_reader_pos = reader.position;
			const uint32_t inclusive_size = reader.read<uint32_t>();
			attachment.node = Node(reader);
			attachment.path = reader.read_string(256);
			attachment.reserved = reader.read<uint32_t>();
			attachment.attachment_id = reader.read<uint32_t>();
			while (reader.position < node_reader_pos + inclusive_size) {
				attachment.node.animated_data.load_tracks(reader);
			}
			attachments.push_back(std::move(attachment));
		}
	}

	PIVT::PIVT(BinaryReader& reader) {
		const int reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			pivots = reader.read_vector<glm::vec3>(size / 12);
		}
	}

	PREM::PREM(BinaryReader& reader) {
		const int reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			ParticleEmitter1 emitter;
			const int node_reader_pos = reader.position;
			const uint32_t inclusive_size = reader.read<uint32_t>();
			emitter.node = Node(reader);
			emitter.emission_rate = reader.read<float>();
			emitter.gravity = reader.read<float>();
			emitter.longitude = reader.read<float>();
			emitter.latitude = reader.read<float>();
			emitter.path = reader.read_string(256);
			emitter.reserved = reader.read<uint32_t>();
			emitter.life_span = reader.read<float>();
			emitter.speed = reader.read<float>();
			while (reader.position < node_reader_pos + inclusive_size) {
				emitter.node.animated_data.load_tracks(reader);
			}
			emitters.push_back(std::move(emitter));
		}
	}

	PRE2::PRE2(BinaryReader& reader) {
		const int reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			ParticleEmitter2 emitter2;
			const int node_reader_pos = reader.position;
			const uint32_t inclusive_size = reader.read<uint32_t>();
			emitter2.node = Node(reader);

			emitter2.speed = reader.read<float>();
			emitter2.variation = reader.read<float>();
			emitter2.latitude = reader.read<float>();
			emitter2.gravity = reader.read<float>();
			emitter2.life_span = reader.read<float>();
			emitter2.emission_rate = reader.read<float>();
			emitter2.length = reader.read<float>();
			emitter2.width = reader.read<float>();
			emitter2.filter_mode = reader.read<uint32_t>();
			emitter2.rows = reader.read<uint32_t>();
			emitter2.columns = reader.read<uint32_t>();
			emitter2.head_or_tail = reader.read<uint32_t>();
			emitter2.tail_length = reader.read<float>();
			emitter2.time_middle = reader.read<float>();
			for (int time = 0; time < 3; time++) {
				for (int i = 0; i < 3; i++) {
					emitter2.segment_color[time][i] = reader.read<float>();
				}
			}
			for (int i = 0; i < 3; i++) {
				emitter2.segment_alphas[i] = reader.read<uint8_t>();
			}
			for (int i = 0; i < 3; i++) {
				emitter2.segment_scaling[i] = reader.read<float>();
			}
			for (int i = 0; i < 3; i++) {
				emitter2.head_intervals[i] = reader.read<float>();
			}
			for (int i = 0; i < 3; i++) {
				emitter2.head_decay_intervals[i] = reader.read<float>();
			}
			for (int i = 0; i < 3; i++) {
				emitter2.tail_intervals[i] = reader.read<float>();
			}
			for (int i = 0; i < 3; i++) {
				emitter2.tail_decay_intervals[i] = reader.read<float>();
			}
			emitter2.texture_id = reader.read<uint32_t>();
			emitter2.squirt = reader.read<uint32_t>();
			emitter2.priority_plane = reader.read<uint32_t>();
			emitter2.replaceable_id = reader.read<uint32_t>();

			while (reader.position < node_reader_pos + inclusive_size) {
				emitter2.node.animated_data.load_tracks(reader);
			}
			emitters.push_back(std::move(emitter2));
		}
	}

	RIBB::RIBB(BinaryReader& reader) {
		const int reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			RibbonEmitter emitter;
			const int node_reader_pos = reader.position;
			const uint32_t inclusive_size = reader.read<uint32_t>();
			emitter.node = Node(reader);
			emitter.height_above = reader.read<float>();
			emitter.height_below = reader.read<float>();
			emitter.alpha = reader.read<float>();
			emitter.color = reader.read<glm::vec3>();
			emitter.life_span = reader.read<float>();
			emitter.texture_slot = reader.read<uint32_t>();
			emitter.emission_rate = reader.read<uint32_t>();
			emitter.rows = reader.read<uint32_t>();
			emitter.columns = reader.read<uint32_t>();
			emitter.material_id = reader.read<uint32_t>();
			emitter.gravity = reader.read<float>();
			while (reader.position < node_reader_pos + inclusive_size) {
				emitter.node.animated_data.load_tracks(reader);
			}
			emitters.push_back(std::move(emitter));
		}
	}

	EVTS::EVTS(BinaryReader& reader) {
		const int reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			EventObject evt;
			evt.node = Node(reader);
			reader.read<uint32_t>(); // read KEVT
			evt.count = reader.read<uint32_t>();
			evt.global_sequence_id = reader.read<int32_t>(); //signed
			for (int i = 0; i < evt.count; i++) {
				evt.times.push_back(reader.read<uint32_t>());
			}
			eventObjects.push_back(std::move(evt));
		}
	}

	CLID::CLID(BinaryReader& reader) {
		const int reader_pos = reader.position;
		const uint32_t size = reader.read<uint32_t>();

		while (reader.position < reader_pos + size) {
			CollisionShape shape;
			shape.node = Node(reader);
			uint32_t type_index = reader.read<uint32_t>();
			switch (type_index) {
			case 1:
				shape.type = CollisionShapeType::Plane;
				break;
			case 2:
				shape.type = CollisionShapeType::Sphere;
				break;
			case 3:
				shape.type = CollisionShapeType::Cylinder;
				break;
			default:
			case 0:
				shape.type = CollisionShapeType::Box;
				break;
			}
			int vertex_count;
			if (shape.type != CollisionShapeType::Sphere) {
				vertex_count = 2;
			}
			else {
				vertex_count = 1;
			}
			for (int vertex_id = 0; vertex_id < vertex_count; vertex_id++) {
				for (int i = 0; i < 3; i++) {
					shape.vertices[vertex_id][i] = reader.read<float>();
				}
			}
			if (shape.type == CollisionShapeType::Sphere || shape.type == CollisionShapeType::Cylinder) {
				shape.radius = reader.read<float>();
			}
		}
	}

	MDX::MDX(BinaryReader& reader) {
		load(reader);
	}

	void MDX::load(BinaryReader& reader) {
		const std::string magic_number = reader.read_string(4);
		if (magic_number != "MDLX") {
			std::cout << "The file's magic number is incorrect. Should be MDLX, is: " << magic_number << std::endl;
			return;
		}

		while (reader.remaining() > 0) {
			uint32_t header = reader.read<uint32_t>();
			//std::cout << "Reading: "  << (char)(header >> 0) << (char)(header >> 8) << (char)(header >> 16) << (char)(header >> 24) << std::endl;

			switch (static_cast<ChunkTag>(header)) {
				case ChunkTag::SEQS:
					chunks[ChunkTag::SEQS] = std::make_shared<SEQS>(reader);
					break;
				case ChunkTag::GLBS:
					chunks[ChunkTag::GLBS] = std::make_shared<GLBS>(reader);
					break;
				case ChunkTag::MTLS:
					chunks[ChunkTag::MTLS] = std::make_shared<MTLS>(reader);
					break;
				case ChunkTag::TEXS:
					chunks[ChunkTag::TEXS] = std::make_shared<TEXS>(reader);
					break;
				case ChunkTag::GEOS:
					chunks[ChunkTag::GEOS] = std::make_shared<GEOS>(reader);
					break;
				case ChunkTag::GEOA:
					chunks[ChunkTag::GEOA] = std::make_shared<GEOA>(reader);
					break;
				case ChunkTag::BONE:
					chunks[ChunkTag::BONE] = std::make_shared<BONE>(reader);
					break;
				case ChunkTag::LITE:
					chunks[ChunkTag::LITE] = std::make_shared<LITE>(reader);
					break;
				case ChunkTag::HELP:
					chunks[ChunkTag::HELP] = std::make_shared<HELP>(reader);
					break;
				case ChunkTag::ATCH:
					chunks[ChunkTag::ATCH] = std::make_shared<ATCH>(reader);
					break;
				case ChunkTag::PIVT:
					chunks[ChunkTag::PIVT] = std::make_shared<PIVT>(reader);
					break;
				case ChunkTag::PREM:
					chunks[ChunkTag::PREM] = std::make_shared<PREM>(reader);
					break;
				case ChunkTag::PRE2:
					chunks[ChunkTag::PRE2] = std::make_shared<PRE2>(reader);
					break;
				case ChunkTag::RIBB:
					chunks[ChunkTag::RIBB] = std::make_shared<RIBB>(reader);
					break;
				case ChunkTag::EVTS:
					chunks[ChunkTag::EVTS] = std::make_shared<EVTS>(reader);
					break;
				case ChunkTag::CLID:
					chunks[ChunkTag::CLID] = std::make_shared<CLID>(reader);
					break;
				default:
					reader.advance(reader.read<uint32_t>());
			}
		}

		//std::cout << "Done reading MDX!" << std::endl;
	}
	void GeosetAnimation::getColor(glm::vec3 & out, int frame, SkeletalModelInstance & instance)
	{
		if (animated_data.has_track(TrackTag::KGAC)) {
			std::shared_ptr<TrackHeader<glm::vec3>> header = animated_data.track<glm::vec3>(TrackTag::KGAC);
			header->matrixEaterInterpolate(out, frame, instance, color);
		}
		else {
			out = color;
		}
	}
	void GeosetAnimation::getVisibility(float & out, int frame, SkeletalModelInstance & instance)
	{
		if (animated_data.has_track(TrackTag::KGAO)) {
			std::shared_ptr<TrackHeader<float>> header = animated_data.track<float>(TrackTag::KGAO);
			header->matrixEaterInterpolate(out, frame, instance, alpha);
		}
		else {
			out = alpha;
		}
	}
}
void mdx::MDX::forEachNode(const std::function<void(NodeContainer&)>& F)
{
	// The chunks were set up in this insane
	// way so we have to be able to do this to use
	// similar logic on all nodes.
	if (has_chunk<BONE>()) {
		std::shared_ptr<BONE> boneChunk = chunk<BONE>();
		for (std::vector<Bone>::iterator it = boneChunk->bones.begin(); it != boneChunk->bones.end(); ++it) {
			NodeContainer& bone = *it;
			F(bone);
		}
	}
	if (has_chunk<LITE>()) {
		std::shared_ptr<LITE> liteChunk = chunk<LITE>();
		for (std::vector<Light>::iterator it = liteChunk->lights.begin(); it != liteChunk->lights.end(); ++it) {
			NodeContainer& light = *it;
			F(light);
		}
	}
	if (has_chunk<HELP>()) {
		std::shared_ptr<HELP> helperChunk = chunk<HELP>();
		for (std::vector<NodeContainer>::iterator it = helperChunk->bones.begin(); it != helperChunk->bones.end(); ++it) {
			NodeContainer& node = *it;
			F(node);
		}
	}
	if (has_chunk<ATCH>()) {
		std::shared_ptr<ATCH> atchChunk = chunk<ATCH>();
		for (std::vector<Attachment>::iterator it = atchChunk->attachments.begin(); it != atchChunk->attachments.end(); ++it) {
			NodeContainer& atc = *it;
			F(atc);
		}
	}
	if (has_chunk<PREM>()) {
		std::shared_ptr<PREM> premChunk = chunk<PREM>();
		for (std::vector<ParticleEmitter1>::iterator it = premChunk->emitters.begin(); it != premChunk->emitters.end(); ++it) {
			NodeContainer& prem = *it;
			F(prem);
		}
	}
	// In Java I never used the 4 letter constants (they were used in Oger-Lord's library) and instead
	// I used code with readable names. I feel so much more like a neckbeard using
	// this unreadable horrible design, it's empowering. PREM2 power!!
	if (has_chunk<PRE2>()) {
		std::shared_ptr<PRE2> prem2Chunk = chunk<PRE2>();
		for (std::vector<ParticleEmitter2>::iterator it = prem2Chunk->emitters.begin(); it != prem2Chunk->emitters.end(); ++it) {
			NodeContainer& pre2 = *it;
			F(pre2);
		}
	}
	if (has_chunk<RIBB>()) {
		std::shared_ptr<RIBB> ribbonChunk = chunk<RIBB>();
		for (std::vector<RibbonEmitter>::iterator it = ribbonChunk->emitters.begin(); it != ribbonChunk->emitters.end(); ++it) {
			NodeContainer& ribb = *it;
			F(ribb);
		}
	}
	if (has_chunk<EVTS>()) {
		std::shared_ptr<EVTS> evtsChunk = chunk<EVTS>();
		for (std::vector<EventObject>::iterator it = evtsChunk->eventObjects.begin(); it != evtsChunk->eventObjects.end(); ++it) {
			NodeContainer& evt = *it;
			F(evt);
		}
	}
	if (has_chunk<CLID>()) {
		std::shared_ptr<CLID> clidChunk = chunk<CLID>();
		for (std::vector<CollisionShape>::iterator it = clidChunk->collisionShapes.begin(); it != clidChunk->collisionShapes.end(); ++it) {
			NodeContainer& clid = *it;
			F(clid);
		}
	}
}