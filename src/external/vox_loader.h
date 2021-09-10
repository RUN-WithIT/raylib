/*
   The MIT License (MIT)

   Copyright (c) 2021 Johann Nadalutti.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
   THE SOFTWARE.


	vox_loader - v1.01
			 no warranty implied; use at your own risk

	Do this:
	#define VOX_LOADER_INCLUDE__H
	before you include this file in* one* C or C++ file to create the implementation.

	// i.e. it should look like this:
	#include ...
	#include ...
	#include ...
	#define VOX_LOADER_INCLUDE__H
	#include "vox_loader.h"

revision history:
	1.00  (2021-09-03)	first released version
	1.01  (2021-09-07)	Support custom memory allocators
						Removed Raylib dependencies
						Changed Vox_LoadFileName to Vox_LoadFromMemory

*/


#ifndef VOX_LOADER_H
#define VOX_LOADER_H


#include <string.h>


#ifdef __cplusplus
extern "C" {
#endif

	#define VOX_SUCCESS (0)
	#define VOX_ERROR_FILE_NOT_FOUND (-1)
	#define VOX_ERROR_INVALID_FORMAT (-2)
	#define VOX_ERROR_FILE_VERSION_TOO_OLD (-3)

	// VoxColor, 4 components, R8G8B8A8 (32bit)
	typedef struct {
		unsigned char r, g, b, a;
	} VoxColor;

	// VoxVector3, 3 components
	typedef struct  {
		float x, y, z;
	} VoxVector3;


	typedef struct
	{
		int* array;
		int used, size;
	} ArrayInt;

	typedef struct
	{
		VoxVector3* array;
		int used, size;
	} ArrayVector3;

	typedef struct
	{
		VoxColor* array;
		int used, size;
	} ArrayColor;

	typedef struct
	{
		unsigned short* array;
		int used, size;
	} ArrayUShort;


	// A chunk that contain voxels
	typedef struct
	{
		unsigned char* m_array; //If Sparse != null
		int arraySize; //Size for m_array in bytes (DEBUG ONLY)
	} CubeChunk3D;

	// Array for voxels
	// Array is divised into chunks of CHUNKSIZE*CHUNKSIZE*CHUNKSIZE voxels size
	typedef struct
	{
		//Array size in voxels
		int sizeX;
		int sizeY;
		int sizeZ;

		//Chunks size into array (array is divised into chunks)
		int chunksSizeX;
		int chunksSizeY;
		int chunksSizeZ;

		//Chunks array
		CubeChunk3D* m_arrayChunks;
		int arrayChunksSize; //Size for m_arrayChunks in bytes (DEBUG ONLY)

		int ChunkFlattenOffset;
		int chunksAllocated;
		int chunksTotal;

		//Arrays for mesh build
		ArrayVector3 vertices;
		ArrayUShort indices;
		ArrayColor colors;

		//Palette for voxels
		VoxColor palette[256];

	} VoxArray3D;


	// Functions
	extern int Vox_LoadFromMemory(const unsigned char* pvoxData, unsigned int voxDataSize, VoxArray3D* pvoxarray);
	extern void Vox_FreeArrays(VoxArray3D* voxarray);


#ifdef __cplusplus
}
#endif



////   end header file   /////////////////////////////////////////////////////
#endif // VOX_LOADER_H



/////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
//									Implementation
///////////////////////////////////////////////////////////////////////////////////////////// 
/////////////////////////////////////////////////////////////////////////////////////////////

#ifdef VOX_LOADER_IMPLEMENTATION



/////////////////////////////////////////////////////////////////////////////////////////////
// ArrayInt helper
/////////////////////////////////////////////////////////////////////////////////////////////

void initArrayInt(ArrayInt* a, int initialSize)
{
	a->array = VOX_MALLOC(initialSize * sizeof(int));
	a->used = 0;
	a->size = initialSize;
}

void insertArrayInt(ArrayInt* a, int element)
{
	if (a->used == a->size)
	{
		a->size *= 2;
		a->array = VOX_REALLOC(a->array, a->size * sizeof(int));
	}
	a->array[a->used++] = element;
}

