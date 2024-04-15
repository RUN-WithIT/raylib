#ifndef _RCAMERA_XTRA
#define _RCAMERA_XTRA

typedef struct Camera3DXtra {
  Camera3D camera;
  int mode;
  int ignore_gesture;
  int ignore_scroll;
  int ignore_rotate;
  int ignore_pan;
} CameraXtra;

#endif
