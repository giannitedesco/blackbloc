/*
 * md5anim.c -- md5mesh model loader + animation
 * last modification: aug. 14, 2007
 *
 * Doom3's md5mesh viewer with animation.  Animation portion.
 * Dependences: md5model.h, md5mesh.c.
 *
 * Copyright (c) 2005-2007 David HENRY
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * gcc -Wall -ansi -lGL -lGLU -lglut md5anim.c md5anim.c -o md5model
 */

#include <blackbloc/blackbloc.h>
#include <blackbloc/textreader.h>
#include <blackbloc/vector.h>
#include <blackbloc/tex.h>
#include <blackbloc/model/md5.h>

#include <stdio.h>
#include "md5.h"

/* Joint info */
struct joint_info_t {
	char name[64];
	int parent;
	int flags;
	int startIndex;
};

/* Base frame joint */
struct baseframe_joint_t {
	vec3_t pos;
	quat4_t orient;
};

/**
 * Check if an animation can be used for a given model.  Model's
 * skeleton and animation's skeleton must match.
 */
int
CheckAnimValidity(const struct md5_mesh *mdl, const struct md5_anim *anim)
{
	unsigned int i;

	/* md5mesh and md5anim must have the same number of joints */
	if (mdl->num_joints != anim->num_joints)
		return 0;

	/* We just check with frame[0] */
	for (i = 0; i < mdl->num_joints; ++i) {
		/* Joints must have the same parent index */
		if (mdl->baseSkel[i].parent != anim->skelFrames[0][i].parent)
			return 0;

		/* Joints must have the same name */
		if (strcmp(mdl->baseSkel[i].name, anim->skelFrames[0][i].name)
		    != 0)
			return 0;
	}

	return 1;
}

/**
 * Build skeleton for a given frame data.
 */
static void
BuildFrameSkeleton(const struct joint_info_t *jointInfos,
		   const struct baseframe_joint_t *baseFrame,
		   const float *animFrameData,
		   struct md5_joint_t *skelFrame, int num_joints)
{
	int i;

	for (i = 0; i < num_joints; ++i) {
		const struct baseframe_joint_t *baseJoint = &baseFrame[i];
		vec3_t animatedPos;
		quat4_t animatedOrient;
		int j = 0;

		memcpy(animatedPos, baseJoint->pos, sizeof(vec3_t));
		memcpy(animatedOrient, baseJoint->orient, sizeof(quat4_t));

		if (jointInfos[i].flags & 1) {	/* Tx */
			animatedPos[0] =
			    animFrameData[jointInfos[i].startIndex + j];
			++j;
		}

		if (jointInfos[i].flags & 2) {	/* Ty */
			animatedPos[1] =
			    animFrameData[jointInfos[i].startIndex + j];
			++j;
		}

		if (jointInfos[i].flags & 4) {	/* Tz */
			animatedPos[2] =
			    animFrameData[jointInfos[i].startIndex + j];
			++j;
		}

		if (jointInfos[i].flags & 8) {	/* Qx */
			animatedOrient[0] =
			    animFrameData[jointInfos[i].startIndex + j];
			++j;
		}

		if (jointInfos[i].flags & 16) {	/* Qy */
			animatedOrient[1] =
			    animFrameData[jointInfos[i].startIndex + j];
			++j;
		}

		if (jointInfos[i].flags & 32) {	/* Qz */
			animatedOrient[2] =
			    animFrameData[jointInfos[i].startIndex + j];
			++j;
		}

		/* Compute orient quaternion's w value */
		Quat_computeW(animatedOrient);

		/* NOTE: we assume that this joint's parent has
		   already been calculated, i.e. joint's ID should
		   never be smaller than its parent ID. */
		struct md5_joint_t *thisJoint = &skelFrame[i];

		int parent = jointInfos[i].parent;
		thisJoint->parent = parent;
		strcpy(thisJoint->name, jointInfos[i].name);

		/* Has parent? */
		if (thisJoint->parent < 0) {
			memcpy(thisJoint->pos, animatedPos, sizeof(vec3_t));
			memcpy(thisJoint->orient, animatedOrient,
			       sizeof(quat4_t));
		} else {
			struct md5_joint_t *parentJoint = &skelFrame[parent];
			vec3_t rpos;	/* Rotated position */

			/* Add positions */
			Quat_rotatePoint(parentJoint->orient, animatedPos,
					 rpos);
			thisJoint->pos[0] = rpos[0] + parentJoint->pos[0];
			thisJoint->pos[1] = rpos[1] + parentJoint->pos[1];
			thisJoint->pos[2] = rpos[2] + parentJoint->pos[2];

			/* Concatenate rotations */
			Quat_multQuat(parentJoint->orient, animatedOrient,
				      thisJoint->orient);
			Quat_normalize(thisJoint->orient);
		}
	}
}

