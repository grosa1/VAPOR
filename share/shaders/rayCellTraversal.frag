#version 330 core

#include RayMath.frag

#define EPSILON 1.19e-07

uniform mat4 MVP;
// uniform vec2 resolution;
uniform vec3 cameraPos;
uniform vec3 dataBoundsMin;
uniform vec3 dataBoundsMax;
uniform float LUTMin;
uniform float LUTMax;

uniform ivec3 coordDims;
uniform float unitDistance;
uniform int BBLevels;

vec3 coordDimsF = vec3(coordDims);
ivec3 cellDims = coordDims - 1;

uniform sampler3D data;
uniform sampler1D LUT;
uniform sampler3D coords;
uniform sampler2DArray boxMins;
uniform sampler2DArray boxMaxs;
uniform isampler2D levelDims;

in vec2 ST;

out vec4 fragColor;

#define FI_LEFT  0
#define FI_RIGHT 1
#define FI_UP    2
#define FI_DOWN  3
#define FI_FRONT 4
#define FI_BACK  5
#define FI_NONE 99

#define F_LEFT  ivec3(-1, 0, 0)
#define F_RIGHT ivec3( 1, 0, 0)
#define F_UP    ivec3( 0, 0, 1)
#define F_DOWN  ivec3( 0, 0,-1)
#define F_FRONT ivec3( 0,-1, 0)
#define F_BACK  ivec3( 0, 1, 0)
#define F_NONE  ivec3(-1,-1,-1)

// face   fast  slow
// DOWN    0     1
// UP      0     1
// LEFT    1     2
// RIGHT   1     2
// FRONT   0     2
// BACK    0     2

int GetFastDimForFaceIndex(int i)
{
    if (i == FI_LEFT || i == FI_RIGHT)
    return 1;
    return 0;
}

int GetSlowDimForFaceIndex(int i)
{
    if (i == FI_DOWN || i == FI_UP)
    return 1;
    return 2;
}

ivec3 GetFaceFromFaceIndex(int i)
{
    if (i == 0) return F_LEFT;
    if (i == 1) return F_RIGHT;
    if (i == 2) return F_UP;
    if (i == 3) return F_DOWN;
    if (i == 4) return F_FRONT;
    if (i == 5) return F_BACK;
}

int GetFaceIndexFromFace(ivec3 face)
{
    if (face == F_LEFT)  return FI_LEFT;
    if (face == F_RIGHT) return FI_RIGHT;
    if (face == F_UP)    return FI_UP;
    if (face == F_DOWN)  return FI_DOWN;
    if (face == F_FRONT) return FI_FRONT;
    if (face == F_BACK)  return FI_BACK;
}

vec4 DEBUG_GetFaceColor(ivec3 face)
{
    if (face == F_LEFT)  return vec4(0,0,1,1); // Blue
    if (face == F_RIGHT) return vec4(0,1,0,1); // Green
    if (face == F_UP)    return vec4(0,1,1,1); // Cyan
    if (face == F_DOWN)  return vec4(1,0,0,1); // Red
    if (face == F_FRONT) return vec4(1,0,1,1); // Purple
    if (face == F_BACK)  return vec4(1,1,0,1); // Yellow
}

void GetFaceCoordinateIndices(ivec3 cell, ivec3 face, out ivec3 i0, out ivec3 i1, out ivec3 i2, out ivec3 i3)
{
    // CCW
    if (face == F_DOWN) {
        i0 = cell + ivec3(0, 0, 0);
        i1 = cell + ivec3(0, 1, 0);
        i2 = cell + ivec3(1, 1, 0);
        i3 = cell + ivec3(1, 0, 0);
    }
    else if (face == F_UP) {
        i0 = cell + ivec3(0, 0, 1);
        i1 = cell + ivec3(1, 0, 1);
        i2 = cell + ivec3(1, 1, 1);
        i3 = cell + ivec3(0, 1, 1);
    }
    else if (face == F_LEFT) {
        i0 = cell + ivec3(0, 0, 0);
        i1 = cell + ivec3(0, 0, 1);
        i2 = cell + ivec3(0, 1, 1);
        i3 = cell + ivec3(0, 1, 0);
    }
    else if (face == F_RIGHT) {
        i0 = cell + ivec3(1, 0, 0);
        i1 = cell + ivec3(1, 1, 0);
        i2 = cell + ivec3(1, 1, 1);
        i3 = cell + ivec3(1, 0, 1);
    }
    else if (face == F_FRONT) {
        i0 = cell + ivec3(0, 0, 0);
        i1 = cell + ivec3(1, 0, 0);
        i2 = cell + ivec3(1, 0, 1);
        i3 = cell + ivec3(0, 0, 1);
    }
    else if (face == F_BACK) {
        i0 = cell + ivec3(0, 1, 0);
        i1 = cell + ivec3(0, 1, 1);
        i2 = cell + ivec3(1, 1, 1);
        i3 = cell + ivec3(1, 1, 0);
    }
}

