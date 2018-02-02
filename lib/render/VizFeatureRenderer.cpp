//-- VizFeatureRenderer.cpp ----------------------------------------------------------
//
//				   Copyright (C)  2015
//	 University Corporation for Atmospheric Research
//				   All Rights Reserved
//
//----------------------------------------------------------------------------
//
//	  File:		   VizFeatureRenderer.cpp
//
//	  Author:		 Alan Norton
//
//	  Description:  Implementation of VizFeatureRenderer class
//
//----------------------------------------------------------------------------

#include <vapor/glutil.h> // Must be included first!!!
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cfloat>
#include <sstream>
#include <iomanip>

#ifndef WIN32
#include <unistd.h>
#endif

#include <vapor/DataStatus.h>
#include <vapor/VizFeatureRenderer.h>
#include <vapor/GetAppPath.h>

using namespace VAPoR;
using namespace Wasp;

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VizFeatureRenderer::VizFeatureRenderer(
    const ParamsMgr *pm, const DataStatus *dataStatus, string winName) {
    m_paramsMgr = pm;
    m_dataStatus = dataStatus;
    m_winName = winName;
    m_shaderMgr = NULL;

    _textObjectsValid = false;
    _textObject = NULL;

    vector<string> fpath;
    fpath.push_back("fonts");
    _fontFile = GetAppPath("VAPOR", "share", fpath);
    _fontFile = _fontFile + "//arimo.ttf";
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
VizFeatureRenderer::~VizFeatureRenderer() {
#ifdef DEAD
    if (_textObjectsValid)
        invalidateCache();
#endif
}

void VizFeatureRenderer::InitializeGL(ShaderMgr *shaderMgr) {
    m_shaderMgr = shaderMgr;
}

//Issue OpenGL commands to draw a grid of lines of the full domain.
//Grid resolution is up to 2x2x2
//
void VizFeatureRenderer::drawDomainFrame(size_t ts) const {

    VizFeatureParams *vfParams = m_paramsMgr->GetVizFeatureParams(m_winName);

    vector<double> minExts, maxExts;
    m_dataStatus->GetActiveExtents(
        m_paramsMgr, m_winName, ts, minExts, maxExts);

#ifdef DEAD
    vector<double> stretchFac = vfParams->GetStretchFactors();

#endif

    int i;
    int numLines[3];
    double fullSize[3], modMin[3], modMax[3];

    //Instead:  either have 2 or 1 lines in each dimension.  2 if the size is < 1/3
    for (i = 0; i < minExts.size(); i++) {
        double regionSize = maxExts[i] - minExts[i];

        //Stretch size by 1%
        fullSize[i] = (maxExts[i] - minExts[i]) * 1.01;
        double mid = 0.5f * (maxExts[i] + minExts[i]);
        modMin[i] = mid - 0.5f * fullSize[i];
        modMax[i] = mid + 0.5f * fullSize[i];
        if (regionSize < fullSize[i] * .3)
            numLines[i] = 2;
        else
            numLines[i] = 1;
    }

    glPushAttrib(GL_CURRENT_BIT);
    double clr[3];
    vfParams->GetDomainColor(clr);
    glColor3dv(clr);
    glLineWidth(2.0);
    //Now draw the lines.  Divide each dimension into numLines[dim] sections.

    int x, y, z;
    //Do the lines in each z-plane
    //Turn on writing to the z-buffer
    glDepthMask(GL_TRUE);

    glBegin(GL_LINES);
    for (z = 0; z <= numLines[2]; z++) {
        float zCrd = modMin[2] + ((float)z / (float)numLines[2]) * fullSize[2];
        //Draw lines in x-direction for each y
        for (y = 0; y <= numLines[1]; y++) {
            float yCrd = modMin[1] + ((float)y / (float)numLines[1]) * fullSize[1];

            glVertex3f(modMin[0], yCrd, zCrd);
            glVertex3f(modMax[0], yCrd, zCrd);
        }
        //Draw lines in y-direction for each x
        for (x = 0; x <= numLines[0]; x++) {
            float xCrd = modMin[0] + ((float)x / (float)numLines[0]) * fullSize[0];

            glVertex3f(xCrd, modMin[1], zCrd);
            glVertex3f(xCrd, modMax[1], zCrd);
        }
    }
    //Do the lines in each y-plane

    for (y = 0; y <= numLines[1]; y++) {
        float yCrd = modMin[1] + ((float)y / (float)numLines[1]) * fullSize[1];
        //Draw lines in x direction for each z
        for (z = 0; z <= numLines[2]; z++) {
            float zCrd = modMin[2] + ((float)z / (float)numLines[2]) * fullSize[2];

            glVertex3f(modMin[0], yCrd, zCrd);
            glVertex3f(modMax[0], yCrd, zCrd);
        }
        //Draw lines in z direction for each x
        for (x = 0; x <= numLines[0]; x++) {
            float xCrd = modMin[0] + ((float)x / (float)numLines[0]) * fullSize[0];

            glVertex3f(xCrd, yCrd, modMin[2]);
            glVertex3f(xCrd, yCrd, modMax[2]);
        }
    }

    //Do the lines in each x-plane
    for (x = 0; x <= numLines[0]; x++) {
        float xCrd = modMin[0] + ((float)x / (float)numLines[0]) * fullSize[0];
        //Draw lines in y direction for each z
        for (z = 0; z <= numLines[2]; z++) {
            float zCrd = modMin[2] + ((float)z / (float)numLines[2]) * fullSize[2];

            glVertex3f(xCrd, modMin[1], zCrd);
            glVertex3f(xCrd, modMax[1], zCrd);
        }
        //Draw lines in z direction for each y
        for (y = 0; y <= numLines[1]; y++) {
            float yCrd = modMin[1] + ((float)y / (float)numLines[1]) * fullSize[1];

            glVertex3f(xCrd, yCrd, modMin[2]);
            glVertex3f(xCrd, yCrd, modMax[2]);
        }
    }

    glEnd(); //GL_LINES
    glPopAttrib();
}

void VizFeatureRenderer::DrawText() {
    DrawText(_miscAnnot);
    DrawText(_timeAnnot);
    DrawText(_axisAnnot);
}

void VizFeatureRenderer::DrawText(vector<billboard> billboards) {
    double txtColor[] = {1.f, 1.f, 1.f, 1.f};
    double bgColor[] = {0.f, 0.f, 0.f, 0.f};
    double coords[] = {67.5f, 31.6f, 0.f};

    for (int i = 0; i < billboards.size(); i++) {
        string text = billboards[i].text;
        coords[0] = billboards[i].x;
        coords[1] = billboards[i].y;
        int size = billboards[i].size;
        txtColor[0] = billboards[i].color[0];
        txtColor[1] = billboards[i].color[1];
        txtColor[2] = billboards[i].color[2];

        if (_textObject != NULL) {
            delete _textObject;
            _textObject = NULL;
        }

        _textObject = new TextObject();
        _textObject->Initialize(_fontFile,
                                text, size, txtColor, bgColor);
        _textObject->drawMe(coords);
    }
}

void VizFeatureRenderer::AddText(string text,
                                 int x, int y, int size,
                                 float color[3], int type) {
    //_billboards.clear();  // Temporary hack.  We eventually need separate
    // billboard groups for time annotations, axis
    // labels, etc.  Grouping them all in the same
    // vector makes it hard if not impossible to
    // make changes to any of the labels (color, size,
    // etc)
    billboard myBoard;
    myBoard.text = text;
    myBoard.x = x;
    myBoard.y = y;
    myBoard.size = size;
    myBoard.color[0] = color[0];
    myBoard.color[1] = color[1];
    myBoard.color[2] = color[2];

    if (type == 0) { // Miscellaneous annotation
        _miscAnnot.push_back(myBoard);
    } else if (type == 1) { // Time annotation
        _timeAnnot.push_back(myBoard);
    } else if (type == 2) {
        _axisAnnot.push_back(myBoard);
    }
}

void VizFeatureRenderer::ClearText(int type) {
    if (type == -1) {
        _miscAnnot.clear();
        _timeAnnot.clear();
        _axisAnnot.clear();
    }
    if (type == 0) {
        _miscAnnot.clear();
    } else if (type == 1) {
        _timeAnnot.clear();
    } else if (type == 2) {
        _axisAnnot.clear();
    }
}

#ifdef DEAD

void VizFeatureRenderer::drawRegionBounds(size_t ts) const {
    RegionParams *rParams = _visualizer->getActiveRegionParams();
    double extents[6];
    rParams->GetBox()->GetLocalExtents(extents, ts);
    _dataStatus->stretchCoords(extents);
    _dataStatus->stretchCoords(extents + 3);

    glPushAttrib(GL_CURRENT_BIT);
    glLineWidth(2.0);
    double clr[3];
    ((VizFeatureParams *)_params)->GetRegionColor(clr);
    glColor3dv(clr);
    glEnable(GL_LINE_SMOOTH);
    glBegin(GL_LINE_LOOP);
    glVertex3f(extents[0], extents[1], extents[2]);
    glVertex3f(extents[0], extents[1], extents[5]);
    glVertex3f(extents[0], extents[4], extents[5]);
    glVertex3f(extents[0], extents[4], extents[2]);
    glEnd();
    glBegin(GL_LINE_LOOP);
    glVertex3f(extents[3], extents[1], extents[2]);
    glVertex3f(extents[3], extents[1], extents[5]);
    glVertex3f(extents[3], extents[4], extents[5]);
    glVertex3f(extents[3], extents[4], extents[2]);
    glEnd();
    glBegin(GL_LINE_LOOP);
    glVertex3f(extents[0], extents[4], extents[2]);
    glVertex3f(extents[3], extents[4], extents[2]);
    glVertex3f(extents[3], extents[4], extents[5]);
    glVertex3f(extents[0], extents[4], extents[5]);
    glEnd();
    glBegin(GL_LINE_LOOP);
    glVertex3f(extents[0], extents[1], extents[2]);
    glVertex3f(extents[3], extents[1], extents[2]);
    glVertex3f(extents[3], extents[1], extents[5]);
    glVertex3f(extents[0], extents[1], extents[5]);
    glEnd();
    glDisable(GL_LINE_SMOOTH);
    glPopAttrib();
}

#endif

void VizFeatureRenderer::applyTransform(Transform *t) {
    vector<double> scale = t->GetScales();
    vector<double> origin = t->GetOrigin();
    vector<double> translate = t->GetTranslations();
    vector<double> rotate = t->GetRotations();
    assert(translate.size() == 3);
    assert(rotate.size() == 3);
    assert(scale.size() == 3);
    assert(origin.size() == 3);

    glTranslatef(origin[0], origin[1], origin[2]);
    glScalef(scale[0], scale[1], scale[2]);
    glRotatef(rotate[0], 1, 0, 0);
    glRotatef(rotate[1], 0, 1, 0);
    glRotatef(rotate[2], 0, 0, 1);
    glTranslatef(-origin[0], -origin[1], -origin[2]);

    glTranslatef(translate[0], translate[1], translate[2]);
}

void VizFeatureRenderer::InScenePaint(size_t ts) {

    VizFeatureParams *vfParams = m_paramsMgr->GetVizFeatureParams(m_winName);

    // Push or reset state
    glMatrixMode(GL_TEXTURE);
    glPushMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glPushAttrib(GL_ALL_ATTRIB_BITS);

    vector<string> winNames = m_paramsMgr->GetVisualizerNames();
    ViewpointParams *vpParams = m_paramsMgr->GetViewpointParams(winNames[0]);

    vector<string> names = m_paramsMgr->GetDataMgrNames();
    Transform *t = vpParams->GetTransform(names[0]);
    applyTransform(t);

    double mvMatrix[16];
    glGetDoublev(GL_MODELVIEW_MATRIX, mvMatrix);
    vpParams->SetModelViewMatrix(mvMatrix);

#ifdef DEAD
    if (vfParams->GetUseRegionFrame())
        drawRegionBounds(ts);
#endif

    if (vfParams->GetUseDomainFrame())
        drawDomainFrame(ts);

    if (vfParams->GetShowAxisArrows()) {

        vector<double> minExts, maxExts;
        m_dataStatus->GetActiveExtents(
            m_paramsMgr, m_winName, ts, minExts, maxExts);
        drawAxisArrows(minExts, maxExts);
    }

    AxisAnnotation *aa = vfParams->GetAxisAnnotation();
    if (aa->GetAxisAnnotationEnabled()) {
        drawAxisTics();
    }

    glPopAttrib();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_TEXTURE);
    glPopMatrix();

    glGetDoublev(GL_MODELVIEW_MATRIX, mvMatrix);
    vpParams->SetModelViewMatrix(mvMatrix);
    printOpenGLErrorMsg(m_winName.c_str());
}

