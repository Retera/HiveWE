#pragma once

// Ghostwolf mentioned this to me once, so I used it,
// as 0.75, experimentally determined as a guess at
// whatever WC3 is doing. Do more reserach if necessary?
#define MAGIC_RENDER_SHOW_CONSTANT 0.75

class SkeletalModelInstance {
private:
	void setupHierarchy(mdx::Node* node);

public:
	std::shared_ptr<mdx::MDX> model;
	int sequence_index; // can be -1 if not animating
	int current_frame;
	explicit SkeletalModelInstance(std::shared_ptr<mdx::MDX> model);
	std::vector<std::shared_ptr<RenderNode>> sortedNodes;

	glm::quat inverseCameraRotation = glm::quat(0.f, 0.f, 0.f, 1.0f);
	glm::quat inverseCameraRotationXSpin;
	glm::quat inverseCameraRotationYSpin;
	glm::quat inverseCameraRotationZSpin;

	glm::mat4 matrix = glm::mat4(1.f);
	glm::vec3 position;
	float facingAngle;
	glm::quat inverseInstanceRotation;


	std::chrono::steady_clock::time_point last_render_time;

	void updateLocation(glm::vec3& position, float angle, glm::vec3& scale);

	void updateTimer();

	void updateNodes(bool forced);

	// ToDo "renderNodes" should be a fixed sized dynamic array
	// but I'm afraid to do the RenderNode* that I was taught in
	// class with the weird static shared ptrs so we are doing
	// int to object mapping. Map should always be fully populated
	// with [0-N] keys where N is the maximum ObjectId value
	// in the MDL.
	std::map<int, std::shared_ptr<RenderNode>> renderNodes;
};