void GetFaceCoordsAndVertices(ivec3 cellIndex, ivec3 face, out ivec3 i0, out ivec3 i1, out ivec3 i2, out ivec3 i3, out vec3 v0, out vec3 v1, out vec3 v2, out vec3 v3)
{
    GetFaceCoordinateIndices(cellIndex, face, i0, i1, i2, i3);
    v0 = texelFetch(coords, i0, 0).xyz;
    v1 = texelFetch(coords, i1, 0).xyz;
    v2 = texelFetch(coords, i2, 0).xyz;
    v3 = texelFetch(coords, i3, 0).xyz;
}

void GetFaceVertices(ivec3 cellIndex, ivec3 face, out vec3 v0, out vec3 v1, out vec3 v2, out vec3 v3)
{
    ivec3 i0, i1, i2, i3;
    GetFaceCoordsAndVertices(cellIndex, face, i0, i1, i2, i3, v0, v1, v2, v3);
}

float GetDataCoordinateSpace(vec3 coordinates)
{
    return texture(data, coordinates/coordDimsF).r;
}

float GetDataForCoordIndex(ivec3 coordIndex)
{
    vec3 coord = vec3(coordIndex)+vec3(0.5);
    return texture(data, (coord)/(coordDims-1)).r;
}

float NormalizeData(float data)
{
    return (data - LUTMin) / (LUTMax - LUTMin);
}

vec4 GetColorForNormalizedData(float normalizedData)
{
    return texture(LUT, normalizedData);
}

vec4 GetAverageColorForCoordIndex(ivec3 coordIndex)
{
    return GetColorForNormalizedData(NormalizeData(GetDataForCoordIndex(coordIndex)));
}

vec4 GetColorAtCoord(vec3 coord)
{
    return GetColorForNormalizedData(NormalizeData(GetDataCoordinateSpace(coord)));
}

bool IntersectRayCellFace(vec3 o, vec3 d, float rt0, ivec3 cellIndex, ivec3 face, out float t, out vec3 dataCoordinate)
{
    ivec3 i0, i1, i2, i3;
    vec3 v0, v1, v2, v3;
    GetFaceCoordsAndVertices(cellIndex, face, i0, i1, i2, i3, v0, v1, v2, v3);
    
    vec4 weights;
    if (IntersectRayQuad(o, d, rt0, v0, v1, v2, v3, t, weights)) {
        dataCoordinate = (weights.x*i0 + weights.y*i1 + weights.z*i2 + weights.w*i3 + vec3(0.5));
        return true;
    }
    return false;
}

vec3 GetTriangleNormal(vec3 v0, vec3 v1, vec3 v2)
{
    return cross(v1-v0, v2-v0);
}

vec3 GetCellFaceNormal(ivec3 cellIndex, ivec3 face)
{
    vec3 v0, v1, v2, v3;
    GetFaceVertices(cellIndex, face, v0, v1, v2, v3);
    
    return (GetTriangleNormal(v0, v1, v2) + GetTriangleNormal(v0, v2, v3)) / 2.0f;
}

bool FindCellExit(vec3 origin, vec3 dir, float t0, ivec3 currentCell, ivec3 entranceFace, out ivec3 exitFace, out vec3 exitCoord, out float t1)
{
    for (int i = 0; i < 6; i++) {
        ivec3 testFace = GetFaceFromFaceIndex(i);
        
        if (testFace == entranceFace)
            continue;
            
        if (IntersectRayCellFace(origin, dir, t0, currentCell, testFace, t1, exitCoord)) {
            if (t1 - t0 > EPSILON) {
                exitFace = testFace;
                return true;
            }
        }
    }
    return false;
}

bool IsCellInBounds(ivec3 cellIndex)
{
    return !(any(lessThan(cellIndex, ivec3(0))) || any(greaterThanEqual(cellIndex, cellDims)));
}

bool SearchNeighboringCells(vec3 origin, vec3 dir, float t0, ivec3 currentCell, out ivec3 nextCell, out ivec3 exitFace, out vec3 exitCoord, out float t1)
{
    for (int sideID = 0; sideID < 6; sideID++) {
        ivec3 side = GetFaceFromFaceIndex(sideID);
        ivec3 testCell = currentCell + side;
        if (IsCellInBounds(testCell)) {
            if (FindCellExit(origin, dir, t0, testCell, F_NONE, exitFace, exitCoord, t1)) {
                nextCell = testCell + exitFace;
                return true;
            }
        }
    }
    return false;
}

