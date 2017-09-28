//************************************************************************

//		     Copyright (C)  2008										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		TwoDDataRenderer.cpp
//
//	Author:		John Clyne
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		March 2016
//
//	Description:	Implementation of the twoDImageRenderer class
//

#include <vapor/glutil.h>    // Must be included first!!!

#include <iostream>
#include <fstream>
#include <numeric>

#include <vapor/Proj4API.h>
#include <vapor/CFuncs.h>
#include <vapor/utils.h>
#include <vapor/ShaderMgr.h>
#include <vapor/DataMgrUtils.h>
#include <vapor/TwoDDataRenderer.h>
#include <vapor/TwoDDataParams.h>

using namespace VAPoR;

//
// Register class with object factory!!!
//
static RendererRegistrar<TwoDDataRenderer> registrar(TwoDDataRenderer::GetClassType(), TwoDDataParams::GetClassType());

namespace {

// Set to true to force structured grids to be renderered as
// unstructured grids.
//
const bool ForceUnstructured = false;

// GLSL shader constants
//
const string EffectBaseName = "2DData";
const string EffectName = "2DData";
const string EffectNameAttr = "2DDataAttr";
const string VertexDataAttr = "vertexDataAttr";

// Rendering primitives will be aligned with grid points
//
const bool GridAligned = true;

// Texture units. Only use data texture if GridAligned is false
//
const int dataTexUnit = 0;        // GL_TEXTURE0
const int colormapTexUnit = 1;    // GL_TEXTURE1

// Return name of GLSL shader instance to use
//
string getEffectInstance(bool useVertAttr)
{
    if (useVertAttr)
        return (EffectNameAttr);
    else
        return (EffectName);
}

// Compute surface normal (gradient) for point (x,y) using 1st order
// central differences. 'hgtGrid' is a displacement map for Z coordinate.
//
void computeNormal(const Grid *hgtGrid, float x, float y, float dx, float dy, float mv, float &nx, float &ny, float &nz)
{
    nx = ny = 0.0;
    nz = 1.0;
    if (!hgtGrid) return;

    // Missing value?
    //
    if ((hgtGrid->GetValue(x, y)) == mv) return;

    float z_xpdx = hgtGrid->GetValue(x + dx, y);
    if (z_xpdx == mv) { z_xpdx = x; }

    float z_xmdx = hgtGrid->GetValue(x - dx, y);
    if (z_xmdx == mv) { z_xmdx = x; }

    float z_ypdy = hgtGrid->GetValue(x, y + dy);
    if (z_ypdy == mv) { z_ypdy = y; }

    float z_ymdy = hgtGrid->GetValue(x, y - dy);
    if (z_ymdy == mv) { z_ymdy = x; }

    float dzx = z_xpdx - z_xmdx;
    float dzy = z_ypdy - z_ymdy;

    nx = dy * dzx;
    ny = dx * dzy;
    nz = 1.0;
}
}    // namespace

TwoDDataRenderer::TwoDDataRenderer(const ParamsMgr *pm, string winName, string dataSetName, string instName, DataMgr *dataMgr)
: TwoDRenderer(pm, winName, dataSetName, TwoDDataParams::GetClassType(), TwoDDataRenderer::GetClassType(), instName, dataMgr)
{
    _texWidth = 0;
    _texHeight = 0;
    _texelSize = 8;
    _currentTimestep = 0;
    _currentRefLevel = -1;
    _currentLod = -1;
    _currentVarname.clear();
    _currentBoxMinExts.clear();
    _currentBoxMaxExts.clear();
    _currentRefLevelTex = -1;
    _currentLodTex = -1;
    _currentTimestepTex = 0;
    _currentHgtVar.clear();
    _currentBoxMinExtsTex.clear();
    _currentBoxMaxExtsTex.clear();
    _vertsWidth = 0;
    _vertsHeight = 0;
    _nindices = 0;
    _colormap = NULL;
    _colormapsize = 0;
    _vertexDataAttr = -1;

    _cMapTexID = 0;

    TwoDDataParams *  rp = (TwoDDataParams *)GetActiveParams();
    TransferFunction *tf = rp->MakeTransferFunc(rp->GetVariableName());

    _colormapsize = tf->getNumEntries();
    _colormap = new GLfloat[_colormapsize * 4];

    for (int i = 0; i < _colormapsize; i++) {
        _colormap[i * 4 + 0] = (float)i / (float)(_colormapsize - 1);
        _colormap[i * 4 + 1] = (float)i / (float)(_colormapsize - 1);
        _colormap[i * 4 + 2] = (float)i / (float)(_colormapsize - 1);
        _colormap[i * 4 + 3] = 1.0;
    }
}

