#include "stdafx.h"


void SkeletalModelInstance::setupHierarchy(mdx::Node* parent)
{
	int parent_id = -1;
	if (parent) {
		parent_id = parent->object_id;
	}
	std::shared_ptr<mdx::PIVT> pivots = model->chunk<mdx::PIVT>();
	// ToDo ensure this use of lambda is auto freed (I have no idea).
	// Leaking is bad, if you leak, you are serving the Demon Lord.
	auto func = [parent_id, pivots, instance = this](mdx::NodeContainer& object) {
		if (object.node.parent_id == parent_id) {
			glm::vec3 pivot;
			int object_id = object.node.object_id;
			if (pivots && object_id < pivots->pivots.size()) {
				pivot = pivots->pivots[object_id];
			}
			else {
				pivot = glm::vec3(0, 0, 0);
			}
			std::shared_ptr<RenderNode> renderNode = instance->renderNodes[object_id];
			if (!renderNode) {
				renderNode = std::make_shared<RenderNode>(std::make_shared<mdx::NodeContainer>(object), pivot);
				if (parent_id != -1) {
					renderNode->parent = instance->renderNodes[parent_id];
				}
				else {
					renderNode->parent = nullptr;
				}
				instance->renderNodes[object_id] = renderNode;
				instance->sortedNodes.push_back(renderNode);
			}
			instance->setupHierarchy(&object.node);
		}
	};
	this->model->forEachNode(func);
}

SkeletalModelInstance::SkeletalModelInstance(std::shared_ptr<mdx::MDX> model)
{
	this->model = model;
	uint32_t nodeCount = 0;
	if (model->has_chunk<mdx::PIVT>()) {
		// Retrieve number of nodes
		nodeCount = model->chunk<mdx::PIVT>()->pivots.size();
	}
	//renderNodes.reserve(nodeCount); // We only need exactly this many array entries
	// ToDo: for each camera: add camera source node to renderNodes and sortedNodes
	setupHierarchy(nullptr);
	// ToDo: for each camera: add camera target node to renderNodes and sortedNodes
	this->sequence_index = 0;
}

void SkeletalModelInstance::updateLocation(glm::vec3 & position, float angle, glm::vec3& scale)
{
	//matrix = glm::translate(glm::mat4(1.f), position);
	//matrix = glm::scale(matrix, scale / 128.f);
	//matrix = glm::rotate(matrix, angle, glm::vec3(0, 0, 1));
	this->position = position;
	this->facingAngle = angle;
	glm::vec3 origin = glm::vec3(0, 0, 0);
	glm::vec3 stackScale = scale / 128.f;
	glm::vec3 axis = glm::vec3(0, 0, 1);
	glm::quat rotation = glm::angleAxis(angle, axis);
	inverseInstanceRotation.x = -rotation.x;
	inverseInstanceRotation.y = -rotation.y;
	inverseInstanceRotation.z = -rotation.z;
	inverseInstanceRotation.w = rotation.w;
	fromRotationTranslationScaleOrigin(rotation, position, stackScale, matrix, origin);
}

void SkeletalModelInstance::updateTimer()
{
	if (model->has_chunk<mdx::SEQS>() && sequence_index != -1) {
		auto start_time = std::chrono::high_resolution_clock::now();
		long long delta_ms = std::chrono::duration_cast<std::chrono::nanoseconds>(start_time - last_render_time).count() / 1000000;

		std::shared_ptr<mdx::SEQS> seqs = model->chunk<mdx::SEQS>();
		mdx::Sequence& sequence = seqs->sequences[sequence_index];
		int relative_frame = current_frame - sequence.interval_start;
		int animation_length = sequence.interval_end - sequence.interval_start;
		if (sequence.flags & 0x1 == 0x1) {
			relative_frame += delta_ms;
			if (relative_frame > animation_length) {
				relative_frame = animation_length;
			}
		}
		else {
			relative_frame = (relative_frame + delta_ms) % (animation_length);
		}
		current_frame = sequence.interval_start + relative_frame;
		updateNodes(false);
		
		last_render_time = start_time;

	}

	inverseCameraRotation =  camera->decomposed_rotation;
}

