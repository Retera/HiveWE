#pragma once


class SkeletalModelInstance;

namespace mdx {
	extern std::map<int, std::string> replacable_id_to_texture;

	// Constants here are in HEX because apparently we don't have the
	// old 2001 C++ compiler that could use a multi digit
	// character constant like 'KMTF', so basically we have ligma.
	// Should be:
	// KMTF = 'KMTF';
	enum class TrackTag {
		KMTF = 0x46544d4b,
		KMTA = 0x41544d4b,
		KTAT = 0x5441544b,
		KTAR = 0x5241544b,
		KTAS = 0x5341544b,
		KGAO = 0x4f41474b,
		KGAC = 0x4341474b,
		KLAS = 0x53414c4b,
		KLAE = 0x45414c4b,
		KLAC = 0x43414c4b,
		KLAI = 0x49414c4b,
		KLBI = 0x49424c4b,
		KLBC = 0x43424c4b,
		KLAV = 0x56414c4b,
		KATV = 0x5654414b,
		KPEE = 0x4545504b,
		KPEG = 0x4745504b,
		KPLN = 0x4e4c504b,
		KPLT = 0x544c504b,
		KPEL = 0x4c45504b,
		KPES = 0x5345504b,
		KPEV = 0x5645504b,
		KP2S = 0x5332504b,
		KP2R = 0x5232504b,
		KP2L = 0x4c32504b,
		KP2G = 0x4732504b,
		KP2E = 0x4532504b,
		KP2N = 0x4e32504b,
		KP2W = 0x5732504b,
		KP2V = 0x5632504b,
		KRHA = 0x4148524b,
		KRHB = 0x4248524b,
		KRAL = 0x4c41524b,
		KRCO = 0x4f43524b,
		KRTX = 0x5854524b,
		KRVS = 0x5356524b,
		KCTR = 0x5254434b,
		KTTR = 0x5254544b,
		KCRL = 0x4c52434b,
		KGTR = 0x5254474b,
		KGRT = 0x5452474b,
		KGSC = 0x4353474b
	};

	// ToDo: Automatically generate the int????? Just following eejin convention atm
	enum class ChunkTag {
		VERS = 1397900630,
		MODL = 1279545165,
		SEQS = 1397835091,
		MTLS = 1397511245,
		TEXS = 1398293844,
		GEOS = 1397704007,
		GEOA = 1095714119,
		BONE = 1162760002,
		LITE = 1163151692,
		HELP = 1347175752,
		ATCH = 1212372033,
		PIVT = 1414941008,
		PREM = 1296388688,
		PRE2 = 843403856,
		RIBB = 1111640402,
		EVTS = 1398036037,
		CLID = 1145654339,
		GLBS = 1396853831
	};

	struct Extent {
		float bounds_radius;
		glm::vec3 minimum;
		glm::vec3 maximum;

		Extent() = default;
		explicit Extent(BinaryReader& reader);
	};

	struct Sequence {
		explicit Sequence(BinaryReader& reader);

		std::string name;
		uint32_t interval_start;
		uint32_t interval_end;
		float movespeed;
		uint32_t flags; // 0: looping
						// 1: non looping
		float rarity;
		uint32_t sync_point;
		Extent extent;
	};

	template <typename T>
	struct Track {
		int32_t frame;
		T value;
		T inTan;
		T outTan;
	};

	struct TrackHeaderBase {
		int32_t interpolation_type;
		int32_t global_sequence_ID;

		virtual ~TrackHeaderBase() = default;
	};

	template <typename T>
	struct TrackHeader : TrackHeaderBase {
		std::vector<Track<T>> tracks;

		explicit TrackHeader(BinaryReader& reader) {
			const int tracks_count = reader.read<int32_t>();
			interpolation_type = reader.read<int32_t>();
			global_sequence_ID = reader.read<int32_t>();

			for (int i = 0; i < tracks_count; i++) {
				Track<T> track;
				track.frame = reader.read<int32_t>();
				track.value = reader.read<T>();
				if (interpolation_type > 1) {
					track.inTan = reader.read<T>();
					track.outTan = reader.read<T>();
				}
				tracks.push_back(track);
			}
		}