TwoDDataRenderer::~TwoDDataRenderer()
{
    if (_cMapTexID) glDeleteTextures(1, &_cMapTexID);
    if (_colormap) delete[] _colormap;
}

int TwoDDataRenderer::_initializeGL()
{
//#define	NOSHADER
#ifndef NOSHADER
    if (!_shaderMgr) {
        SetErrMsg("Programmable shading not available");
        return (-1);
    }

    int rc;

    // First shader is used when 'GridAligned' is false
    //
    if (!_shaderMgr->EffectExists(EffectName)) {
        rc = _shaderMgr->DefineEffect(EffectBaseName, "", EffectName);
        if (rc < 0) return (-1);
    }

    // Second shader is used when 'GridAligned' is true
    //
    if (!_shaderMgr->EffectExists(EffectNameAttr)) {
        rc = _shaderMgr->DefineEffect(EffectBaseName, "USE_VERTEX_ATTR;", EffectNameAttr);
        if (rc < 0) return (-1);
    }

    //	rc = _shaderMgr->EnableEffect(EffectNameAttr);
    //	if (rc<0) return(-1);

    rc = (int)_shaderMgr->AttributeLocation(EffectNameAttr, VertexDataAttr);
    if (rc < 0) return (-1);
    _vertexDataAttr = rc;

    //	_shaderMgr->DisableEffect();

#endif

    glGenTextures(1, &_cMapTexID);

    //
    // Standard colormap
    //
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, _cMapTexID);

    glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, _colormapsize, 0, GL_RGBA, GL_FLOAT, _colormap);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_1D, 0);
    return (TwoDRenderer::_initializeGL());
}

int TwoDDataRenderer::_paintGL()
{
    if (printOpenGLError() != 0) return (-1);

    TwoDDataParams *rp = (TwoDDataParams *)GetActiveParams();

    TransferFunction *tf = rp->MakeTransferFunc(rp->GetVariableName());
    tf->makeLut(_colormap);
    vector<double> crange = tf->getMinMaxMapValue();

    int rc;
#ifndef NOSHADER

    string effect = getEffectInstance(GridAligned);

    rc = _shaderMgr->EnableEffect(effect);
    if (rc < 0) return (-1);

    // 2D Data LIGHT parameters hard coded
    //
    _shaderMgr->UploadEffectData(effect, "lightingEnabled", (int)false);
    _shaderMgr->UploadEffectData(effect, "kd", (float)0.6);
    _shaderMgr->UploadEffectData(effect, "ka", (float)0.3);
    _shaderMgr->UploadEffectData(effect, "ks", (float)0.1);
    _shaderMgr->UploadEffectData(effect, "expS", (float)16.0);
    _shaderMgr->UploadEffectData(effect, "lightDirection", (float)0.0, (float)0.0, (float)1.0);

    _shaderMgr->UploadEffectData(effect, "minLUTValue", (float)crange[0]);
    _shaderMgr->UploadEffectData(effect, "maxLUTValue", (float)crange[1]);

    _shaderMgr->UploadEffectData(effect, "colormap", colormapTexUnit);

    // If data aren't grid aligned we sample the data values with a
    // texture.
    //
    if (!GridAligned) { _shaderMgr->UploadEffectData(effect, "dataTexture", dataTexUnit); }

#endif

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_1D, _cMapTexID);
    glEnable(GL_TEXTURE_1D);

    // Really only need to reload colormap texture if it changes
    //
    glTexSubImage1D(GL_TEXTURE_1D, 0, 0, _colormapsize, GL_RGBA, GL_FLOAT, _colormap);

    glActiveTexture(GL_TEXTURE0);
    rc = TwoDRenderer::_paintGL();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_1D, 0);
    glDisable(GL_TEXTURE_1D);

