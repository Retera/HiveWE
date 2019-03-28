#include "stdafx.h"

RenderNode::RenderNode(std::shared_ptr<mdx::NodeContainer> node, glm::vec3 pivot)
{
	this->pivot = pivot;
	this->node = node;

	this->dontInheritScaling = node->node.flags & 0x1;
	this->billboarded = node->node.flags & 0x8;
	this->billboardedX = node->node.flags & 0x10;
	this->billboardedY = node->node.flags & 0x20;
	this->billboardedZ = node->node.flags & 0x40;
}

void RenderNode::setTransformation(glm::vec3 location, glm::quat rotation, glm::vec3 scale)
{
	localLocation = location;
	localRotation = rotation;
	localScale = scale;

	dirty = true;
}

void RenderNode::resetTransformation() {
	// C++ programmer should code review this,
	// is this best way to zero a vec3?
	localLocation = glm::vec3(0, 0, 0);
	localRotation = glm::quat(1, 0, 0, 0);
	localScale = glm::vec3(1, 1, 1);
	dirty = true;
}

void RenderNode::recalculateTransformation()
{
	boolean dirty = this->dirty;
	std::shared_ptr<RenderNode> parent = this->parent;

	// Need to update if this node is dirty, or if its parent was dirty.
	this->wasDirty = this->dirty;

	if (parent) {
		dirty = dirty || parent->wasDirty;
	}

	this->wasDirty = dirty;

	// Matrix Eater functionality begins below.
	// Everything above was only in the JS.
	// Apparently I completely removed and omitted all logic
	// relating to dirty nodes, perhaps because I wanted
	// correct behavior and did not want to implement the
	// performance optimization that Ghostwolf and I discussed
	// about dirty nodes. Shouldn't that mean the ME impl is buggy?
	// Surely I didn't invent some way to omit the above?

	// ToDo remove above comment for being too verbose.

	if (dirty) {
		this->dirty = false;

		if (parent) {
			glm::vec3 computedLocation;
			glm::vec3 computedScaling;
			computedLocation = localLocation;

			// ToDo Ghostwolf has some commented-out code
			// here to process "dontInheritRotation"
			// and "dontInheritTranslation" flags.
			// Matrix Eater completely skipped it so it's probably
			// bugged, too??? Needs a test.

			if (this->dontInheritScaling) {
				glm::vec3 parentInverseScale = parent->inverseWorldScale;
				computedScaling = parentInverseScale * localScale;

				worldScale = localScale;
			}
			else {
				computedScaling = localScale;

				glm::vec3 parentScale = parent->worldScale;
				worldScale = parentScale * localScale;
			}
			fromRotationTranslationScaleOrigin(localRotation, computedLocation, computedScaling, localMatrix, pivot);

			worldMatrix = parent->worldMatrix * localMatrix;

			worldRotation = parent->worldRotation * localRotation;
			//quatMul(parent->worldRotation, localRotation, worldRotation);
		}
		else {
			fromRotationTranslationScaleOrigin(localRotation, localLocation, localScale, localMatrix, pivot);
			worldMatrix = localMatrix;
			worldRotation = localRotation;
			worldScale = localScale;
		}

		// Inverse world rotation (ToDo use overloaded assignment operator)
		inverseWorldRotation.x = -worldRotation.x;
		inverseWorldRotation.y = -worldRotation.y;
		inverseWorldRotation.z = -worldRotation.z;
		inverseWorldRotation.w = worldRotation.w;

		// Inverse world scale
		inverseWorldScale.x = 1 / worldScale.x;
		inverseWorldScale.y = 1 / worldScale.y;
		inverseWorldScale.z = 1 / worldScale.z;

		// World location
		worldLocation.x = worldMatrix[3][0];
		worldLocation.y = worldMatrix[3][1];
		worldLocation.z = worldMatrix[3][2];

		// Inverse world location
		inverseWorldLocation.x = -worldLocation.x;
		inverseWorldLocation.y = -worldLocation.y;
		inverseWorldLocation.z = -worldLocation.z;
	}
}

void RenderNode::update(void * scene)
{
	if (this->dirty || (parent && parent->wasDirty)) {
		this->dirty = true; // In this case this node isn't dirty, but the parent was
		this->wasDirty = true;
		this->recalculateTransformation();
	}
	else {
		this->wasDirty = false;
	}

	this->updateObject(scene);

	this->updateChildren(scene);
}

void RenderNode::updateChildren(void * scene)
{
	for (std::vector<std::shared_ptr<RenderNode>>::iterator it = children.begin(); it != children.end(); ++it) {
		std::shared_ptr<RenderNode> childNode = *it;
		// MatrixEater has error trapping and always blows up here after editing models,
		// because cached list of children becomes out of date
		childNode->update(scene);
	}
}

void RenderNode::updateObject(void * scene)
{
	// for subclassing
}