#ifdef DEAD

void VizFeatureRenderer::OverlayPaint(size_t ts) {
}

#endif

void VizFeatureRenderer::drawAxisTics() {
    // Preserve the current GL color state
    glPushAttrib(GL_CURRENT_BIT);

    VizFeatureParams *vfParams = m_paramsMgr->GetVizFeatureParams(m_winName);

    vector<string> names = m_paramsMgr->GetDataMgrNames();
    AxisAnnotation *aa = vfParams->GetAxisAnnotation(names[0]);

    vector<double> origin = aa->GetAxisOrigin();
    vector<double> minTic = aa->GetMinTics();
    vector<double> maxTic = aa->GetMaxTics();
    vector<double> ticLength = aa->GetTicSize();
    vector<double> ticDir = aa->GetTicDirs();
    vector<double> numTics = aa->GetNumTics();
    double width = aa->GetTicWidth();
    vector<double> axisColor = aa->GetAxisColor();

    _drawAxes(minTic, maxTic, origin, axisColor, width);

    double pointOnAxis[3];
    double ticVec[3], drawPosn[3];

    double startPosn[3], endPosn[3];

    //Now draw tic marks for x:
    if (numTics[0] > 1 && ticLength[0] > 0.f) {
        pointOnAxis[1] = origin[1];
        pointOnAxis[2] = origin[2];
        ticVec[0] = 0.f;
        ticVec[1] = 0.f;
        ticVec[2] = 0.f;
        if (ticDir[0] == 1)
            ticVec[1] = ticLength[0];
        else
            ticVec[2] = ticLength[0];
        for (int i = 0; i < numTics[0]; i++) {
            pointOnAxis[0] = minTic[0] + (float)i * (maxTic[0] - minTic[0]) / (float)(numTics[0] - 1);

            vsub(pointOnAxis, ticVec, startPosn);
            vadd(pointOnAxis, ticVec, endPosn);
            _drawTic(startPosn, endPosn, width, axisColor);
            renderText(pointOnAxis[0], startPosn[0], startPosn[1]);
        }
    }
    //Now draw tic marks for y:
    if (numTics[1] > 1 && ticLength[1] > 0.f) {
        pointOnAxis[0] = origin[0];
        pointOnAxis[2] = origin[2];
        ticVec[0] = 0.f;
        ticVec[1] = 0.f;
        ticVec[2] = 0.f;
        if (ticDir[1] == 0)
            ticVec[0] = ticLength[1];
        else
            ticVec[2] = ticLength[1];
        for (int i = 0; i < numTics[1]; i++) {
            pointOnAxis[1] = minTic[1] + (float)i * (maxTic[1] - minTic[1]) / (float)(numTics[1] - 1);
            vsub(pointOnAxis, ticVec, startPosn);
            vadd(pointOnAxis, ticVec, endPosn);
            _drawTic(startPosn, endPosn, width, axisColor);
            renderText(pointOnAxis[1], startPosn[0], startPosn[1]);
        }
    }
    //Now draw tic marks for z:
    if (numTics[2] > 1 && ticLength[2] > 0.f) {
        pointOnAxis[0] = origin[0];
        pointOnAxis[1] = origin[1];
        ticVec[0] = 0.f;
        ticVec[1] = 0.f;
        ticVec[2] = 0.f;
        if (ticDir[2] == 0)
            ticVec[0] = ticLength[2];
        else
            ticVec[1] = ticLength[2];
        for (int i = 0; i < numTics[2]; i++) {
            pointOnAxis[2] = minTic[2] + (float)i * (maxTic[2] - minTic[2]) / (float)(numTics[2] - 1);
            vsub(pointOnAxis, ticVec, startPosn);
            vadd(pointOnAxis, ticVec, endPosn);
            _drawTic(startPosn, endPosn, width, axisColor);
            renderText(pointOnAxis[1], startPosn[0], startPosn[1]);
        }
    }
    glPopAttrib();
}

