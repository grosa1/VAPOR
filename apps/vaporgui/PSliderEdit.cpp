#include "PSliderEdit.h"
#include "VLineItem.h"
#include <vapor/ParamsBase.h>
#include "VSliderEdit.h"
#include "VDoubleSliderEdit.h"
#include "VIntSliderEdit.h"

#define USER_RANGE_MIN_TAG "_PWidget_UserRangeMinimum"
#define USER_RANGE_MAX_TAG "_PWidget_UserRangeMaximum"

// ===============================
//        PDoubleSliderEdit
// ===============================

PDoubleSliderEdit::PDoubleSliderEdit(const std::string &tag, const std::string &label) : PLineItem(tag, label, _sliderEdit = new VDoubleSliderEdit(0, 1, 0, true))
{
    connect(_sliderEdit, &VDoubleSliderEdit::ValueChanged, this, &PDoubleSliderEdit::valueChanged);
    connect(_sliderEdit, &VDoubleSliderEdit::ValueChangedIntermediate, this, &PDoubleSliderEdit::valueChangedIntermediate);
    connect(_sliderEdit, &VDoubleSliderEdit::MinimumChanged, this, &PDoubleSliderEdit::minimumChanged);
    connect(_sliderEdit, &VDoubleSliderEdit::MaximumChanged, this, &PDoubleSliderEdit::maximumChanged);
}

PDoubleSliderEdit *PDoubleSliderEdit::SetRange(double min, double max)
{
    _defaultRangeMin = min;
    _defaultRangeMax = max;
    _sliderEdit->SetMinimum(min);
    _sliderEdit->SetMaximum(max);
    return this;
}

PDoubleSliderEdit *PDoubleSliderEdit::AllowUserRange(bool allowed)
{
    _sliderEdit->AllowUserRange(allowed);
    return this;
}

void PDoubleSliderEdit::updateGUI() const
{
    auto p = getParams();
    _sliderEdit->SetMinimum(p->GetValueDouble(getTag() + USER_RANGE_MIN_TAG, _defaultRangeMin));
    _sliderEdit->SetMaximum(p->GetValueDouble(getTag() + USER_RANGE_MAX_TAG, _defaultRangeMax));
    _sliderEdit->SetValue(getParamsDouble());
}

void PDoubleSliderEdit::valueChanged(double v) { 
    setParamsDouble(v); 

    /*if (v==.123) setParamsDouble(.234);
    if (v==.234) setParamsDouble(.345);
    if (v==.345) setParamsDouble(.456);
    if (v==.456) setParamsDouble(.567);
    if (v==.567) setParamsDouble(.678);
    if (v==.678) setParamsDouble(.789);
    if (v==.789) setParamsDouble(.890);
    if (v==.890) setParamsDouble(1.);*/
    /*if (v==.123) valueChanged(.234); Update(_params,_paramsMgr,_dataMgr);
    if (v==.234) valueChanged(.345); Update(_params,_paramsMgr,_dataMgr);
    if (v==.345) valueChanged(.456); Update(_params,_paramsMgr,_dataMgr);
    if (v==.456) valueChanged(.567); Update(_params,_paramsMgr,_dataMgr);
    if (v==.567) valueChanged(.678); Update(_params,_paramsMgr,_dataMgr);
    if (v==.678) valueChanged(.789); Update(_params,_paramsMgr,_dataMgr);
    if (v==.789) valueChanged(.890); Update(_params,_paramsMgr,_dataMgr);
    if (v==.890) valueChanged(.9); Update(_params,_paramsMgr,_dataMgr);*/
}

void PDoubleSliderEdit::valueChangedIntermediate(double v) { dynamicSetParamsDouble(v); }

void PDoubleSliderEdit::minimumChanged(double v)
{
    auto p = getParams();
    p->SetValueDouble(getTag() + USER_RANGE_MIN_TAG, "", v);
}

void PDoubleSliderEdit::maximumChanged(double v)
{
    auto p = getParams();
    p->SetValueDouble(getTag() + USER_RANGE_MAX_TAG, "", v);
}

// ===============================
//       PIntegerSliderEdit
// ===============================

PIntegerSliderEdit::PIntegerSliderEdit(const std::string &tag, const std::string &label) : PLineItem(tag, label, _sliderEdit = new VIntSliderEdit(0, 1, 0, true))
{
    connect(_sliderEdit, &VIntSliderEdit::ValueChanged, this, &PIntegerSliderEdit::valueChanged);
    connect(_sliderEdit, &VIntSliderEdit::ValueChangedIntermediate, this, &PIntegerSliderEdit::valueChangedIntermediate);
    connect(_sliderEdit, &VIntSliderEdit::MinimumChanged, this, &PIntegerSliderEdit::minimumChanged);
    connect(_sliderEdit, &VIntSliderEdit::MaximumChanged, this, &PIntegerSliderEdit::maximumChanged);
}

PIntegerSliderEdit *PIntegerSliderEdit::SetRange(int min, int max)
{
    _defaultRangeMin = min;
    _defaultRangeMax = max;
    _sliderEdit->SetMinimum(min);
    _sliderEdit->SetMaximum(max);
    return this;
}

PIntegerSliderEdit *PIntegerSliderEdit::AllowUserRange(bool allowed)
{
    _sliderEdit->AllowUserRange(allowed);
    return this;
}

void PIntegerSliderEdit::updateGUI() const
{
    auto p = getParams();
    _sliderEdit->SetMinimum(p->GetValueLong(getTag() + USER_RANGE_MIN_TAG, _defaultRangeMin));
    _sliderEdit->SetMaximum(p->GetValueLong(getTag() + USER_RANGE_MAX_TAG, _defaultRangeMax));
    _sliderEdit->SetValue(getParamsLong());
}

void PIntegerSliderEdit::valueChanged(int v) { setParamsLong(v); }

void PIntegerSliderEdit::valueChangedIntermediate(int v) { dynamicSetParamsLong(v); }

void PIntegerSliderEdit::minimumChanged(int v)
{
    auto p = getParams();
    p->SetValueLong(getTag() + USER_RANGE_MIN_TAG, "", v);
}

void PIntegerSliderEdit::maximumChanged(int v)
{
    auto p = getParams();
    p->SetValueLong(getTag() + USER_RANGE_MAX_TAG, "", v);
}