#ifndef NOSHADER
    _shaderMgr->DisableEffect();
#endif

    return (rc);
}

const GLvoid *TwoDDataRenderer::GetTexture(DataMgr *dataMgr, GLsizei &width, GLsizei &height, GLint &internalFormat, GLenum &format, GLenum &type, size_t &texelSize, bool &gridAligned)
{
    internalFormat = GL_RG32F;
    format = GL_RG;
    type = GL_FLOAT;
    texelSize = _texelSize;
    gridAligned = GridAligned;

    TwoDDataParams *rParams = (TwoDDataParams *)GetActiveParams();

    GLvoid *texture = (GLvoid *)_getTexture(dataMgr);
    if (!texture) return (NULL);

    width = _texWidth;
    height = _texHeight;
    return (texture);
}

int TwoDDataRenderer::GetMesh(DataMgr *dataMgr, GLfloat **verts, GLfloat **normals, GLsizei &width, GLsizei &height, GLuint **indices, GLsizei &nindices, bool &structuredMesh)
{
    width = 0;
    height = 0;
    nindices = 0;

    // See if already in cache
    //
    if (!_gridStateDirty() && _sb_verts.GetBuf()) {
        width = _vertsWidth;
        height = _vertsHeight;
        *verts = (GLfloat *)_sb_verts.GetBuf();
        *normals = (GLfloat *)_sb_normals.GetBuf();

        nindices = _nindices;
        *indices = (GLuint *)_sb_indices.GetBuf();
        return (0);
    }

    _gridStateClear();

    TwoDDataParams *rParams = (TwoDDataParams *)GetActiveParams();
    int             refLevel = rParams->GetRefinementLevel();
    int             lod = rParams->GetCompressionLevel();

    // Find box extents for ROI
    //
    vector<double> minBoxReq, maxBoxReq;
    size_t         ts = rParams->GetCurrentTimestep();
    rParams->GetBox()->GetExtents(minBoxReq, maxBoxReq);

    string varname = rParams->GetVariableName();
    int    orientation = _getOrientation(dataMgr, varname);
    if (orientation != 2) {
        SetErrMsg("Only XY plane orientations currently supported");
        return (-1);
    }

    Grid *g = NULL;
    int   rc = DataMgrUtils::GetGrids(dataMgr, ts, varname, minBoxReq, maxBoxReq, true, &refLevel, &lod, &g);
    if (rc < 0) return (-1);

    assert(g);

    if (dynamic_cast<StructuredGrid *>(g) && !ForceUnstructured) {
        rc = _getMeshStructured(dataMgr, dynamic_cast<StructuredGrid *>(g), minBoxReq[2]);
        structuredMesh = true;
    } else {
        rc = _getMeshUnStructured(dataMgr, g, minBoxReq[2]);
        structuredMesh = false;
    }

    dataMgr->UnlockGrid(g);
    delete g;

    if (rc < 0) return (-1);

    _gridStateSet();

    *verts = (GLfloat *)_sb_verts.GetBuf();
    *normals = (GLfloat *)_sb_normals.GetBuf();
    *indices = (GLuint *)_sb_indices.GetBuf();

    width = _vertsWidth;
    height = _vertsHeight;
    nindices = _nindices;

    return (0);
}

bool TwoDDataRenderer::_gridStateDirty() const
{
    TwoDDataParams *rParams = (TwoDDataParams *)GetActiveParams();

    int            refLevel = rParams->GetRefinementLevel();
    int            lod = rParams->GetCompressionLevel();
    string         hgtVar = rParams->GetHeightVariableName();
    int            ts = rParams->GetCurrentTimestep();
    vector<double> boxMinExts, boxMaxExts;
    rParams->GetBox()->GetExtents(boxMinExts, boxMaxExts);

    return (refLevel != _currentRefLevel || lod != _currentLod || hgtVar != _currentHgtVar || ts != _currentTimestep || boxMinExts != _currentBoxMinExts || boxMaxExts != _currentBoxMaxExts);
}

