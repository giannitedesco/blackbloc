//
//                 Half-Life Model Viewer (c) 1999 by Mete Ciragan
//
// file:           ViewerSettings.h
// last modified:  May 29 1999, Mete Ciragan
// copyright:      The programs and associated files contained in this
//                 distribution were developed by Mete Ciragan. The programs
//                 are not in the public domain, but they are freely
//                 distributable without licensing fees. These programs are
//                 provided without guarantee or warrantee expressed or
//                 implied.
//
// version:        1.2
//
// email:          mete@swissquake.ch
// web:            http://www.swissquake.ch/chumbalum-soft/
//
#ifndef INCLUDED_VIEWERSETTINGS
#define INCLUDED_VIEWERSETTINGS

enum // render modes
{
	RM_WIREFRAME,
	RM_FLATSHADED,
	RM_SMOOTHSHADED,
	RM_TEXTURED
};



typedef struct
{
	// model 
	float rot[3];
	float trans[3];

	// render
	int renderMode;
	float transparency;
	unsigned int showBackground:1;
	unsigned int showHitBoxes:1;
	unsigned int showBones:1;
	unsigned int showTexture:1;
	unsigned int showAttachments:1;
	unsigned int mirror:1;
	unsigned int useStencil:1;	// if 3dfx fullscreen set false
	unsigned int use3dfx:1;
	unsigned int cds:1;
	unsigned int pause:1;
	int texture;
	float textureScale;
	int skin;

	// animation
	int sequence;
	float speedScale;

	// bodyparts and bonecontrollers
	int submodels[32];
	float controllers[8];
} ViewerSettings;

extern ViewerSettings g_viewerSettings;

#endif // INCLUDED_VIEWERSETTINGS
