#pragma once

// templates
template<typename T>
inline void mdx::NodeContainer::getValue(T & out, mdx::TrackTag tag, SkeletalModelInstance & instance, const T & defaultValue)
{
	std::shared_ptr<TrackHeader<T>> header = node.animated_data.track<T>(tag);
	if (header) {
		header->matrixEaterInterpolate(out, instance.current_frame, instance, defaultValue);
	}
	else {
		out = defaultValue;
	}
}
template<typename T>
int mdx::TrackHeader<T>::ceilIndex(int frame, mdx::Sequence& sequence)
{
	// ToDo fix, this function does not work yet.
	int trackSize = tracks.size();
	if (frame < sequence.interval_start) {
		return -1;
	}
	else if (frame >= sequence.interval_end) {
		return trackSize;
	}
	else {
		for (int i = 1; i < trackSize; i++) {
			Track<T>& track = tracks[i];
			if (track.frame > frame) {
				return i;
			}
		}
		return -1;
	}
	return 0;
}
template<typename T>
void mdx::TrackHeader<T>::getValue(T & out, int frame, mdx::Sequence& sequence, const T & defaultValue)
{
	// ToDo fix, this function does not work yet.
	int index = ceilIndex(frame, sequence);
	int length = tracks.size();
	if (index == -1) {
		out = tracks[0].value;
	}
	else if (index == length) {
		out = tracks[length - 1].value;
	}
	else {
		Track<T>& start = tracks[index - 1];
		Track<T>& end = tracks[index];
		float t = clampValue((frame - start.frame) / (end.frame - start.frame), 0, 1);
	}
}

/* Matrix Eater interpolate is slower than ghostwolf interpolate,
   because this one does more CPU operations. But it does not
   require modifying the model on load to inject additional keyframes
*/
template<typename T>
void mdx::TrackHeader<T>::matrixEaterInterpolate(T & out, int time, SkeletalModelInstance & instance, const T & defaultValue)
{
	int sequenceStart;
	int sequenceEnd;
	if (global_sequence_ID >= 0 && instance.model->has_chunk<mdx::GLBS>()) {
		sequenceStart = 0;
		sequenceEnd = instance.model->chunk<mdx::GLBS>()->global_sequences[global_sequence_ID];
		if (sequenceEnd == 0) {
			time = 0;
		}
		else {
			time = (int)(std::chrono::duration_cast<std::chrono::milliseconds>(instance.last_render_time.time_since_epoch()).count() % sequenceEnd);
		}
	}
	else if (instance.model->has_chunk<SEQS>() && instance.sequence_index != -1) {
		std::shared_ptr<SEQS> seqs = instance.model->chunk<SEQS>();
		Sequence& sequence = seqs->sequences[instance.sequence_index];
		sequenceStart = sequence.interval_start;
		sequenceEnd = sequence.interval_end;
	}
	else {
		out = defaultValue;
		return;
	}
	if (tracks.empty()) {
		out = defaultValue;
		return;
	}
	int ceilIndex = -1;
	int floorIndex = 0;
	const T* floorInTan;
	const T* floorOutTan;
	const T* floorValue;
	const T* ceilValue;
	int floorIndexTime;
	int ceilIndexTime;
	// ToDo "if global seq" check is here in MXE java

	int floorAnimStartIndex = 0;
	int floorAnimEndIndex = 0;
	// get floor:
	int tracksSize = tracks.size();
	for (int i = 0; i < tracksSize; i++) {
		Track<T>& track = tracks[i];
		if (track.frame <= sequenceStart) {
			floorAnimStartIndex = i;
		}
		if (track.frame <= time) {
			floorIndex = i;
		}
		if (track.frame >= time && ceilIndex == -1) {
			ceilIndex = i;
		}
		if (track.frame <= sequenceEnd) {
			floorAnimEndIndex = i;
		}
		else {
			// end of our sequence
			break;
		}
	}
	if (ceilIndex == -1) {
		ceilIndex = tracksSize - 1;
	}
	// end get floor
	if (ceilIndex < floorIndex) {
		ceilIndex = floorIndex;
		// was a problem in matrix eater, different impl, not problem here?
	}
	floorValue = &tracks[floorIndex].value;
	if (interpolation_type > 1) {
		floorInTan = &tracks[floorIndex].inTan;
		floorOutTan = &tracks[floorIndex].outTan;
	}
	ceilValue = &tracks[ceilIndex].value;
	floorIndexTime = tracks[floorIndex].frame;
	ceilIndexTime = tracks[ceilIndex].frame;
	if (ceilIndexTime < sequenceStart) {
		out = defaultValue;
		return;
	}
	if (floorIndexTime > sequenceEnd) {
		out = defaultValue;
		return;
	}
	auto floorBeforeStart = floorIndexTime < sequenceStart;
	auto ceilAfterEnd = ceilIndexTime > sequenceEnd;
	if (floorBeforeStart && ceilAfterEnd) {
		out = defaultValue;
		return;
	}
	else if (floorBeforeStart) {
		if (tracks[floorAnimEndIndex].frame == sequenceEnd) {
			// no "floor" frame found, but we have a ceil frame,
			// so the prev frame is a repeat of animation's end
			// placed at the beginning
			floorIndex = floorAnimEndIndex;
			floorValue = &tracks[floorAnimEndIndex].value;
			floorIndexTime = sequenceStart;
			if (interpolation_type > 1) {
				floorInTan = &tracks[floorAnimEndIndex].inTan;
				floorOutTan = &tracks[floorAnimEndIndex].outTan;
			}
		}
		else {
			floorValue = &defaultValue;
			floorInTan = floorOutTan = &defaultValue;
			floorIndexTime = sequenceStart;
		}
	}
	else if (ceilAfterEnd || ceilIndexTime < time && tracks[floorAnimEndIndex].frame < time) {
		// if we have a floor frame but the "ceil" frame is after end of sequence,
		// or our ceil frame is before our time, meaning that we're at the end of the
		// entire timeline, then we need to inject a "ceil" frame at end of sequence
		if (tracks[floorAnimStartIndex].frame == sequenceStart) {
			ceilValue = &tracks[floorAnimStartIndex].value;
			ceilIndex = floorAnimStartIndex;
			ceilIndexTime = sequenceStart;
		}
		// for the else case here, Matrix Eater code says to leave it blank,
		// example model is Water Elemental's birth animation, to verify behavior
	}
	if (floorIndex == ceilIndex) {
		out = *floorValue;
		return;
	}
	const T* ceilInTan = &tracks[ceilIndex].inTan;
	float t = clampValue((time - floorIndexTime) / (float)(ceilIndexTime - floorIndexTime), 0.f, 1.f);
	interpolate<T>(out, floorValue, floorOutTan, ceilInTan, ceilValue, t, interpolation_type);
}