void VizFeatureRenderer::_drawAxes(
    std::vector<double> min,
    std::vector<double> max,
    std::vector<double> origin,
    std::vector<double> color,
    double width) {

    glPushAttrib(GL_CURRENT_BIT);
    glDisable(GL_LIGHTING);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glColor4d(color[0], color[1], color[2], color[3]);
    glLineWidth(width);
    glEnable(GL_LINE_SMOOTH);
    glBegin(GL_LINES);
    glVertex3d(min[0], origin[1], origin[2]);
    glVertex3d(max[0], origin[1], origin[2]);
    glVertex3d(origin[0], min[1], origin[2]);
    glVertex3d(origin[0], max[1], origin[2]);
    glVertex3d(origin[0], origin[1], min[2]);
    glVertex3d(origin[0], origin[1], max[2]);
    glEnd();
    glDisable(GL_LINE_SMOOTH);
    //glEnable(GL_LIGHTING);
    glPopAttrib();
}

void VizFeatureRenderer::_drawTic(
    double startPosn[],
    double endPosn[],
    double width,
    std::vector<double> color) {
    glPushAttrib(GL_CURRENT_BIT);
    glColor4d(color[0], color[1], color[2], color[3]);
    glLineWidth(width);
    glEnable(GL_LINE_SMOOTH);
    glBegin(GL_LINES);
    glVertex3dv(startPosn);
    glVertex3dv(endPosn);
    glEnd();
    glDisable(GL_LINE_SMOOTH);
    glPopAttrib();
}