		// We have to pass the sequence here because
		// the return value is dependent on start/end time
		int ceilIndex(int frame, Sequence& sequence);
		void getValue(T& out, int frame, Sequence& sequence, const T& defaultValue);
		void matrixEaterInterpolate(T& out, int frame, SkeletalModelInstance& sequence, const T& defaultValue);



		~TrackHeader() = default;
	};

	struct AnimatedData {
		std::unordered_map<TrackTag, std::shared_ptr<TrackHeaderBase>> tracks;

		void load_tracks(BinaryReader& reader);

		AnimatedData() = default;
		AnimatedData(const AnimatedData&) = default;
		AnimatedData(AnimatedData&&) = default;
		AnimatedData& operator=(const AnimatedData&) = default;

		template<typename T>
		std::shared_ptr<TrackHeader<T>> track(const TrackTag track) {
			return std::dynamic_pointer_cast<TrackHeader<T>>(tracks[track]);
		}

		bool has_track(const TrackTag track) {
			return tracks.find(track) != tracks.end();
		}

	};

	struct TextureCoordinateSet {
		TextureCoordinateSet() = default;
		explicit TextureCoordinateSet(BinaryReader& reader);
		std::vector<glm::vec2> coordinates;
	};

	struct Layer {
		Layer() = default;
		explicit Layer(BinaryReader& reader);

		uint32_t blend_mode; // 0: none
							// 1: transparent
							// 2: blend
							// 3: additive
							// 4: add alpha
							// 5: modulate
							// 6: modulate 2x
		uint32_t shading_flags; // 0x1: unshaded
							// 0x2: sphere environment map
							// 0x4: ? (Retera: used 1.07 Game.dll's MDX->MDL, this is omitted in MDL, so means nothing) 
							// 0x8: ? (Retera: used 1.07 Game.dll's MDX->MDL, this is omitted in MDL, so means nothing)
							// 0x10: two sided
							// 0x20: unfogged
							// 0x30: no depth test
							// 0x40: no depth set
		uint32_t texture_id;
		uint32_t texture_animation_id;
		uint32_t coord_id;
		float alpha;
		AnimatedData animated_data;

		void getVisibility(float& out, int frame, SkeletalModelInstance& instance);
	};

	struct Node {
		Node() = default;
		explicit Node(BinaryReader& reader);

		std::string name;
		int object_id;
		int parent_id;
		int flags;
		AnimatedData animated_data;
	};

	// ToDo: I added this because I was trying not to
	// instigate too much change. At some point, maybe
	// NodeContainer and Node should become the same
	// struct/class?
	struct NodeContainer {
		Node node;
		virtual void getVisibility(float& out, SkeletalModelInstance& instance);
		
		template <typename  T>
		void getValue(T& out, TrackTag tag, SkeletalModelInstance& instance, const T& defaultValue);
	};

	struct Geoset {
		std::vector<glm::vec3> vertices;
		std::vector<glm::vec3> normals;
		std::vector<uint32_t> face_type_groups;
		std::vector<uint32_t> face_groups;
		std::vector<uint16_t> faces;
		std::vector<uint8_t> vertex_groups;
		std::vector<uint32_t> matrix_groups;
		std::vector<uint32_t> matrix_indices;

		uint32_t material_id;
		uint32_t selection_group;
		uint32_t selection_flags;
		Extent extent;

		std::vector<Extent> extents;
		std::vector<TextureCoordinateSet> texture_coordinate_sets;
	};

	struct GeosetAnimation {
		float alpha;
		uint32_t flags;
		glm::vec3 color;
		uint32_t geoset_id;
		AnimatedData animated_data;

		void getColor(glm::vec3& out, int frame, SkeletalModelInstance& instance);
		void getVisibility(float& out, int frame, SkeletalModelInstance& instance);
	};

	struct Texture {
		explicit Texture(BinaryReader& reader);
		uint32_t replaceable_id;
		std::string file_name;
		uint32_t flags;
	};

	struct Material {
		uint32_t priority_plane;
		uint32_t flags;
		std::vector<Layer> layers;
	};

