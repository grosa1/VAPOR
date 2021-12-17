#ifndef SLICERENDERER_H
#define SLICERENDERER_H

#include <vapor/glutil.h>

#ifdef Darwin
    #include <OpenGL/gl.h>
#else
    #include <GL/gl.h>
#endif

#include <glm/glm.hpp>
#include <vapor/DataMgr.h>
#include <vapor/utils.h>
#include <vapor/Renderer.h>

namespace VAPoR {

class Slicer;

class RENDER_API SliceRenderer : public Renderer {
public:
    SliceRenderer(const ParamsMgr *pm, string winName, string dataSetName, string instName, DataMgr *dataMgr);

    static string GetClassType() { return ("Slice"); }

    virtual ~SliceRenderer();

protected:
    virtual int _initializeGL();
    virtual int _paintGL(bool fast);

private:
    struct {
        string              varName;
        string              heightVarName;
        size_t              ts;
        int                 refinementLevel;
        int                 compressionLevel;
        int                 textureSampleRate;
        int                 orientation;
        double              xRotation;
        double              yRotation;
        double              zRotation;
        double              xOrigin;
        double              yOrigin;
        double              zOrigin;
        std::vector<float>  tf_lut;
        std::vector<double> tf_minMax;
        std::vector<double> boxMin, boxMax;
        std::vector<double> domainMin, domainMax;
        std::vector<double> sampleLocation;
    } _cacheParams;

    // The SliceRenderer calculates a series of vertices in 3D space that define
    // the corners of a rectangle that we sample along.  These vertices need to
    // be projected into 2D space for sampling.  This struct defines a vertex in
    // 3D space, as well as its projection in 2D space.
    struct _vertexIn2dAnd3d {
        glm::vec3 threeD;
        glm::vec2 twoD;
    };

    Slicer* _slicer;


    void _initVAO();
    void _initTexCoordVBO();
    void _initVertexVBO();

    bool      _isColormapCacheDirty() const;
    bool      _isDataCacheDirty() const;
    bool      _isBoxCacheDirty() const;
    void      _getModifiedExtents(vector<double> &min, vector<double> &max) const;
    int       _saveCacheParams();
    void      _resetColormapCache();
    void      _resetCache();
    void      _initTextures();
    void      _createDataTexture(float *dataValues);
    int       _regenerateSlice();
    int       _getGrid3D(Grid*& grid) const;
    void      _drawDebugPolygons();

    double _newWaySeconds;
    double _newWayInlineSeconds;
    double _oldWaySeconds;

    int _getConstantAxis() const;

    void _configureShader();
    void _resetState();
    void _initializeState();

    bool _initialized;
    int  _textureSideSize;

    GLuint _colorMapTextureID;
    GLuint _dataValueTextureID;

    std::vector<double> _windingOrder;
    std::vector<double>  _rectangle3D;
    //std::vector<glm::tvec3<double, glm::highp>>& _rectangle3D;
    //std::vector<glm::tvec3<double, glm::highp>>& _windingOrder;

    GLuint _VAO;
    GLuint _vertexVBO;
    GLuint _texCoordVBO;

    int _colorMapSize;

    void _clearCache() { _cacheParams.varName.clear(); }
};

};    // namespace VAPoR

#endif