void VizFeatureRenderer::convertPointToLonLat(
    double &xCoord, double &yCoord) {
    vector<string> names = m_paramsMgr->GetDataMgrNames();
    DataMgr *dataMgr = m_dataStatus->GetDataMgr(names[0]);
    double coords[2] = {xCoord, yCoord};

    int rc = DataMgrUtils::ConvertPCSToLonLat(dataMgr, coords, 1);
    if (!rc) {
        MyBase::SetErrMsg("Could not convert point %f, %f to Lon/Lat",
                          coords[0], coords[1]);
    }

    xCoord = coords[0];
    yCoord = coords[1];
}

Transform *VizFeatureRenderer::getCurrentTransform() {
    vector<string> winNames = m_paramsMgr->GetVisualizerNames();
    ViewpointParams *vpParams = m_paramsMgr->GetViewpointParams(winNames[0]);
    vector<string> names = m_paramsMgr->GetDataMgrNames();
    Transform *t = vpParams->GetTransform(names[0]);
    return t;
}

void VizFeatureRenderer::renderText(
    double text,
    double llx,
    double lly,
    double llz) {
    VizFeatureParams *vfParams = m_paramsMgr->GetVizFeatureParams(m_winName);
    string currentAxisDataMgr = vfParams->GetCurrentAxisDataMgrName();
    AxisAnnotation *aa = vfParams->GetAxisAnnotation(currentAxisDataMgr);

    vector<double> axisColor, txtBackground;
    axisColor = aa->GetAxisColor();
    txtBackground = aa->GetAxisBackgroundColor();
    //	cout << "A " <<  aa->GetAxisDataMgr() << " " << axisColor[0] << " " << axisColor[1] << " " << axisColor[2] << endl;
    //	cout << "B " << aa->GetAxisDataMgr() << " " << txtBackground[0] << " " << txtBackground[1] << " " << txtBackground[2] << endl << endl;
    //	axisColor.push_back(1.f); 		// alpha channel
    //	txtBackground.push_back(1.f);	// alpha channel

    //	float axisColor[] = {1.f, 0.f, 0.f, 1.f};
    //	float txtBackground[] = {0.f, 1.f, 0.f, 1.f};

    //float axisColor[] =
    //float txtBackground[] = {0.f, 1.f, 0.f, 1.f};

    int precision = (int)aa->GetAxisDigits();
    int fontSize = aa->GetAxisFontSize();

    ViewpointParams *vpParams = m_paramsMgr->GetViewpointParams(m_winName);

    std::stringstream ss;
    ss << fixed << setprecision(precision) << text;
    string textString = ss.str();

    if (_textObject != NULL) {
        delete _textObject;
        _textObject = NULL;
    }

    _textObject = new TextObject();
    _textObject->Initialize(
        _fontFile, textString, fontSize,
        axisColor, txtBackground, vpParams, TextObject::BILLBOARD,
        TextObject::CENTERTOP);

    double coords[] = {llx, lly, llz};
    _textObject->drawMe(coords);
    return;
}