	struct Bone : NodeContainer {
		int geoset_id;
		int geoset_animation_id;
	};

	struct Light : NodeContainer {
		int type;
		int attenuation_start;
		int attenuation_end;
		glm::vec3 color;
		float intensity;
		glm::vec3 ambient_color;
		float ambient_intensity;
	};

	struct Attachment : NodeContainer {
		std::string path; // Reference to Undead, NE, or Naga birth anim
		int reserved; // ToDo mine meaning of reserved from Game.dll, likely strlen
		int attachment_id;
	};

	// Dragon/bird death bone emitter; usually emit MDLs based
	// on "path" string, but has a setting to emit TGAs from
	// path also (In practice EmitterUsesTGA setting is almost
	// never used, in favor of ParticleEmitter2).
	struct ParticleEmitter1 : NodeContainer {
		float emission_rate;
		float gravity;
		float longitude;
		float latitude;
		std::string path;
		int reserved; // ToDo mine meaning, same as Attachment's reserved?
		float life_span;
		float speed;
	};

	/*
	ParticleEmitter2: Texture only advanced "v2" emitter (99.9% of use cases)
	
	The "Tail" mode emits elongated particles, and the "Head"
	mode emits perfect square shapes. If you think about
	something like frost armor's damage that looks like a "strike,"
	this is a tail particle. The Wisp is another example. Disenchant
	is another.
	This can also be used where all elongated particles are going
	the same direction (based on latitude settings), such as the
	vertical lines in the yellow "heal" spell effect.

	The "Head" mode would be used for the snowflakes in frost armor's
	damage. It is also used in the water spray from the base of the
	Water Elemental. We say in comments we want to support the bubble geyser.
	That is a "Head" mode particle emitting a bubble
	texture, and a second one with XYQuad setting (flat, not facing camera) emitting the
	ripples above the water.

	There is also a "Both" mode that will emit both head and tail (when head_or_tail=2).

	--
	Each ParticleEmitter2 is a rectangular area specified by length and width that
	can be hand drawn in 3DS max, but many of them choose to forgo use of the rectangle,
	make it very small, and then use latitude settings to emit outward in all directions
	randomly, effectively making a point emitter (like NE Wisp).
	I seem to recall that variation is the random speed variation (when zero,
	all particles would move with equal speed, modified downward by gravity setting
	as an acceleration ).
	*/
	struct ParticleEmitter2 : NodeContainer {
		float speed;
		float variation;
		float latitude;
		float gravity;
		float life_span;
		float emission_rate;
		float length;
		float width;
		uint32_t filter_mode;
		uint32_t rows; // for Textures\Clouds8x8 files
		uint32_t columns;
		uint32_t head_or_tail;
		float tail_length;
		float time_middle;
		float segment_color[3][3]; // rows [Begin, Middle, End], column is color
		uint8_t segment_alphas[3];
		float segment_scaling[3];
		uint32_t head_intervals[3];
		uint32_t head_decay_intervals[3];
		uint32_t tail_intervals[3];
		uint32_t tail_decay_intervals[3];
		uint32_t texture_id;
		uint32_t squirt;
		uint32_t priority_plane;
		uint32_t replaceable_id; // for Wisp team color particles
	};

	struct RibbonEmitter : NodeContainer {
		float height_above;
		float height_below;
		float alpha;
		glm::vec3 color;
		float life_span;
		uint32_t texture_slot;
		uint32_t emission_rate;
		uint32_t rows;
		uint32_t columns;
		uint32_t material_id; // note: not a texture id, avoids need for filtermode field like PE2
		float gravity;
	};
	
