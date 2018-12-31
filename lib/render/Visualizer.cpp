//************************************************************************
//									*
//		     Copyright (C)  2004				*
//     University Corporation for Atmospheric Research			*
//		     All Rights Reserved				*
//									*
//************************************************************************/
//
//	File:		Visualizer.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		September, 2013
//
#include <vapor/glutil.h>    // Must be included first!!!
#include <limits>
#include <algorithm>
#include <vector>
#include <cmath>
#include <cassert>
#ifdef WIN32
    #include <tiff/tiffio.h>
#else
    #include <tiffio.h>
    #include <xtiffio.h>
#endif

#ifdef WIN32
    #pragma warning(disable : 4996)
#endif

#include <vapor/RenderParams.h>
#include <vapor/ViewpointParams.h>
#include <vapor/regionparams.h>
#include <vapor/Renderer.h>
#include <vapor/DataStatus.h>
#include <vapor/Visualizer.h>

#include <vapor/common.h>
#include "vapor/GLManager.h"
#include "vapor/LegacyGL.h"

#include "vapor/ImageWriter.h"
#include "vapor/GeoTIFWriter.h"
#include <vapor/DVRenderer.h>

using namespace VAPoR;

Visualizer::Visualizer(const ParamsMgr *pm, const DataStatus *dataStatus, string winName)
{
    MyBase::SetDiagMsg("Visualizer::Visualizer() begin");

    m_paramsMgr = pm;
    m_dataStatus = dataStatus;
    m_winName = winName;
    _glManager = nullptr;
    m_vizFeatures = new AnnotationRenderer(pm, dataStatus, winName);
    _insideGLContext = false;
    _imageCaptureEnabled = false;
    _animationCaptureEnabled = false;

    _renderers.clear();

    MyBase::SetDiagMsg("Visualizer::Visualizer() end");
}

Visualizer::~Visualizer()
{
    for (int i = 0; i < _renderers.size(); i++) delete _renderers[i];
    _renderers.clear();
}

int Visualizer::resizeGL(int wid, int ht) { return 0; }

int Visualizer::getCurrentTimestep() const
{
    vector<string> dataSetNames = m_dataStatus->GetDataMgrNames();

    bool   first = true;
    size_t min_ts = 0;
    size_t max_ts = 0;
    for (int i = 0; i < dataSetNames.size(); i++) {
        vector<RenderParams *> rParams;
        m_paramsMgr->GetRenderParams(m_winName, dataSetNames[i], rParams);

        if (rParams.size()) {
            // Use local time of first RenderParams instance on window
            // for current data set. I.e. it is assumed that every
            // RenderParams instance for a data set has same current
            // time step.
            //
            size_t local_ts = rParams[0]->GetCurrentTimestep();
            size_t my_min_ts, my_max_ts;
            m_dataStatus->MapLocalToGlobalTimeRange(dataSetNames[i], local_ts, my_min_ts, my_max_ts);
            if (first) {
                min_ts = my_min_ts;
                max_ts = my_max_ts;
                first = false;
            } else {
                if (my_min_ts > min_ts) min_ts = my_min_ts;
                if (my_max_ts < max_ts) max_ts = my_max_ts;
            }
        }
    }
    if (min_ts > max_ts) return (-1);

    return (min_ts);
}

void Visualizer::_applyTransformsForRenderer(Renderer *r)
{
    string datasetName = r->GetMyDatasetName();
    string myName = r->GetMyName();
    string myType = r->GetMyType();

    VAPoR::ViewpointParams *vpParams = getActiveViewpointParams();
    vector<double>          scales, rotations, translations, origin;
    Transform *             t = vpParams->GetTransform(datasetName);
    assert(t);
    scales = t->GetScales();
    rotations = t->GetRotations();
    translations = t->GetTranslations();
    origin = t->GetOrigin();

    MatrixManager *mm = _glManager->matrixManager;

    mm->Translate(origin[0], origin[1], origin[2]);
    mm->Scale(scales[0], scales[1], scales[2]);
    mm->Rotate(rotations[0], 1, 0, 0);
    mm->Rotate(rotations[1], 0, 1, 0);
    mm->Rotate(rotations[2], 0, 0, 1);
    mm->Translate(-origin[0], -origin[1], -origin[2]);

    mm->Translate(translations[0], translations[1], translations[2]);
}