/**
 * Load an MD5 animation from file.
 */
int ReadMD5Anim(const char *filename, struct md5_anim *anim)
{
	struct gfile f;
	textreader_t txt;
	char *line;
	struct joint_info_t *jointInfos = NULL;
	struct baseframe_joint_t *baseFrame = NULL;
	float *animFrameData = NULL;
	unsigned int version, numAnimatedComponents;
	unsigned int i, frame_index;
	int ret = 0;

	if ( !game_open(&f, filename) ) {
		con_printf("md5: error: couldn't open \"%s\"!\n", filename);
		return 0;
	}

	txt = textreader_new(f.f_ptr, f.f_len);
	if ( NULL == txt )
		goto out_close;

	while( (line = textreader_gets(txt)) ) {
		if (sscanf(line, " MD5Version %d", &version) == 1) {
			if (version != 10) {
				/* Bad version */
				con_printf("Error: bad animation version\n");
				goto out;
			}
		} else if (sscanf(line, " numFrames %d", &anim->num_frames) ==
			   1) {
			/* Allocate memory for skeleton frames and bounding boxes */
			if (anim->num_frames > 0) {
				anim->skelFrames = (struct md5_joint_t **)
				    malloc(sizeof(struct md5_joint_t *) *
					   anim->num_frames);
				anim->bboxes = (struct md5_bbox_t *)
				    malloc(sizeof(struct md5_bbox_t) *
					   anim->num_frames);
			}
		} else if (sscanf(line, " numJoints %d", &anim->num_joints) ==
			   1) {
			if (anim->num_joints > 0) {
				for (i = 0; i < anim->num_frames; ++i) {
					/* Allocate memory for joints of each frame */
					anim->skelFrames[i] =
					    (struct md5_joint_t *)
					    malloc(sizeof(struct md5_joint_t) *
						   anim->num_joints);
				}

				/* Allocate temporary memory for building skeleton frames */
				jointInfos = (struct joint_info_t *)
				    malloc(sizeof(struct joint_info_t) *
					   anim->num_joints);

				baseFrame = (struct baseframe_joint_t *)
				    malloc(sizeof(struct baseframe_joint_t) *
					   anim->num_joints);
			}
		} else if (sscanf(line, " frameRate %d", &anim->frameRate) == 1) {
			/*
			   printf ("md5anim: animation's frame rate is %d\n", anim->frameRate);
			 */
		} else
		    if (sscanf
			(line, " numAnimatedComponents %d",
			 &numAnimatedComponents) == 1) {
			if (numAnimatedComponents > 0) {
				/* Allocate memory for animation frame data */
				animFrameData =
				    (float *)malloc(sizeof(float) *
						    numAnimatedComponents);
			}
		} else if (strncmp(line, "hierarchy {", 11) == 0) {
			for (i = 0; i < anim->num_joints; ++i) {
				/* Read whole line */
				line = textreader_gets(txt);

				/* Read joint info */
				sscanf(line, " %s %d %d %d", jointInfos[i].name,
				       &jointInfos[i].parent,
				       &jointInfos[i].flags,
				       &jointInfos[i].startIndex);
			}
		} else if (strncmp(line, "bounds {", 8) == 0) {
			for (i = 0; i < anim->num_frames; ++i) {
				/* Read whole line */
				line = textreader_gets(txt);

				/* Read bounding box */
				sscanf(line, " ( %f %f %f ) ( %f %f %f )",
				       &anim->bboxes[i].min[0],
				       &anim->bboxes[i].min[1],
				       &anim->bboxes[i].min[2],
				       &anim->bboxes[i].max[0],
				       &anim->bboxes[i].max[1],
				       &anim->bboxes[i].max[2]);
			}
		} else if (strncmp(line, "baseframe {", 10) == 0) {
			for (i = 0; i < anim->num_joints; ++i) {
				/* Read whole line */
				line = textreader_gets(txt);

				/* Read base frame joint */
				if (sscanf(line, " ( %f %f %f ) ( %f %f %f )",
					   &baseFrame[i].pos[0],
					   &baseFrame[i].pos[1],
					   &baseFrame[i].pos[2],
					   &baseFrame[i].orient[0],
					   &baseFrame[i].orient[1],
					   &baseFrame[i].orient[2]) == 6) {
					/* Compute the w component */
					Quat_computeW(baseFrame[i].orient);
				}
			}
		} else if (sscanf(line, "frame %d {", &frame_index) == 1) {
			int x, y;
			for(i = 0;;) {
				char *tok[numAnimatedComponents - i];
				line = textreader_gets(txt);
				if ( NULL == line || line[0] == '}' )
					break;
				x = easy_explode(line, 0, tok,
						sizeof(tok)/sizeof(*tok));
				for(y = 0; y < x; y++, i++) {
					sscanf(tok[y], "%f", &animFrameData[i]);
				}
			}

			/* Build frame skeleton from the collected data */
			BuildFrameSkeleton(jointInfos, baseFrame, animFrameData,
					   anim->skelFrames[frame_index],
					   anim->num_joints);
		}
	}

	/* Free temporary data allocated */
	if (animFrameData)
		free(animFrameData);

	if (baseFrame)
		free(baseFrame);

	if (jointInfos)
		free(jointInfos);

	ret = 1;

out:
	textreader_free(txt);
out_close:
	game_close(&f);
	return ret;
}

