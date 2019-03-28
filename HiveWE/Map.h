#pragma once

class Map {
public:
	bool loaded = false;

	TriggerStrings trigger_strings;
	Triggers triggers;
	MapInfo info;
	Terrain terrain;
	TerrainUndo terrain_undo;
	PathingMap pathing_map;
	Imports imports;
	Doodads doodads;
	Units units;
	Regions regions;
	GameCameras cameras;
	Sounds sounds;

	Brush* brush = nullptr;

	bool is_protected = false;
	bool units_loaded = false;

	bool enforce_water_height_limits = true;

	bool render_doodads = true;
	bool render_units = true;
	bool render_pathing = true;
	bool render_brush = true;
	bool render_lighting = true;
	bool render_wireframe = false;
	bool render_debug = false;

	fs::path filesystem_path;

	// For instancing
	std::vector<StaticMesh*> meshes;
	std::vector<AnimatedMesh*> animatedMeshes;

	double terrain_time;
	double terrain_tiles_time;
	double terrain_water_time;
	double terrain_cliff_time;
	double mouse_world_time;
	double doodad_time;
	double unit_time;
	double render_time;
	double total_time;
	double total_time_min = std::numeric_limits<double>::max();
	double total_time_max = std::numeric_limits<double>::min();

	~Map();

	void load(const fs::path& path);
	bool save(const fs::path& path, bool switch_working = true);
	void play_test();
	void render(int width, int height);
};