int Visualizer::paintEvent(bool fast)
{
    _insideGLContext = true;
    MyBase::SetDiagMsg("Visualizer::paintGL()");
    GL_ERR_BREAK();

    MatrixManager *mm = _glManager->matrixManager;

    // Do not proceed if there is no DataMgr
    if (!m_dataStatus->GetDataMgrNames().size()) return (0);

    _clearFramebuffer();

    // Set up the OpenGL environment
    int timeStep = getCurrentTimestep();
    if (timeStep < 0) {
        MyBase::SetErrMsg("Invalid time step");
        return -1;
    }

    _loadMatricesFromViewpointParams();
    if (placeLights()) return -1;

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    _deleteFlaggedRenderers();
    if (_initializeNewRenderers() < 0) return -1;

    int rc = 0;
    for (int i = 0; i < _renderers.size(); i++) {
        _glManager->matrixManager->MatrixModeModelView();
        _glManager->matrixManager->PushMatrix();

        if (_renderers[i]->IsGLInitialized()) {
            _applyTransformsForRenderer(_renderers[i]);

            int myrc = _renderers[i]->paintGL(fast);
            GL_ERR_BREAK();
            if (myrc < 0) rc = -1;
        }
        mm->MatrixModeModelView();
        mm->PopMatrix();
        int myrc = CheckGLErrorMsg(_renderers[i]->GetMyName().c_str());
        if (myrc < 0) rc = -1;
    }

    if (m_vizFeatures) {
        // Draw the domain frame and other in-scene features
        m_vizFeatures->InScenePaint(timeStep);
        GL_ERR_BREAK();
    }

    _glManager->matrixManager->MatrixModeModelView();

    // Draw any features that are overlaid on scene

    if (m_vizFeatures) m_vizFeatures->DrawText();
    GL_ERR_BREAK();
    _renderColorbars(timeStep);
    GL_ERR_BREAK();

    // _glManager->ShowDepthBuffer();

    glFlush();

    int captureImageSuccess = 0;
    if (_imageCaptureEnabled) {
        captureImageSuccess = _captureImage(_captureImageFile);
    } else if (_animationCaptureEnabled) {
        captureImageSuccess = _captureImage(_captureImageFile);
        incrementPath(_captureImageFile);
    }
    if (captureImageSuccess < 0) {
        SetErrMsg("Failed to save image");
        return -1;
    }

    GL_ERR_BREAK();
    if (CheckGLError()) return -1;

    _insideGLContext = false;
    return rc;
}

void Visualizer::_loadMatricesFromViewpointParams()
{
    ViewpointParams *const vpParams = getActiveViewpointParams();
    MatrixManager *const   mm = _glManager->matrixManager;

    double m[16];
    vpParams->GetProjectionMatrix(m);
    mm->MatrixModeProjection();
    mm->LoadMatrixd(m);

    vpParams->GetModelViewMatrix(m);
    mm->MatrixModeModelView();
    mm->LoadMatrixd(m);
}

int Visualizer::initializeGL(GLManager *glManager)
{
    if (!glManager->IsCurrentOpenGLVersionSupported()) return -1;

    _glManager = glManager;
    m_vizFeatures->InitializeGL(glManager);

    // glewExperimental = GL_TRUE;
    GLenum err = glewInit();
    assert(GLManager::CheckError());
    if (GLEW_OK != err) {
        MyBase::SetErrMsg("Error: Unable to initialize GLEW");
        return -1;
    }

    if (GetVendor() == MESA) { SetErrMsg("GL Vendor String is MESA.\nGraphics drivers may need to be reinstalled"); }

    return 0;
}

// Move to back of rendering list
void Visualizer::moveRendererToFront(Renderer *ren)
{
    auto it = std::find(_renderers.begin(), _renderers.end(), ren);
    assert(it != _renderers.end());
    _renderers.erase(it);
    _renderers.push_back(ren);
}

void Visualizer::moveVolumeRenderersToFront()
{
    Renderer *firstRendererMoved = nullptr;
    auto      rendererPointersCopy = _renderers;
    for (auto it = rendererPointersCopy.rbegin(); it != rendererPointersCopy.rend(); ++it) {
        if (*it == firstRendererMoved) break;
        if ((*it)->GetMyType() == DVRenderer::GetClassType()) {
            moveRendererToFront(*it);
            if (firstRendererMoved == nullptr) firstRendererMoved = *it;
        }
    }
}

void Visualizer::InsertRenderer(Renderer *ren) { _renderers.push_back(ren); }

Renderer *Visualizer::GetRenderer(string type, string instance) const
{
    for (int i = 0; i < _renderers.size(); i++) {
        Renderer *ren = _renderers[i];
        if (ren->GetMyType() == type && ren->GetMyName() == instance) { return (ren); }
    }
    return (NULL);
}