/**
 * Free resources allocated for the animation.
 */
void FreeAnim(struct md5_anim *anim)
{
	unsigned int i;

	if (anim->skelFrames) {
		for (i = 0; i < anim->num_frames; ++i) {
			free(anim->skelFrames[i]);
		}
		free(anim->skelFrames);
	}

	free(anim->bboxes);
	free(anim->name);
	list_del(&anim->list);
	free(anim);
}

static LIST_HEAD(md5_anims);

static struct md5_anim *do_mesh_load(const char *filename)
{
	struct md5_anim *anim;

	anim = calloc(1, sizeof(*anim));
	if ( NULL == anim )
		return NULL;

	/* Load MD5 model file */
	if (!ReadMD5Anim(filename, anim))
		goto err;

	anim->name = strdup(filename);
	if ( NULL == anim->name )
		goto err;

	con_printf("md5: anim loaded: %s\n", anim->name);

	anim->ref = 1;
	list_add_tail(&anim->list, &md5_anims);
	return anim;
err:
	FreeAnim(anim);
	free(anim->name);
	free(anim);
	return NULL;
}

struct md5_anim *md5_anim_get_by_name(const char *filename)
{
	struct md5_anim  *anim;

	list_for_each_entry(anim, &md5_anims, list) {
		if ( !strcmp(filename, anim->name) ) {
			anim->ref++;
			return anim;
		}
	}

	return do_mesh_load(filename);
}

/**
 * Smoothly interpolate two skeletons
 */
void
InterpolateSkeletons(const struct md5_joint_t *skelA,
		     const struct md5_joint_t *skelB,
		     int num_joints, float interp, struct md5_joint_t *out)
{
	int i;

	for (i = 0; i < num_joints; ++i) {
		/* Copy parent index */
		out[i].parent = skelA[i].parent;

		/* Linear interpolation for position */
		out[i].pos[0] =
		    skelA[i].pos[0] + interp * (skelB[i].pos[0] -
						skelA[i].pos[0]);
		out[i].pos[1] =
		    skelA[i].pos[1] + interp * (skelB[i].pos[1] -
						skelA[i].pos[1]);
		out[i].pos[2] =
		    skelA[i].pos[2] + interp * (skelB[i].pos[2] -
						skelA[i].pos[2]);

		/* Spherical linear interpolation for orientation */
		Quat_slerp(skelA[i].orient, skelB[i].orient, interp,
			   out[i].orient);
	}
}

void md5_anim_put(struct md5_anim *anim)
{
	if ( anim ) {
		assert(anim->ref);
		if ( 0 == --anim->ref ) {
			FreeAnim(anim);
		}
	}
}