void TwoDDataRenderer::_gridStateClear()
{
    _currentRefLevel = 0;
    _currentLod = 0;
    _currentHgtVar.clear();
    _currentTimestep = -1;
    _currentBoxMinExts.clear();
    _currentBoxMaxExts.clear();
}

void TwoDDataRenderer::_gridStateSet()
{
    TwoDDataParams *rParams = (TwoDDataParams *)GetActiveParams();
    _currentRefLevel = rParams->GetRefinementLevel();
    _currentLod = rParams->GetCompressionLevel();
    _currentHgtVar = rParams->GetHeightVariableName();
    _currentTimestep = rParams->GetCurrentTimestep();
    rParams->GetBox()->GetExtents(_currentBoxMinExts, _currentBoxMaxExts);
}

bool TwoDDataRenderer::_texStateDirty(DataMgr *dataMgr) const
{
    TwoDDataParams *rParams = (TwoDDataParams *)GetActiveParams();

    int            refLevel = rParams->GetRefinementLevel();
    int            lod = rParams->GetCompressionLevel();
    int            ts = rParams->GetCurrentTimestep();
    vector<double> boxMinExts, boxMaxExts;
    rParams->GetBox()->GetExtents(boxMinExts, boxMaxExts);
    string varname = rParams->GetVariableName();

    return (_currentRefLevelTex != refLevel || _currentLodTex != lod || _currentTimestepTex != ts || _currentBoxMinExtsTex != boxMinExts || _currentBoxMaxExtsTex != boxMaxExts
            || _currentVarname != varname);
}

void TwoDDataRenderer::_texStateSet(DataMgr *dataMgr)
{
    TwoDDataParams *rParams = (TwoDDataParams *)GetActiveParams();
    string          varname = rParams->GetVariableName();

    _currentRefLevelTex = rParams->GetRefinementLevel();
    _currentLodTex = rParams->GetCompressionLevel();
    _currentTimestepTex = rParams->GetCurrentTimestep();
    rParams->GetBox()->GetExtents(_currentBoxMinExtsTex, _currentBoxMaxExtsTex);
    _currentVarname = varname;
}

void TwoDDataRenderer::_texStateClear()
{
    _currentRefLevelTex = 0;
    _currentLodTex = 0;
    _currentTimestepTex = -1;
    _currentBoxMinExtsTex.clear();
    _currentBoxMaxExtsTex.clear();
    _currentVarname.clear();
}

// Get mesh for a structured grid
//
int TwoDDataRenderer::_getMeshStructured(DataMgr *dataMgr, const StructuredGrid *g, double defaultZ)
{
    TwoDDataParams *rParams = (TwoDDataParams *)GetActiveParams();

    vector<size_t> dims = g->GetDimensions();
    assert(dims.size() == 2);

    _vertsWidth = dims[0];
    _vertsHeight = dims[1];
    _nindices = _vertsWidth * 2;

    // (Re)allocate space for verts
    //
    size_t vertsSize = _vertsWidth * _vertsHeight * 3;
    _sb_verts.Alloc(vertsSize * sizeof(GLfloat));
    _sb_normals.Alloc(vertsSize * sizeof(GLfloat));
    _sb_indices.Alloc(2 * _vertsWidth * sizeof(GLuint));

    int rc;
    if (!rParams->GetHeightVariableName().empty()) {
        rc = _getMeshStructuredDisplaced(dataMgr, g, defaultZ);
    } else {
        rc = _getMeshStructuredPlane(dataMgr, g, defaultZ);
    }
    if (rc < 0) return (rc);

    // Compute vertex normals
    //
    GLfloat *verts = (GLfloat *)_sb_verts.GetBuf();
    GLfloat *normals = (GLfloat *)_sb_normals.GetBuf();
    ComputeNormals(verts, _vertsWidth, _vertsHeight, normals);

    // Construct indices for a triangle strip covering one row
    // of the mesh
    //
    GLuint *indices = (GLuint *)_sb_indices.GetBuf();
    for (GLuint i = 0; i < _vertsWidth; i++) indices[2 * i] = i;
    for (GLuint i = 0; i < _vertsWidth; i++) indices[2 * i + 1] = i + _vertsWidth;

    return (0);
}