	/*
	 EventObjects:
	 The type of sound or spawned effect is determined by node name
	 and SLK table lookup. The default World Editor ignores EventObjects
	 entirely, so they are only viewable in game. Even when you activate
	 the GEM setting in the Terrain Editor and listen for unit death sounds
	 when deleting them, the World Editor plays the sound file from the soundset
	 information despite the game playing the sound file from the EventObject.
	
	 Every EventObject's name is typically 8 characters. It usually starts with:
	    SPN to spawn a model file from "Splats\SpawnData.slk"
	       - Example: illidan footprints, blood particle emitters
	    SPLT to spawn a ground texture from "Splats\SplatData.slk"
			 - Example: blood ground texture on unit death
	    FPT to spawn a footprint also from "Splats\SplatData.slk"
	       - It is possible that FPT animates differently,
	         such as only shows on certain terrain?? (ToDo research if needed)
	       - Some FPT entries make situational sounds, such as spiders walking on metallic
	         tiles (icecrown tileset bricks/runes) if memory serves? (ToDo research if needed)
	    UBR to spawn a temporary uber splat from "Splats\UberSplatData.slk"
	       - Example: Several buildings, when they die, use an UBR tag to create a crater
			   style ground texture. Flamestrike also uses this style of model tag
	         to spawn its ground texture.
	    SND to play a sound from "UI\SoundInfo\AnimLookups.slk" (unit death and spell sounds)
	
	 The 4 last characters of the 8-character name will be the 4-digit rawcode
	 SLK table lookup key within the particular table being used.
	 RoC Beta had 5-digit rawcodes present in the UberSplatData.slk
	 that I did not research. They were probably just a different way
	 to store terrain information; I do not know if they were used in any
	 model files. The uber splat table is also used by the World Editor
	 for building Ground Textures, so it is possible that the World Editor
	 accepts the five letter codes and this allowed Blizzard to test the
	 different per-tileset variations of the entries.
	
	 For 3-letter table names, like FPT, the 4th character is often "x" or "y",
	 presumably a redundant indicator for left or right, although both
	 flipped versions of most footprint textures exist and are loaded with
	 separate table entries.
	
	 Tags like "SNDxPOOP" might exist for an SLK table entry we do not have that is only
	 found in custom environments. Although modern maps cannot easily override
	 these tables, old MPQ mods on historic versions of the game exist with
	 fan created table entries. For example, in the TToR Mod, when you
	 kill a Balrog it makes a very loud custom noise that is embedded into
	 the model via a custom EventObject. So we must allow invalid entries without crashing.
	 If the "times" vector is empty, then War3 does not load the model, and we can consider
	 it invalid.
	*/
	struct EventObject : NodeContainer {
		uint32_t count;
		int global_sequence_id; // signed, -1 to specify "none"
		std::vector<uint32_t> times;
	};

	enum class CollisionShapeType {
		Box = 0, // 2 verts
		Plane = 1, // 2 verts
		Sphere = 2, // 1 verts
		Cylinder = 3 // 2 vert
	};

	/*
		I was pretty sure that in the old days, not having a CollisionShape meant that
		a unit was not able to be selected in-game. However, at some point,
		I think they patched it so that it usually always works. Might've been
		the 2009 patch cycle, could have been TFT.

		So at this point I've seen some models that didn't have collision shapes that
		worked fine, but we need to parse them for rendering since they are legal
		nodes and could technically be a parent of another node.

		(They are used for ray intersection bounding cues, not for in-game "collision")
		(World editor doesn't use them and uses MODL/GEOS for selection and bounding cues, which
		is also why you don't ever have a doodad model that cant be clicked on in WE,
		even though some doodad models can't be clicked on easily in-game)
	
		Voidwalker's attack animation animates his CollisionShape to float outside
		of his center and DracoL1ch said that for DotA at some point he had to
		replace the Voidwalker model because of user complaints where the Voidwalker
		model had "invincibility frames" effectively, where it could not be attacked,
		because the CollisionShape had floated away.
	*/
	struct CollisionShape : NodeContainer {
		CollisionShapeType type;
		float vertices[2][3]; // sometimes only 1 is used
		float radius; // used for sphere/cylinder
	};

	struct Chunk {
		virtual ~Chunk() = default;
	};

	// ToDo consider an approach where an instance
	// of MDX struct has many std::vectors and does
	// not require a map lookup followed by dereference
	// of Chunk pointer to query information. In MatrixEater,
	// class MDL is a container for a bunch of array lists.
	// They are not stored in a map, I had no idea about
	// gross 4-letter MDX naming. They are an exhaustive
	// list of class members. Compiled class member lookup
	// is likely to outperform hashmap?
	class VERS : public Chunk {
		uint32_t version;