void freeArrayInt(ArrayInt* a)
{
	VOX_FREE(a->array);
	a->array = NULL;
	a->used = a->size = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// ArrayUShort helper
/////////////////////////////////////////////////////////////////////////////////////////////

void initArrayUShort(ArrayUShort* a, int initialSize)
{
	a->array = VOX_MALLOC(initialSize * sizeof(unsigned short));
	a->used = 0;
	a->size = initialSize;
}

void insertArrayUShort(ArrayUShort* a, unsigned short element)
{
	if (a->used == a->size)
	{
		a->size *= 2;
		a->array = VOX_REALLOC(a->array, a->size * sizeof(unsigned short));
	}
	a->array[a->used++] = element;
}

void freeArrayUShort(ArrayUShort* a)
{
	VOX_FREE(a->array);
	a->array = NULL;
	a->used = a->size = 0;
}


/////////////////////////////////////////////////////////////////////////////////////////////
// ArrayVector3 helper
/////////////////////////////////////////////////////////////////////////////////////////////

void initArrayVector3(ArrayVector3* a, int initialSize)
{
	a->array = VOX_MALLOC(initialSize * sizeof(VoxVector3));
	a->used = 0;
	a->size = initialSize;
}

void insertArrayVector3(ArrayVector3* a, VoxVector3 element)
{
	if (a->used == a->size)
	{
		a->size *= 2;
		a->array = VOX_REALLOC(a->array, a->size * sizeof(VoxVector3));
	}
	a->array[a->used++] = element;
}

void freeArrayVector3(ArrayVector3* a)
{
	VOX_FREE(a->array);
	a->array = NULL;
	a->used = a->size = 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// ArrayColor helper
/////////////////////////////////////////////////////////////////////////////////////////////

void initArrayColor(ArrayColor* a, int initialSize)
{
	a->array = VOX_MALLOC(initialSize * sizeof(VoxColor));
	a->used = 0;
	a->size = initialSize;
}

void insertArrayColor(ArrayColor* a, VoxColor element)
{
	if (a->used == a->size)
	{
		a->size *= 2;
		a->array = VOX_REALLOC(a->array, a->size * sizeof(VoxColor));
	}
	a->array[a->used++] = element;
}

void freeArrayColor(ArrayColor* a)
{
	VOX_FREE(a->array);
	a->array = NULL;
	a->used = a->size = 0;
}


/////////////////////////////////////////////////////////////////////////////////////////////
// Vox Loader
/////////////////////////////////////////////////////////////////////////////////////////////

#define CHUNKSIZE 16 // chunk size (CHUNKSIZE*CHUNKSIZE*CHUNKSIZE) in voxels 
#define CHUNKSIZE_OPSHIFT 4 // 1<<4=16 -> Warning depend of CHUNKSIZE
#define CHUNK_FLATTENOFFSET_OPSHIFT 8  //Warning depend of CHUNKSIZE

//
// used right handed system and CCW face
// 
// indexes for voxelcoords, per face orientation
//

//#      Y
//#      |
//#      o----X
//#     /
//#    Z     2------------3
//#         /|           /|
//#        6------------7 |
//#        | |          | |
//#        |0 ----------|- 1
//#        |/           |/
//#        4------------5

// 
// CCW
const int fv[6][4] = {
	{0, 2, 6, 4 }, //-X
	{5, 7, 3, 1 }, //+X
	{0, 4, 5, 1 }, //-y
	{6, 2, 3, 7 }, //+y
	{1, 3, 2, 0 }, //-Z
	{4, 6, 7, 5 } };//+Z


const VoxVector3 SolidVertex[] = {
	{0, 0, 0},   //0
	{1, 0, 0},   //1
	{0, 1, 0},   //2
	{1, 1, 0},   //3
	{0, 0, 1},   //4
	{1, 0, 1},   //5
	{0, 1, 1},   //6
	{1, 1, 1} }; //7





// Allocated VoxArray3D size
void Vox_AllocArray(VoxArray3D* pvoxarray, int _sx, int _sy, int _sz)
{
	int sx = _sx + ((CHUNKSIZE - (_sx % CHUNKSIZE)) % CHUNKSIZE);
	int sy = _sy + ((CHUNKSIZE - (_sy % CHUNKSIZE)) % CHUNKSIZE);
	int sz = _sz + ((CHUNKSIZE - (_sz % CHUNKSIZE)) % CHUNKSIZE);

	int chx = sx >> CHUNKSIZE_OPSHIFT; //Chunks Count in X
	int chy = sy >> CHUNKSIZE_OPSHIFT; //Chunks Count in Y
	int chz = sz >> CHUNKSIZE_OPSHIFT; //Chunks Count in Z

	//VoxArray3D* parray = (VoxArray3D*)VOX_MALLOC(sizeof(VoxArray3D));
	pvoxarray->sizeX = sx;
	pvoxarray->sizeY = sy;
	pvoxarray->sizeZ = sz;

	pvoxarray->chunksSizeX = chx;
	pvoxarray->chunksSizeY = chy;
	pvoxarray->chunksSizeZ = chz;

	pvoxarray->ChunkFlattenOffset = (chy * chz); //m_arrayChunks[(x * (sy*sz)) + (z * sy) + y]

	//Alloc chunks array
	int size = sizeof(CubeChunk3D) * chx * chy * chz;
	pvoxarray->m_arrayChunks = VOX_MALLOC(size);
	pvoxarray->arrayChunksSize = size;


	//Init chunks array
	size = chx * chy * chz;
	pvoxarray->chunksTotal = size;
	pvoxarray->chunksAllocated = 0;

	for (int i = 0; i < size; i++)
	{
		pvoxarray->m_arrayChunks[i].m_array = 0;
		pvoxarray->m_arrayChunks[i].arraySize = 0;
	}
}

// Set voxel ID from its position into VoxArray3D
void Vox_SetVoxel(VoxArray3D* pvoxarray, int x, int y, int z, unsigned char id)
{
	//Get chunk from array pos
	int chX = x >> CHUNKSIZE_OPSHIFT; //x / CHUNKSIZE;
	int chY = y >> CHUNKSIZE_OPSHIFT; //y / CHUNKSIZE;
	int chZ = z >> CHUNKSIZE_OPSHIFT; //z / CHUNKSIZE;
	int offset = (chX * pvoxarray->ChunkFlattenOffset) + (chZ * pvoxarray->chunksSizeY) + chY;

	//if (offset > voxarray->arrayChunksSize)
	//{
	//	TraceLog(LOG_ERROR, "Out of array");
	//}

	CubeChunk3D* chunk = &pvoxarray->m_arrayChunks[offset];

	//Set Chunk
	chX = x - (chX << CHUNKSIZE_OPSHIFT); //x - (bx * CHUNKSIZE);
	chY = y - (chY << CHUNKSIZE_OPSHIFT); //y - (by * CHUNKSIZE);
	chZ = z - (chZ << CHUNKSIZE_OPSHIFT); //z - (bz * CHUNKSIZE);

	if (chunk->m_array == 0)
	{
		int size = CHUNKSIZE * CHUNKSIZE * CHUNKSIZE;
		chunk->m_array = VOX_MALLOC(size);
		chunk->arraySize = size;
		memset(chunk->m_array, 0, size);

		pvoxarray->chunksAllocated++;
	}

	offset = (chX << CHUNK_FLATTENOFFSET_OPSHIFT) + (chZ << CHUNKSIZE_OPSHIFT) + chY;

	//if (offset > chunk->arraySize)
	//{
	//	TraceLog(LOG_ERROR, "Out of array");
	//}

	chunk->m_array[offset] = id;

}

// Get voxel ID from its position into VoxArray3D
unsigned char Vox_GetVoxel(VoxArray3D* pvoxarray, int x, int y, int z)
{
	if (x < 0 || y < 0 || z < 0)
		return 0;

	if (x >= pvoxarray->sizeX || y >= pvoxarray->sizeY || z >= pvoxarray->sizeZ)
		return 0;


	//Get chunk from array pos
	int chX = x >> CHUNKSIZE_OPSHIFT; //x / CHUNKSIZE;
	int chY = y >> CHUNKSIZE_OPSHIFT; //y / CHUNKSIZE;
	int chZ = z >> CHUNKSIZE_OPSHIFT; //z / CHUNKSIZE;
	int offset = (chX * pvoxarray->ChunkFlattenOffset) + (chZ * pvoxarray->chunksSizeY) + chY;

	//if (offset > voxarray->arrayChunksSize)
	//{
	//	TraceLog(LOG_ERROR, "Out of array");
	//}

	CubeChunk3D* chunk = &pvoxarray->m_arrayChunks[offset];

	//Set Chunk
	chX = x - (chX << CHUNKSIZE_OPSHIFT); //x - (bx * CHUNKSIZE);
	chY = y - (chY << CHUNKSIZE_OPSHIFT); //y - (by * CHUNKSIZE);
	chZ = z - (chZ << CHUNKSIZE_OPSHIFT); //z - (bz * CHUNKSIZE);

	if (chunk->m_array == 0)
	{
		return 0;
	}

	offset = (chX << CHUNK_FLATTENOFFSET_OPSHIFT) + (chZ << CHUNKSIZE_OPSHIFT) + chY;

	//if (offset > chunk->arraySize)
	//{
	//	TraceLog(LOG_ERROR, "Out of array");
	//}
	return chunk->m_array[offset];

}

// Calc visibles faces from a voxel position
unsigned char Vox_CalcFacesVisible(VoxArray3D* pvoxArray, int cx, int cy, int cz)
{
	unsigned char idXm1 = Vox_GetVoxel(pvoxArray, cx - 1, cy, cz);
	unsigned char idXp1 = Vox_GetVoxel(pvoxArray, cx + 1, cy, cz);

	unsigned char idYm1 = Vox_GetVoxel(pvoxArray, cx, cy - 1, cz);
	unsigned char idYp1 = Vox_GetVoxel(pvoxArray, cx, cy + 1, cz);

	unsigned char idZm1 = Vox_GetVoxel(pvoxArray, cx, cy, cz - 1);
	unsigned char idZp1 = Vox_GetVoxel(pvoxArray, cx, cy, cz + 1);

	unsigned char byVFMask = 0;

	//#-x
	if (idXm1 == 0)
		byVFMask |= (1 << 0);

	//#+x
	if (idXp1 == 0)
		byVFMask |= (1 << 1);

	//#-y
	if (idYm1 == 0)
		byVFMask |= (1 << 2);

	//#+y
	if (idYp1 == 0)
		byVFMask |= (1 << 3);

	//#-z
	if (idZm1 == 0)
		byVFMask |= (1 << 4);

	//#+z
	if (idZp1 == 0)
		byVFMask |= (1 << 5);

	return byVFMask;
}

// Get a vertex position from a voxel's corner
VoxVector3 Vox_GetVertexPosition(int _wcx, int _wcy, int _wcz, int _nNumVertex)
{
	float scale = 0.25;
	VoxVector3 vtx = SolidVertex[_nNumVertex];
	vtx.x = (vtx.x + _wcx) * scale;
	vtx.y = (vtx.y + _wcy) * scale;
	vtx.z = (vtx.z + _wcz) * scale;
	return vtx;
}

// Build a voxel vertices/colors/indices
void Vox_Build_Voxel(VoxArray3D* pvoxArray, int x, int y, int z, int matID)
{

	unsigned char byVFMask = Vox_CalcFacesVisible(pvoxArray, x, y, z);

	if (byVFMask == 0)
		return;

	int i, j;
	VoxVector3 vertComputed[8];
	int bVertexComputed[8];
	memset(vertComputed, 0, sizeof(vertComputed));
	memset(bVertexComputed, 0, sizeof(bVertexComputed));


	//For each Cube's faces
	for (i = 0; i < 6; i++) // 6 faces
	{
		if ((byVFMask & (1 << i)) != 0)	//If face is visible
		{
			for (j = 0; j < 4; j++)   // 4 corners
			{
				int  nNumVertex = fv[i][j];  //Face,Corner
				if (bVertexComputed[nNumVertex] == 0) //if never calc
				{
					bVertexComputed[nNumVertex] = 1;
					vertComputed[nNumVertex] = Vox_GetVertexPosition(x, y, z, nNumVertex);
				}
			}
		}
	}

	//Add face
	for (i = 0; i < 6; i++)// 6 faces
	{
		if ((byVFMask & (1 << i)) == 0)
			continue; //Face invisible

		int v0 = fv[i][0];  //Face, Corner
		int v1 = fv[i][1];  //Face, Corner
		int v2 = fv[i][2];  //Face, Corner
		int v3 = fv[i][3];  //Face, Corner

		//Arrays
		int idx = pvoxArray->vertices.used;
		insertArrayVector3(&pvoxArray->vertices, vertComputed[v0]);
		insertArrayVector3(&pvoxArray->vertices, vertComputed[v1]);
		insertArrayVector3(&pvoxArray->vertices, vertComputed[v2]);
		insertArrayVector3(&pvoxArray->vertices, vertComputed[v3]);

		VoxColor col = pvoxArray->palette[matID];

		insertArrayColor(&pvoxArray->colors, col);
		insertArrayColor(&pvoxArray->colors, col);
		insertArrayColor(&pvoxArray->colors, col);
		insertArrayColor(&pvoxArray->colors, col);


		//v0 - v1 - v2, v0 - v2 - v3
		insertArrayUShort(&pvoxArray->indices, idx + 0);
		insertArrayUShort(&pvoxArray->indices, idx + 2);
		insertArrayUShort(&pvoxArray->indices, idx + 1);

		insertArrayUShort(&pvoxArray->indices, idx + 0);
		insertArrayUShort(&pvoxArray->indices, idx + 3);
		insertArrayUShort(&pvoxArray->indices, idx + 2);



	}

}

// MagicaVoxel *.vox file format Loader
int Vox_LoadFromMemory(const unsigned char* pvoxData, unsigned int voxDataSize, VoxArray3D* pvoxarray)
{

	//////////////////////////////////////////////////
	//Read VOX file
	//4 bytes: magic number ('V' 'O' 'X' 'space' )
	//4 bytes: version number (current version is 150 )

	unsigned long signature;

	unsigned char* fileData = pvoxData;

	unsigned char* fileDataPtr = fileData;
	unsigned char* endfileDataPtr = fileData + voxDataSize;

	signature = *((unsigned long *)fileDataPtr);
	fileDataPtr += sizeof(unsigned long);

	if (signature != 0x20584F56) //56 4F 58 20
	{
		return VOX_ERROR_INVALID_FORMAT; //"Not an MagicaVoxel File format"
	}

	unsigned long version;

	version = *((unsigned long*)fileDataPtr);
	fileDataPtr += sizeof(unsigned long);

	if (version < 150)
	{
		return VOX_ERROR_FILE_VERSION_TOO_OLD; //"MagicaVoxel version too old"
	}


	// header
	//4 bytes: chunk id
	//4 bytes: size of chunk contents (n)
	//4 bytes: total size of children chunks(m)

	//// chunk content
	//n bytes: chunk contents

	//// children chunks : m bytes
	//{ child chunk 0 }
	//{ child chunk 1 }
	unsigned long sizeX, sizeY, sizeZ;
	sizeX = sizeY = sizeZ = 0;
	unsigned long numVoxels = 0;
	int offsetX, offsetY, offsetZ;
	offsetX = offsetY = offsetZ = 0;

	while (fileDataPtr < endfileDataPtr)
	{
		char szChunkName[5];
		memcpy(szChunkName, fileDataPtr, 4);
		szChunkName[4] = 0;
		fileDataPtr += 4;

		unsigned long chunkSize = *((unsigned long*)fileDataPtr);
		fileDataPtr += sizeof(unsigned long);

		unsigned long chunkTotalChildSize = *((unsigned long*)fileDataPtr);
		fileDataPtr += sizeof(unsigned long);

		if (strcmp(szChunkName, "SIZE") == 0)
		{
			//(4 bytes x 3 : x, y, z ) 
			sizeX = *((unsigned long*)fileDataPtr);
			fileDataPtr += sizeof(unsigned long);

			sizeY = *((unsigned long*)fileDataPtr);
			fileDataPtr += sizeof(unsigned long);

			sizeZ = *((unsigned long*)fileDataPtr);
			fileDataPtr += sizeof(unsigned long);

			//Alloc vox array
			Vox_AllocArray(pvoxarray, sizeX, sizeZ, sizeY);	//Reverse Y<>Z for left to right handed system
		}
		else if (strcmp(szChunkName, "XYZI") == 0)
		{
			unsigned char vx, vy, vz, vi;

			//(numVoxels : 4 bytes )
			//(each voxel: 1 byte x 4 : x, y, z, colorIndex ) x numVoxels
			numVoxels = *((unsigned long*)fileDataPtr);
			fileDataPtr += sizeof(unsigned long);

			while (numVoxels > 0)
			{
				vx = *((unsigned char*)fileDataPtr++);
				vy = *((unsigned char*)fileDataPtr++);
				vz = *((unsigned char*)fileDataPtr++);
				vi = *((unsigned char*)fileDataPtr++);

				Vox_SetVoxel(pvoxarray, vx, vz, pvoxarray->sizeZ-vy-1, vi); //Reverse Y<>Z for left to right handed system

				numVoxels--;
			}
		}
		else if (strcmp(szChunkName, "RGBA") == 0)
		{
			VoxColor col;

			//(each pixel: 1 byte x 4 : r, g, b, a ) x 256
			for (int i = 0; i < 256 - 1; i++)
			{
				col.r = *((unsigned char*)fileDataPtr++);
				col.g = *((unsigned char*)fileDataPtr++);
				col.b = *((unsigned char*)fileDataPtr++);
				col.a = *((unsigned char*)fileDataPtr++);

				pvoxarray->palette[i + 1] = col;
			}

		}
		else
		{
			fileDataPtr += chunkSize;
		}
	}


	//TraceLog(LOG_INFO, TextFormat("Vox Size : %dx%dx%d", sizeX, sizeY, sizeZ));

	//TraceLog(LOG_INFO, TextFormat("Vox Chunks Count : %d/%d", pvoxArray->chunksAllocated, pvoxArray->chunksTotal));


	//////////////////////////////////////////////////////////
	// Building Mesh
	//   TODO compute globals indices array

	//TraceLog(LOG_INFO, TextFormat("Building VOX Mesh : %s", pszfileName));

	// Init Arrays
	initArrayVector3(&pvoxarray->vertices, 3 * 1024);
	initArrayUShort(&pvoxarray->indices, 3 * 1024);
	initArrayColor(&pvoxarray->colors, 3 * 1024);

	// Create vertices and indices buffers
	int x, y, z;

	for (x = 0; x <= pvoxarray->sizeX; x++)
	{
		for (z = 0; z <= pvoxarray->sizeZ; z++)
		{
			for (y = 0; y <= pvoxarray->sizeY; y++)
			{
				unsigned char matID = Vox_GetVoxel(pvoxarray, x, y, z);
				if (matID != 0)
					Vox_Build_Voxel(pvoxarray, x, y, z, matID);
			}
		}
	}



	return VOX_SUCCESS;
}

void Vox_FreeArrays(VoxArray3D* voxarray)
{
	//Free chunks
	if (voxarray->m_arrayChunks != 0)
	{
		for (int i = 0; i < voxarray->chunksTotal; i++)
		{
			CubeChunk3D* chunk = &voxarray->m_arrayChunks[i];
			if (chunk->m_array != 0)
			{
				chunk->arraySize = 0;
				VOX_FREE(chunk->m_array);
			}
		}

		VOX_FREE(voxarray->m_arrayChunks);
		voxarray->m_arrayChunks = 0;
		voxarray->arrayChunksSize = 0;

		voxarray->chunksSizeX = voxarray->chunksSizeY = voxarray->chunksSizeZ = 0;
		voxarray->chunksTotal = 0;
		voxarray->chunksAllocated = 0;
		voxarray->ChunkFlattenOffset = 0;
		voxarray->sizeX = voxarray->sizeY = voxarray->sizeZ = 0;
	}

	//Free arrays
	freeArrayVector3(&voxarray->vertices);
	freeArrayUShort(&voxarray->indices);
	freeArrayColor(&voxarray->colors);
}

#endif //VOX_LOADER_IMPLEMENTATION