void SkeletalModelInstance::updateNodes(bool forced)
{
	if (sequence_index < 0) {
		// no animation loaded, render in base position
		for (std::vector<std::shared_ptr<RenderNode>>::iterator it = sortedNodes.begin(); it != sortedNodes.end(); ++it) {
			std::shared_ptr<RenderNode> node = *it;
			node->resetTransformation();
			node->worldMatrix = glm::mat4(1.0f);
		}
	}
	else {
		// update skeleton to position based on animation @ time
		for (std::vector<std::shared_ptr<RenderNode>>::iterator it = sortedNodes.begin(); it != sortedNodes.end(); ++it) {
			std::shared_ptr<RenderNode> node = *it;
			std::shared_ptr<RenderNode> parent = node->parent;
			float visibility;
			node->node->getVisibility(visibility, (*this));
			bool objectVisible = visibility >= MAGIC_RENDER_SHOW_CONSTANT;
			bool nodeVisible = forced || (!parent || parent->visible) && objectVisible;

			node->visible = nodeVisible;
			// Every node only needs to be updated if this is a forced update, or if both
			// the parent node and the
			// generic object corresponding to this node are visible.
			// Incoming messy code for optimizations!
			// --- All copied from Ghostwolf to Matrix Eater, then Matrix Eater to HiveWE
			if (nodeVisible) {
				bool wasDirty = false;
				// TODO variants
				glm::vec3& localLocation = node->localLocation;
				glm::quat& localRotation = node->localRotation;
				glm::vec3& localScale = node->localScale;

				// Only update the local data if there is a need to
				if (forced || true /* variants */) {
					// regarding variants comment (copied from Matrix Eater), see orig ghostwolf code
					// The idea would be not to update if the node has no animation in
					// current sequence, as I recall.
					wasDirty = true;

					// Translation
					if (forced || true /* variants */) {
						// ToDo: Using KGTR here is retarded, possibly. Matrix Eater used a virtual function
						// so that Camera's Target and Camera's Source could subclass the node thing
						// and give their own return values using their own tags to get the Translation.
						// Ghostwolf used "getTranslation()" similarly, but the typing in the JS is
						// hard for me to follow. I'm not actually sure how or where he's updating the
						// camera target. My Matrix Eater version of the camera stuff might still
						// have some bugs, too, but it needs to work for visual editing. Hjorleif
						// asked me to make him a model with an animated camera, like Druid of the Claw
						// Portrait model, and my tool was so unready that even after a weekend of hacking
						// the code I didn't deliver on it and have been forgetting for half a year since
						// then.
						// (The same is true for KGSC and KGRT further below but I will omit duplicate comments)
						node->node->getValue<glm::vec3>(localLocation, mdx::TrackTag::KGTR, (*this), TRANSLATION_IDENTITY);
					}

					// Rotation
					if (forced || true /* variants */) {
						node->node->getValue<glm::quat>(localRotation, mdx::TrackTag::KGRT, (*this), ROTATION_IDENTITY);
					}

					// Scale
					if (forced || true /* variants */) {
						node->node->getValue<glm::vec3>(localScale, mdx::TrackTag::KGSC, (*this), SCALE_IDENTITY);
					}

					node->dirty = true;
				}

				// Billboarding:
				// If the instance is not attached to any scene, this is meaningless
				// ---
				// This is copied from Matrix Eater billboarding which has bugs.
				// Either it wasn't copied from Ghostwolf or he changed his repo online.
				// It is no longer present at this point in "updateNodes" in modelinstance.js
				if (node->billboarded || node->billboardedX) {
					wasDirty = true;

					// Cancel the parent's rotation
					if (parent) {
						localRotation = parent->inverseWorldRotation * inverseInstanceRotation;
					}
					else {
						localRotation = inverseInstanceRotation;
					}

					localRotation *= inverseCameraRotation;
				}
				//else if (node->billboardedY) {
				//	wasDirty = true;

					// Cancel the parent's rotation
				//	localRotation = ROTATION_IDENTITY;
					// The following is simply incorrect, but is copied from
					// the Matrix Eater, and provides a good temporary heuristic
					// until we get the correct code.
				//	localRotation *= inverseCameraRotationYSpin;
				//}
				//else if (node->billboardedZ) {
					//wasDirty = true;

					//if (parent) {
					//	localRotation = parent->inverseWorldRotation;
					//}
					//else {
					//	localRotation = ROTATION_IDENTITY;
					//}

					//localRotation *= inverseCameraRotationZSpin;
				//}

				bool wasReallyDirty = forced || wasDirty || !parent || parent->wasDirty;
				node->wasDirty = wasReallyDirty;

				// If this is a forced upate, or this node's local data was updated, or the
				// parent node updated, do a full world update.

				if (wasReallyDirty) {
					node->recalculateTransformation();
				}

				// If there is an instance object associated with this node, and the node is
				// visible (which might not be the case for a forced update!), update the object.
				// This includes attachments and emitters.

				// let object = node.object;
				if (objectVisible) {
					node->update(nullptr);
				}

				node->updateChildren(nullptr);
			}

		}
	}
}