		static const ChunkTag tag = ChunkTag::VERS;
	};

	struct SEQS : Chunk {
		explicit SEQS(BinaryReader& reader);

		static const ChunkTag tag = ChunkTag::SEQS;
		std::vector<Sequence> sequences;
	};

	struct GLBS : Chunk {
		explicit GLBS(BinaryReader& reader);

		static const ChunkTag tag = ChunkTag::GLBS;
		std::vector<uint32_t> global_sequences;
	};

	struct GEOS : Chunk {
		explicit GEOS(BinaryReader& reader);

		static const ChunkTag tag = ChunkTag::GEOS;
		std::vector<Geoset> geosets;
	};

	struct GEOA : Chunk {
		explicit GEOA(BinaryReader& reader);

		static const ChunkTag tag = ChunkTag::GEOA;
		std::vector<GeosetAnimation> animations;
	};

	struct TEXS : Chunk {
		explicit TEXS(BinaryReader& reader);

		static const ChunkTag tag = ChunkTag::TEXS;
		std::vector<Texture> textures;
	};

	struct MTLS : Chunk {
		explicit MTLS(BinaryReader& reader);

		static const ChunkTag tag = ChunkTag::MTLS;
		std::vector<Material> materials;
	};

	struct BONE : Chunk {
		explicit BONE(BinaryReader& reader);

		static const ChunkTag tag = ChunkTag::BONE;
		std::vector<Bone> bones;
	};

	struct LITE : Chunk {
		explicit LITE(BinaryReader& reader);

		static const ChunkTag tag = ChunkTag::LITE;
		std::vector<Light> lights;
	};

	struct HELP : Chunk {
		explicit HELP(BinaryReader& reader);

		static const ChunkTag tag = ChunkTag::HELP;
		std::vector<NodeContainer> bones;
	};

	struct ATCH : Chunk {
		explicit ATCH(BinaryReader& reader);

		static const ChunkTag tag = ChunkTag::ATCH;
		std::vector<Attachment> attachments;
	};

	struct PIVT : Chunk {
		explicit PIVT(BinaryReader& reader);

		static const ChunkTag tag = ChunkTag::PIVT;
		std::vector<glm::vec3> pivots;
	};

	struct PREM : Chunk {
		explicit PREM(BinaryReader& reader);

		static const ChunkTag tag = ChunkTag::PREM;
		std::vector<ParticleEmitter1> emitters;
	};

	struct PRE2 : Chunk {
		explicit PRE2(BinaryReader& reader);

		static const ChunkTag tag = ChunkTag::PRE2;
		std::vector<ParticleEmitter2> emitters;
	};

	struct RIBB : Chunk {
		explicit RIBB(BinaryReader& reader);

		static const ChunkTag tag = ChunkTag::RIBB;
		std::vector<RibbonEmitter> emitters;
	};

	struct EVTS : Chunk {
		explicit EVTS(BinaryReader& reader);

		static const ChunkTag tag = ChunkTag::EVTS;
		std::vector<EventObject> eventObjects;
	};

	struct CLID : Chunk {
		explicit CLID(BinaryReader& reader);

		static const ChunkTag tag = ChunkTag::CLID;
		std::vector<CollisionShape> collisionShapes;
	};

	class MDX {
		std::map<ChunkTag, std::shared_ptr<Chunk>> chunks;

	public:
		explicit MDX(BinaryReader& reader);
		void load(BinaryReader& reader);


		// parameters will be F(NodeContainer& node);
		void forEachNode(const std::function<void(NodeContainer&)>& lambda);

		template<typename T>
		std::shared_ptr<T> chunk() {
			static_assert(std::is_base_of<Chunk, T>::value, "T must inherit from Chunk");

			return std::dynamic_pointer_cast<T>(chunks[static_cast<ChunkTag>(T::tag)]);
		}

		template<typename T>
		bool has_chunk() {
			static_assert(std::is_base_of<Chunk, T>::value, "T must inherit from Chunk");

			return chunks.find(static_cast<ChunkTag>(T::tag)) != chunks.end();
		}
	};
}