void VizFeatureRenderer::drawAxisArrows(
    vector<double> minExts,
    vector<double> maxExts) {
    assert(minExts.size() == maxExts.size());
    while (minExts.size() < 3) {
        minExts.push_back(0.0);
        maxExts.push_back(0.0);
    }

    // Preserve the current GL color state
    glPushAttrib(GL_CURRENT_BIT);

    float origin[3];
    float maxLen = -1.f;

    VizFeatureParams *vfParams = m_paramsMgr->GetVizFeatureParams(m_winName);

    vector<double> axisArrowCoords = vfParams->GetAxisArrowCoords();

    for (int i = 0; i < 3; i++) {
        origin[i] = minExts[i] + (axisArrowCoords[i]) * (maxExts[i] - minExts[i]);
        if (maxExts[i] - minExts[i] > maxLen) {
            maxLen = maxExts[i] - minExts[i];
        }
    }
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    float len = maxLen * 0.2f;
    glColor3f(1.f, 0.f, 0.f);
    glLineWidth(4.0);
    glEnable(GL_LINE_SMOOTH);
    glBegin(GL_LINES);
    glVertex3fv(origin);
    glVertex3f(origin[0] + len, origin[1], origin[2]);

    glEnd();
    glBegin(GL_TRIANGLES);
    glVertex3f(origin[0] + len, origin[1], origin[2]);
    glVertex3f(origin[0] + .8 * len, origin[1] + .1 * len, origin[2]);
    glVertex3f(origin[0] + .8 * len, origin[1], origin[2] + .1 * len);

    glVertex3f(origin[0] + len, origin[1], origin[2]);
    glVertex3f(origin[0] + .8 * len, origin[1], origin[2] + .1 * len);
    glVertex3f(origin[0] + .8 * len, origin[1] - .1 * len, origin[2]);

    glVertex3f(origin[0] + len, origin[1], origin[2]);
    glVertex3f(origin[0] + .8 * len, origin[1] - .1 * len, origin[2]);
    glVertex3f(origin[0] + .8 * len, origin[1], origin[2] - .1 * len);

    glVertex3f(origin[0] + len, origin[1], origin[2]);
    glVertex3f(origin[0] + .8 * len, origin[1], origin[2] - .1 * len);
    glVertex3f(origin[0] + .8 * len, origin[1] + .1 * len, origin[2]);
    glEnd();

    glColor3f(0.f, 1.f, 0.f);
    glBegin(GL_LINES);
    glVertex3fv(origin);
    glVertex3f(origin[0], origin[1] + len, origin[2]);
    glEnd();
    glBegin(GL_TRIANGLES);
    glVertex3f(origin[0], origin[1] + len, origin[2]);
    glVertex3f(origin[0] + .1 * len, origin[1] + .8 * len, origin[2]);
    glVertex3f(origin[0], origin[1] + .8 * len, origin[2] + .1 * len);

    glVertex3f(origin[0], origin[1] + len, origin[2]);
    glVertex3f(origin[0], origin[1] + .8 * len, origin[2] + .1 * len);
    glVertex3f(origin[0] - .1 * len, origin[1] + .8 * len, origin[2]);

    glVertex3f(origin[0], origin[1] + len, origin[2]);
    glVertex3f(origin[0] - .1 * len, origin[1] + .8 * len, origin[2]);
    glVertex3f(origin[0], origin[1] + .8 * len, origin[2] - .1 * len);

    glVertex3f(origin[0], origin[1] + len, origin[2]);
    glVertex3f(origin[0], origin[1] + .8 * len, origin[2] - .1 * len);
    glVertex3f(origin[0] + .1 * len, origin[1] + .8 * len, origin[2]);
    glEnd();
    glColor3f(0.f, 0.3f, 1.f);
    glBegin(GL_LINES);
    glVertex3fv(origin);
    glVertex3f(origin[0], origin[1], origin[2] + len);
    glEnd();
    glBegin(GL_TRIANGLES);
    glVertex3f(origin[0], origin[1], origin[2] + len);
    glVertex3f(origin[0] + .1 * len, origin[1], origin[2] + .8 * len);
    glVertex3f(origin[0], origin[1] + .1 * len, origin[2] + .8 * len);

    glVertex3f(origin[0], origin[1], origin[2] + len);
    glVertex3f(origin[0], origin[1] + .1 * len, origin[2] + .8 * len);
    glVertex3f(origin[0] - .1 * len, origin[1], origin[2] + .8 * len);

    glVertex3f(origin[0], origin[1], origin[2] + len);
    glVertex3f(origin[0] - .1 * len, origin[1], origin[2] + .8 * len);
    glVertex3f(origin[0], origin[1] - .1 * len, origin[2] + .8 * len);

    glVertex3f(origin[0], origin[1], origin[2] + len);
    glVertex3f(origin[0], origin[1] - .1 * len, origin[2] + .8 * len);
    glVertex3f(origin[0] + .1 * len, origin[1], origin[2] + .8 * len);
    glEnd();

    glDisable(GL_LINE_SMOOTH);
    // Revert to previous GL color state
    glPopAttrib();
}