// Get mesh for an unstructured grid
//
int TwoDDataRenderer::_getMeshUnStructured(DataMgr *dataMgr, const Grid *g, double defaultZ)
{
#ifdef DEAD
    TwoDDataParams *rParams = (TwoDDataParams *)GetActiveParams();

    assert(g->GetTopologyDim() == 2);
    vector<size_t> dims = g->GetDimensions();

    // Unstructured 2d grids are stored in 1d
    //
    _vertsWidth = std::accumulate(dims.begin(), dims.end(), 1, std::multiplies<size_t>());
    _vertsHeight = 1;

    // Count the number of triangle vertex indices needed
    //
    _nindices = 0;
    StructuredGrid::ConstCellIterator citr;
    StructuredGrid::ConstCellIterator endcitr = g->ConstCellEnd();
    for (citr = g->ConstCellBegin(); citr != endcitr; ++citr) {
        std::vector<std::vector<size_t>> nodes;
        g->GetCellNodes(*citr, nodes);

        if (nodes.size() < 3) continue;    // degenerate
        _nindices += 3 * (nodes.size() - 2);
    }

    // (Re)allocate space for verts
    //
    size_t vertsSize = _vertsWidth * 3;
    _sb_verts.Alloc(vertsSize * sizeof(GLfloat));
    _sb_normals.Alloc(vertsSize * sizeof(GLfloat));
    _sb_indices.Alloc(_nindices * sizeof(GLuint));

    return (_getMeshUnStructuredHelper(dataMgr, g, defaultZ));
#endif
}

int TwoDDataRenderer::_getMeshUnStructuredHelper(DataMgr *dataMgr, const Grid *g, double defaultZ)
{
#ifdef DEAD

    TwoDDataParams *rParams = (TwoDDataParams *)GetActiveParams();
    // Construct the displaced (terrain following) grid using
    // a map projection, if specified.
    //
    size_t ts = rParams->GetCurrentTimestep();
    int    refLevel = rParams->GetRefinementLevel();
    int    lod = rParams->GetCompressionLevel();

    // Find box extents for ROI
    //
    vector<double> minExts, maxExts;
    g->GetUserExtents(minExts, maxExts);

    // Try to get requested refinement level or the nearest acceptable level:
    //
    string hgtvar = rParams->GetHeightVariableName();

    Grid *hgtGrid = NULL;

    if (!hgtvar.empty()) {
        int rc = DataMgrUtils::GetGrids(dataMgr, ts, hgtvar, minExts, maxExts, true, &refLevel, &lod, &hgtGrid);

        if (rc < 0) return (rc);
        assert(hgtGrid);
    }

    assert(g->GetTopologyDim() == 2);
    vector<size_t> dims = g->GetDimensions();

    GLfloat *verts = (GLfloat *)_sb_verts.GetBuf();
    GLfloat *normals = (GLfloat *)_sb_normals.GetBuf();
    GLuint * indices = (GLuint *)_sb_indices.GetBuf();

    double mv = hgtGrid ? hgtGrid->GetMissingValue() : 0.0;

    // Hard-code dx and dy for gradient calculation :-(
    //
    float dx = (maxExts[0] - minExts[0]) / 1000.0;
    float dy = (maxExts[1] - minExts[1]) / 1000.0;

    //
    // Visit each node in the grid, build a list of vertices
    //
    Grid::ConstNodeIterator nitr;
    Grid::ConstNodeIterator endnitr = g->ConstNodeEnd();
    size_t                  voffset = 0;
    for (nitr = g->ConstNodeBegin(); nitr != endnitr; ++nitr) {
        vector<double> coords;

        g->GetUserCoordinates(*nitr, coords);

        // Lookup vertical coordinate displacement as a data element
        // from the
        // height variable. Note, missing values are possible if image
        // extents are out side of extents for height variable, or if
        // height variable itself contains missing values.
        //
        double deltaZ = hgtGrid ? hgtGrid->GetValue(coords) : 0.0;
        if (deltaZ == mv) deltaZ = 0.0;

        verts[voffset + 0] = coords[0];
        verts[voffset + 1] = coords[1];
        verts[voffset + 2] = deltaZ + defaultZ;

        // Compute the surface normal using central differences
        //
        computeNormal(hgtGrid, coords[0], coords[1], dx, dy, mv, normals[voffset + 0], normals[voffset + 1], normals[voffset + 2]);

        voffset += 3;
    }

    //
    // Visit each cell in the grid. For each cell triangulate it and
    // and compute an index
    // array for the triangle list
    //
    StructuredGrid::ConstCellIterator citr;
    StructuredGrid::ConstCellIterator endcitr = g->ConstCellEnd();
    size_t                            index = 0;
    size_t                            offset = g->GetNodeOffset();
    for (citr = g->ConstCellBegin(); citr != endcitr; ++citr) {
        std::vector<std::vector<size_t>> nodes;
        g->GetCellNodes(*citr, nodes);

        if (nodes.size() < 3) continue;    // degenerate

        // Compute triangle node indices
        //
        for (int i = 0; i < nodes.size() - 2; i++) {
            indices[index++] = LinearizeCoords(nodes[0], dims) - offset;
            indices[index++] = LinearizeCoords(nodes[i + 1], dims) - offset;
            indices[index++] = LinearizeCoords(nodes[i + 2], dims) - offset;
        }
    }

    if (hgtGrid) {
        dataMgr->UnlockGrid(hgtGrid);
        delete hgtGrid;
    }

#endif
    return (0);
}

