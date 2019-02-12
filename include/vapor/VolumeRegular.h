#pragma once

#include <vapor/VolumeAlgorithm.h>

namespace VAPoR {

class VolumeRegular : public VolumeAlgorithm {
public:
    VolumeRegular(GLManager *gl);
    ~VolumeRegular();

    virtual int            LoadData(const Grid *grid);
    virtual ShaderProgram *GetShader() const;
    virtual void           SetUniforms() const;

public:
    unsigned int dataTexture;
};

}    // namespace VAPoR