int Visualizer::placeLights()
{
    const ViewpointParams *vpParams = getActiveViewpointParams();
    size_t                 nLights = vpParams->getNumLights();
    assert(nLights <= 1);
    LegacyGL *lgl = _glManager->legacy;

    float lightDir[4];
    for (int i = 0; i < 4; i++) { lightDir[i] = vpParams->getLightDirection(0, i); }

    if (nLights > 0) {
        // TODO GL
        // GL_SHININESS = vpParams->getExponent())
        // vpParams->getSpecularCoeff(0)
        // vpParams->getDiffuseCoeff(0)
        // vpParams->getAmbientCoeff()
        // All the geometry will get a white specular color:

        lgl->LightDirectionfv(lightDir);
    }
    if (CheckGLError()) return -1;
    return 0;
}

Visualizer::GLVendorType Visualizer::GetVendor()
{
    string ven_str((const char *)glGetString(GL_VENDOR));
    string ren_str((const char *)glGetString(GL_RENDERER));

    for (int i = 0; i < ven_str.size(); i++) {
        if (isupper(ven_str[i])) ven_str[i] = tolower(ven_str[i]);
    }
    for (int i = 0; i < ren_str.size(); i++) {
        if (isupper(ren_str[i])) ren_str[i] = tolower(ren_str[i]);
    }

    if ((ven_str.find("mesa") != string::npos) || (ren_str.find("mesa") != string::npos)) {
        return (MESA);
    } else if ((ven_str.find("nvidia") != string::npos) || (ren_str.find("nvidia") != string::npos)) {
        return (NVIDIA);
    } else if ((ven_str.find("ati") != string::npos) || (ren_str.find("ati") != string::npos)) {
        return (ATI);
    } else if ((ven_str.find("intel") != string::npos) || (ren_str.find("intel") != string::npos)) {
        return (INTEL);
    }
    return (UNKNOWN);
}

double Visualizer::getPixelSize() const
{
#ifdef VAPOR3_0_0_ALPHA
    double temp[3];

    // Window height is subtended by viewing angle (45 degrees),
    // at viewer distance (dist from camera to view center)
    const AnnotationParams *vfParams = getActiveAnnotationParams();
    const ViewpointParams * vpParams = getActiveViewpointParams();

    size_t width, height;
    vpParams->GetWindowSize(width, height);

    double center[3], pos[3];
    vpParams->GetRotationCenter(center);
    vpParams->GetCameraPos(pos);

    vsub(center, pos, temp);

    // Apply stretch factor:

    vector<double> stretch = vpParams->GetStretchFactors();
    for (int i = 0; i < 3; i++) temp[i] = stretch[i] * temp[i];
    float distToScene = vlength(temp);
    // tan(45 deg *0.5) is ratio between half-height and dist to scene
    double halfHeight = tan(M_PI * 0.125) * distToScene;
    return (2.f * halfHeight / (double)height);

#endif
    return (0.0);
}

ViewpointParams *Visualizer::getActiveViewpointParams() const { return m_paramsMgr->GetViewpointParams(m_winName); }

RegionParams *Visualizer::getActiveRegionParams() const { return m_paramsMgr->GetRegionParams(m_winName); }

AnnotationParams *Visualizer::getActiveAnnotationParams() const { return m_paramsMgr->GetAnnotationParams(m_winName); }

int Visualizer::_captureImage(const std::string &path)
{
    // Turn off the single capture flag
    _imageCaptureEnabled = false;

    ViewpointParams *vpParams = getActiveViewpointParams();
    size_t           width, height;
    vpParams->GetWindowSize(width, height);

    bool geoTiffOutput = vpParams->GetProjectionType() == ViewpointParams::MapOrthographic && (FileUtils::Extension(path) == "tif" || FileUtils::Extension(path) == "tiff");

    ImageWriter *  writer = nullptr;
    unsigned char *framebuffer = nullptr;
    int            writeReturn = -1;

    framebuffer = new unsigned char[3 * width * height];
    if (!getPixelData(framebuffer))
        ;    // goto captureImageEnd;

    if (geoTiffOutput)
        writer = new GeoTIFWriter(path);
    else
        writer = ImageWriter::CreateImageWriterForFile(path);
    if (writer == nullptr) goto captureImageEnd;

    if (geoTiffOutput) {
        string projString = m_dataStatus->GetDataMgr(m_dataStatus->GetDataMgrNames()[0])->GetMapProjection();

        double m[16];
        vpParams->GetModelViewMatrix(m);
        double posvec[3], upvec[3], dirvec[3];
        vpParams->ReconstructCamera(m, posvec, upvec, dirvec);

        float s = vpParams->GetOrthoProjectionSize();
        float x = posvec[0];
        float y = posvec[1];
        float aspect = width / (float)height;

        GeoTIFWriter *geo = (GeoTIFWriter *)writer;
        geo->SetTiePoint(x, y, width / 2.f, height / 2.f);
        geo->SetPixelScale(s * aspect * 2 / (float)width, s * 2 / (float)height);
        if (geo->ConfigureFromProj4(projString) < 0) goto captureImageEnd;
    }

    writeReturn = writer->Write(framebuffer, width, height);

captureImageEnd:
    if (writer) delete writer;
    if (framebuffer) delete[] framebuffer;

    return writeReturn;
}