// Get mesh for a structured grid displaced by a height field
//
int TwoDDataRenderer::_getMeshStructuredDisplaced(DataMgr *dataMgr, const StructuredGrid *g, double defaultZ)
{
    TwoDDataParams *rParams = (TwoDDataParams *)GetActiveParams();
    // Construct the displaced (terrain following) grid using
    // a map projection, if specified.
    //
    size_t ts = rParams->GetCurrentTimestep();
    int    refLevel = rParams->GetRefinementLevel();
    int    lod = rParams->GetCompressionLevel();

    // Find box extents for ROI
    //
    vector<double> minExtsReq, maxExtsReq;
    rParams->GetBox()->GetExtents(minExtsReq, maxExtsReq);

    // Try to get requested refinement level or the nearest acceptable level:
    //
    string hgtvar = rParams->GetHeightVariableName();
    assert(!hgtvar.empty());

    Grid *hgtGrid = NULL;
    int   rc = DataMgrUtils::GetGrids(dataMgr, ts, hgtvar, minExtsReq, maxExtsReq, true, &refLevel, &lod, &hgtGrid);
    if (rc < 0) return (rc);
    assert(hgtGrid);

    vector<size_t> dims = g->GetDimensions();
    assert(dims.size() == 2);

    size_t   width = dims[0];
    size_t   height = dims[1];
    GLfloat *verts = (GLfloat *)_sb_verts.GetBuf();
    double   mv = hgtGrid->GetMissingValue();
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            double x, y, zdummy;
            g->GetUserCoordinates(i, j, x, y, zdummy);

            // Lookup vertical coordinate displacement as a data element from the
            // height variable. Note, missing values are possible if image
            // extents are out side of extents for height variable, or if
            // height variable itself contains missing values.
            //
            double deltaZ = hgtGrid->GetValue(x, y, 0.0);
            if (deltaZ == mv) deltaZ = 0.0;

            double z = deltaZ + defaultZ;

            //
            verts[j * width * 3 + i * 3] = x;
            verts[j * width * 3 + i * 3 + 1] = y;
            verts[j * width * 3 + i * 3 + 2] = z;
        }
    }

    dataMgr->UnlockGrid(hgtGrid);
    delete hgtGrid;

    return (rc);
}

