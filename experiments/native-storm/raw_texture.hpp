#pragma once
#include <d3d9.h>
HRESULT LoadNativeRawTexture(IDirect3DDevice9 *device, const char *path, IDirect3DTexture9 **out);