bool FindNextCell(vec3 origin, vec3 dir, float t0, ivec3 currentCell, ivec3 entranceFace, out ivec3 nextCell, out ivec3 exitFace, out vec3 exitCoord, out float t1)
{
    if (FindCellExit(origin, dir, t0, currentCell, entranceFace, exitFace, exitCoord, t1)) {
        nextCell = currentCell + exitFace;
        if (!IsCellInBounds(nextCell))
            return false;
        return true;
    } else {
        return SearchNeighboringCells(origin, dir, t0, currentCell, nextCell, exitFace, exitCoord, t1);
    }
}

void BlendToBack(inout vec4 accum, vec4 color)
{
    accum.rgb += color.rgb * color.a * (1-accum.a);
    accum.a += color.a * (1-accum.a);
}

// GL_ONE_MINUS_DST_ALPHA, GL_ONE
void BlendToBack2(inout vec4 accum, vec4 color)
{
    accum = color * (1-accum.a) + accum * (1);
}

// In the above descrete blending equation we have (c = color.a):
//
// a_n+1 = a_n + c_n * (1-a_n)
//
// This can be rearranged to:
//
// a_n+1 = a_n(1-c_n)+c_n
//
// Integrating we get:
//
//              f n
//             -|  c_n
// a_n = 1 - e^ j 0
//
// And the for a constant c, the above integral evaluates to the linear function below
//
float IntegrateConstantAlpha(float a, float distance)
{
    return 1 - exp(-a * distance);
}

vec4 IntegrateAbsorption(vec4 a, vec4 b, float distance)
{
    vec4 delta = b - a;
    return 1 - exp(-(delta * distance*distance + a*distance));
}

vec4 Traverse(vec3 origin, vec3 dir, float t0, ivec3 currentCell, ivec3 entranceFace, out float t1)
{
    vec3 entranceCoord;
    ivec3 nextCell;
    ivec3 exitFace;
    vec3 exitCoord;
    bool hasNext = true;
    float tStart = t0;
    ivec3 initialCell = currentCell;
    
    int i = 0;
    vec4 accum = vec4(0);
    float a = 0;
    
    float null;
    IntersectRayCellFace(origin, dir, -FLT_MAX, currentCell, entranceFace, null, entranceCoord);
    
    while (hasNext) {
        hasNext = FindNextCell(origin, dir, t0, currentCell, entranceFace, nextCell, exitFace, exitCoord, t1);
        
        if (t0 >= 0) {
            vec4 colorA = GetColorAtCoord(entranceCoord);
            vec4 colorB = GetColorAtCoord(exitCoord);
            // vec4 color = IntegrateAbsorption(colorA, colorB, (t1-t0)/unitDistance);
            
            vec4 color = GetAverageColorForCoordIndex(currentCell);
            color.a = IntegrateConstantAlpha(color.a, (t1-t0)/unitDistance);
            BlendToBack(accum, color);
        }
        
        currentCell = nextCell;
        entranceFace = -exitFace;
        entranceCoord = exitCoord;
        t0 = t1;
        i++;
        
        // if (i > 1) break;
        if (accum.a > 0.995)
            break;
    }
    return accum;
}

bool IsRayEnteringCell(vec3 d, ivec3 cellIndex, ivec3 face)
{
    vec3 n = GetCellFaceNormal(cellIndex, face);
    return dot(d, n) < 0;
}

void GetSideCellBBox(ivec3 cellIndex, int sideID, int fastDim, int slowDim, out vec3 bmin, out vec3 bmax)
{
    ivec3 index = ivec3(cellIndex[fastDim], cellIndex[slowDim], sideID);
    bmin = texelFetch(boxMins, index, 0).rgb;
    bmax = texelFetch(boxMaxs, index, 0).rgb;
}

bool IntersectRaySideCellBBox(vec3 origin, vec3 dir, float rt0, ivec3 cellIndex, int sideID, int fastDim, int slowDim)
{
    vec3 bmin, bmax;
    float t0, t1;
    GetSideCellBBox(cellIndex, sideID, fastDim, slowDim, bmin, bmax);
    if (IntersectRayBoundingBox(origin, dir, rt0, bmin, bmax, t0, t1)) {
        return true;
    }
    return false;
}

void GetSideCellBBoxDirect(int x, int y, int sideID, int level, out vec3 bmin, out vec3 bmax)
{
    ivec3 index = ivec3(x, y, sideID);
    bmin = texelFetch(boxMins, index, level).rgb;
    bmax = texelFetch(boxMaxs, index, level).rgb;
}