bool Visualizer::getPixelData(unsigned char *data) const
{
    ViewpointParams *vpParams = getActiveViewpointParams();

    size_t width, height;
    vpParams->GetWindowSize(width, height);

    // Must clear previous errors first.
    while (glGetError() != GL_NO_ERROR)
        ;

    glReadBuffer(GL_BACK);
    glDisable(GL_SCISSOR_TEST);

    // Calling pack alignment ensures that we can grab the any size window
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, data);
    if (glGetError() != GL_NO_ERROR) {
        SetErrMsg("Error obtaining GL framebuffer data");
        return false;
    }
    // Unfortunately gl reads these in the reverse order that jpeg expects, so
    // Now we need to swap top and bottom!
    unsigned char val;
    for (int j = 0; j < height / 2; j++) {
        for (int i = 0; i < width * 3; i++) {
            val = data[i + width * 3 * j];
            data[i + width * 3 * j] = data[i + width * 3 * (height - j - 1)];
            data[i + width * 3 * (height - j - 1)] = val;
        }
    }

    return true;
}

void Visualizer::_deleteFlaggedRenderers()
{
    assert(_insideGLContext);
    vector<Renderer *> renderersCopy = _renderers;
    for (auto it = renderersCopy.begin(); it != renderersCopy.end(); ++it) {
        if ((*it)->IsFlaggedForDeletion()) {
            _renderers.erase(std::find(_renderers.begin(), _renderers.end(), *it));
            delete *it;
        }
    }
}

int Visualizer::_initializeNewRenderers()
{
    assert(_insideGLContext);
    for (Renderer *r : _renderers) {
        if (!r->IsGLInitialized() && r->initializeGL(_glManager) < 0) {
            MyBase::SetErrMsg("Failed to initialize renderer %s", r->GetInstanceName().c_str());
            return -1;
        }
        GL_ERR_BREAK();
    }
    return 0;
}

void Visualizer::_clearFramebuffer()
{
    assert(_insideGLContext);
    double clr[3];
    getActiveAnnotationParams()->GetBackgroundColor(clr);

    glDepthMask(GL_TRUE);
    glClearColor(clr[0], clr[1], clr[2], 1.f);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
}

void Visualizer::_renderColorbars(int timeStep)
{
    MatrixManager *mm = _glManager->matrixManager;
    mm->MatrixModeModelView();
    mm->PushMatrix();
    mm->LoadIdentity();
    mm->MatrixModeProjection();
    mm->PushMatrix();
    mm->LoadIdentity();
    for (int i = 0; i < _renderers.size(); i++) {
        // If a renderer is not initialized, or if its bypass flag is set, then don't render.
        // Otherwise push and pop the GL matrix stack, and all attribs
        _renderers[i]->renderColorbar();
    }
    mm->MatrixModeProjection();
    mm->PopMatrix();
    mm->MatrixModeModelView();
    mm->PopMatrix();
}

void Visualizer::incrementPath(string &s)
{
    // truncate the last 4 characters (remove .tif or .jpg)
    string s1 = s.substr(0, s.length() - 4);
    string s_end = s.substr(s.length() - 4);
    if (s_end == "jpeg") {
        s1 = s.substr(0, s.length() - 5);
        s_end = s.substr(s.length() - 5);
    }
    // Find digits (before .tif or .jpg)
    size_t lastpos = s1.find_last_not_of("0123456789");
    assert(lastpos < s1.length());
    string s2 = s1.substr(lastpos + 1);
    int    val = stol(s2);
    // Convert val+1 to a string, with leading zeroes, of same length as s2.
    // Except, if val+1 has more digits than s2, increase it.
    int numDigits = 1 + (int)log10((float)(val + 1));
    int len = s2.length();
    if (len < numDigits) len = numDigits;
    if (len > 9) len = 9;
    char format[5] = {"%04d"};
    char result[100];
    char c = '0' + len;
    format[2] = c;
    sprintf(result, format, val + 1);
    string sval = string(result);
    string newbody = s.substr(0, lastpos + 1);
    s = newbody + sval + s_end;
}