// Get mesh for a structured grid that is NOT displaced by a height field.
// I.e. it's planar.
//
int TwoDDataRenderer::_getMeshStructuredPlane(DataMgr *dataMgr, const StructuredGrid *g, double defaultZ)
{
    vector<size_t> dims = g->GetDimensions();
    assert(dims.size() == 2);

    size_t   width = dims[0];
    size_t   height = dims[1];
    GLfloat *verts = (GLfloat *)_sb_verts.GetBuf();
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            double x, y, zdummy;
            g->GetUserCoordinates(i, j, x, y, zdummy);

            double z = defaultZ;

            verts[j * width * 3 + i * 3] = x;
            verts[j * width * 3 + i * 3 + 1] = y;
            verts[j * width * 3 + i * 3 + 2] = z;
        }
    }

    return (0);
}

int TwoDDataRenderer::_getOrientation(DataMgr *dataMgr, string varname)
{
    vector<string> coordvars;
    bool           ok = dataMgr->GetVarCoordVars(varname, true, coordvars);
    assert(ok);
    assert(coordvars.size() == 2);

    vector<int> axes;    // order list of coordinate axes
    for (int i = 0; i < coordvars.size(); i++) {
        DC::CoordVar cvar;
        dataMgr->GetCoordVarInfo(coordvars[i], cvar);

        axes.push_back(cvar.GetAxis());
    }

    if (axes[0] == 0) {
        if (axes[1] == 1)
            return (2);    // X-Y
        else
            return (1);    // X-Z
    }

    assert(axes[0] == 1 && axes[2] == 2);
    return (0);    // Y-Z
}

// Sets _texWidth, _texHeight, _sb_texture
//
const GLvoid *TwoDDataRenderer::_getTexture(DataMgr *dataMgr)
{
    // See if already in cache
    //
    if (!_texStateDirty(dataMgr) && _sb_texture.GetBuf()) {
        cout << "_getTexture already cached" << endl;
        return ((const GLvoid *)_sb_texture.GetBuf());
    }
    _texStateClear();

    TwoDDataParams *rParams = (TwoDDataParams *)GetActiveParams();
    size_t          ts = rParams->GetCurrentTimestep();

    int refLevel = rParams->GetRefinementLevel();
    int lod = rParams->GetCompressionLevel();

    string varname = rParams->GetVariableName();
    if (varname.empty()) {
        SetErrMsg("No variable name specified");
        return (NULL);
    }

    // Find box extents for ROI
    //
    vector<double> minBoxReq, maxBoxReq;
    rParams->GetBox()->GetExtents(minBoxReq, maxBoxReq);

    Grid *g = NULL;
    int   rc = DataMgrUtils::GetGrids(dataMgr, ts, varname, minBoxReq, maxBoxReq, true, &refLevel, &lod, &g);

    if (g->GetTopologyDim() != 2) {
        SetErrMsg("Invalid variable: %s ", varname.c_str());
        return (NULL);
    }

    if (rc < 0) return (NULL);

    // For structured grid variable data are stored in a 2D array.
    // For structured grid variable data are stored in a 1D array.
    //
    vector<size_t> dims = g->GetDimensions();
    if (dynamic_cast<StructuredGrid *>(g) && !ForceUnstructured) {
        _texWidth = dims[0];
        _texHeight = dims[1];
    } else {
        _texWidth = std::accumulate(dims.begin(), dims.end(), 1, std::multiplies<size_t>());
        _texHeight = 1;
    }

    size_t   texSize = _texWidth * _texHeight;
    GLfloat *texture = (float *)_sb_texture.Alloc(texSize * _texelSize);
    GLfloat *texptr = texture;

    Grid::Iterator itr;
    Grid::Iterator enditr = g->end();
    //	for (itr = g->begin(minBoxReq, maxBoxReq); itr != enditr; ++itr) {
    for (itr = g->begin(); itr != enditr; ++itr) {
        float v = *itr;

        if (v == g->GetMissingValue()) {
            *texptr++ = 0.0;    // Data value
            *texptr++ = 1.0;    // Missing value flag
        } else {
            *texptr++ = v;
            *texptr++ = 0;
        }
    }

    _texStateSet(dataMgr);

    // Unlock the Grid
    //
    dataMgr->UnlockGrid(g);

    return (texture);
}