bool IntersectRaySideCellBBoxDirect(vec3 origin, vec3 dir, float rt0, int x, int y, int sideID, int level)
{
    vec3 bmin, bmax;
    float t0, t1;
    GetSideCellBBoxDirect(x, y, sideID, level, bmin, bmax);
    if (IntersectRayBoundingBox(origin, dir, rt0, bmin, bmax, t0, t1)) {
        return true;
    }
    return false;
}

ivec2 GetBBoxArrayDimensions(int sideID, int level)
{
    return texelFetch(levelDims, ivec2(sideID, level), 0).rg;
}

bool IsFaceThatPassedBBAnInitialCell(vec3 origin, vec3 dir, float t0, ivec3 index, ivec3 side, out ivec3 cellIndex, out ivec3 entranceFace, out float t1)
{
    float tFace;
    vec3 null;
    if (IntersectRayCellFace(origin, dir, t0, index, side, tFace, null)) {
        if (IsRayEnteringCell(dir, index, side)) {
            cellIndex = index;
            entranceFace = side;
            t1 = tFace;
            return true;
        }
    }
    return false;
}

#include BBTraversalAlgorithms.frag

bool SearchSideForInitialCellBasic(vec3 origin, vec3 dir, float t0, int sideID, int fastDim, int slowDim, out ivec3 cellIndex, out ivec3 entranceFace, out float t1)
{
    ivec3 side = GetFaceFromFaceIndex(sideID);
    ivec3 index = (side+1)/2 * (cellDims-1);
    
    for (index[slowDim] = 0; index[slowDim] < cellDims[slowDim]; index[slowDim]++) {
        for (index[fastDim] = 0; index[fastDim] < cellDims[fastDim]; index[fastDim]++) {
            
            if (IntersectRaySideCellBBox(origin, dir, t0, index, sideID, fastDim, slowDim)) {
                if (IsFaceThatPassedBBAnInitialCell(origin, dir, t0, index, side, cellIndex, entranceFace, t1))
                    return true;
            }
        }
    }
    return false;
}

#define SearchSideForInitialCellWithOctree_NLevels(N, origin, dir, t0, sideID, fastDim, slowDim, cellIndex, entranceFace, t1) SearchSideForInitialCellWithOctree_ ## N ## Levels(origin, dir, t0, sideID, fastDim, slowDim, cellIndex, entranceFace, t1)

int SearchSideForInitialCell(vec3 origin, vec3 dir, float t0, int sideID, out ivec3 cellIndex, out ivec3 entranceFace, out float t1)
{
    int fastDim = GetFastDimForFaceIndex(sideID);
    int slowDim = GetSlowDimForFaceIndex(sideID);
    return SearchSideForInitialCellWithOctree_NLevels(BB_LEVELS, origin, dir, t0, sideID, fastDim, slowDim, cellIndex, entranceFace, t1);
    SearchSideForInitialCellBasic(origin, dir, t0, sideID, fastDim, slowDim, cellIndex, entranceFace, t1);
}

int FindInitialCell(vec3 origin, vec3 dir, float t0, out ivec3 cellIndex, out ivec3 entranceFace, out float t1)
{
    int intersections = 0;
    for (int side = 0; side < 6; side++)
        intersections += SearchSideForInitialCell(origin, dir, t0, side, cellIndex, entranceFace, t1);
    return intersections;
}


void main(void)
{
    vec2 screen = ST*2-1;
    vec4 world = inverse(MVP) * vec4(screen, 1, 1);
    world /= world.w;
    vec3 dir = normalize(world.xyz - cameraPos);
    
    float t0, t1, tp;
    
    bool intersectBox = IntersectRayBoundingBox(cameraPos, dir, 0, dataBoundsMin, dataBoundsMax, t0, t1);
    
    if (intersectBox) {
        ivec3 initialCell;
        ivec3 entranceFace;
        float t0 = -FLT_MAX;
        float t1;
        vec4 accum = vec4(0);
        int intersections;
        do {
            intersections = FindInitialCell(cameraPos, dir, t0, initialCell, entranceFace, t1);
            
            
            vec3 dataCoord;
            if (IntersectRayCellFace(cameraPos, dir, 0, initialCell, entranceFace, t1, dataCoord)) {
                // vec4 color = GetColorForNormalizedData(NormalizeData(texture(data, dataCoord/coordDimsF).r));
                vec4 color = GetColorAtCoord(dataCoord);
                
                fragColor = vec4(color);
            } else {
                fragColor = vec4(0);
            }
             // return;
            
            if (intersections > 0) {
                vec4 color = Traverse(cameraPos, dir, t1, initialCell, entranceFace, t1);
                BlendToBack2(accum, color);
            }
                
            t0 = t1;
            
        } while (intersections > 1);
        
        fragColor = accum;
        return;
    }
    discard;
}
