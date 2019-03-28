#pragma once

/**
  * Node in skeletal hierarcy.
  * Based on RenderNode class in MatrixEater
  * that was based on "src/viewer/node.js" in Ghostwolf's
  * viewer.
  */
class RenderNode {

public:
	RenderNode(std::shared_ptr<mdx::NodeContainer> node, glm::vec3 pivot);
	std::shared_ptr<mdx::NodeContainer> node;
	std::shared_ptr<RenderNode> parent;
	glm::vec3 pivot;
	// local space
	glm::vec3 localLocation;
	glm::quat localRotation = glm::quat(1.f, 0.f, 0.f, 0.f);
	glm::vec3 localScale = glm::vec3(1, 1, 1);
	glm::mat4 localMatrix = glm::mat4(1.f);
	// world space (not including game unit X/Y, mdx-m3-viewer's name)
	glm::vec3 worldLocation;
	glm::quat worldRotation = glm::quat(1.f, 0.f, 0.f, 0.f);
	glm::vec3 worldScale = glm::vec3(1, 1, 1);
	glm::mat4 worldMatrix = glm::mat4(1.f);
	// inverse world space
	glm::vec3 inverseWorldLocation;
	glm::quat inverseWorldRotation;
	glm::vec3 inverseWorldScale;
	// cached flags from node, could be removed for RAM
	boolean dontInheritScaling;
	// for some reason MatrixEater didn't have
	// dontInheritTranslation and dontInheritRotation????
	boolean billboarded;
	boolean billboardedX;
	boolean billboardedY;
	boolean billboardedZ;
	// state flags
	boolean visible;
	boolean dirty;
	boolean wasDirty;
	// not in matrixeater
	std::vector<std::shared_ptr<RenderNode>> children;

	void setTransformation(glm::vec3 location, glm::quat rotation, glm::vec3 scale);
	void resetTransformation();
	void recalculateTransformation();
	// void* is a thing from javascript we don't know
	// what it is yet, still reading, just pass null
	// until we need it
	void update(void* scene);
	void updateChildren(void* scene);
	virtual void updateObject(void* scene);
};