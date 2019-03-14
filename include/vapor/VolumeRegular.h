#pragma once

#include <vapor/VolumeAlgorithm.h>

namespace VAPoR {

class VolumeRegular : public VolumeAlgorithm {
  public:
    VolumeRegular(GLManager *gl);
    ~VolumeRegular();

    static std::string GetName() { return "Regular"; }
    static Type GetType() { return Type::DVR; }

    virtual int LoadData(const Grid *grid);
    virtual ShaderProgram *GetShader() const;
    virtual void SetUniforms() const;

  protected:
    unsigned int dataTexture;
    unsigned int missingTexture;

    bool hasMissingData;
};

class IsoRegular : public VolumeRegular {
  public:
    IsoRegular(GLManager *gl) : VolumeRegular(gl) {}
    static std::string GetName() { return "Iso Regular"; }
    static Type GetType() { return Type::Iso; }
    virtual ShaderProgram *GetShader() const;
};

} // namespace VAPoR
