#include <iostream>
#include "vapor/Advection.h"
#include <fstream>
#include <algorithm>

using namespace flow;

// Constructor;
Advection::Advection() : _lowerAngle(3.0f), _upperAngle(15.0f)
{
    _lowerAngleCos = glm::cos(glm::radians(_lowerAngle));
    _upperAngleCos = glm::cos(glm::radians(_upperAngle));

    std::fill(_isPeriodic.begin(), _isPeriodic.end(), false);
    std::fill(_periodicBounds.begin(), _periodicBounds.end(), glm::vec2(0.0, 0.0));
}

void Advection::UseSeedParticles(const std::vector<Particle> &seeds)
{
    _streams.clear();
    _streams.resize(seeds.size());
    for (size_t i = 0; i < seeds.size(); i++)
      _streams[i].push_back(seeds[i]);

    _separatorCount.assign(seeds.size(), 0);
}

int Advection::CheckReady() const
{
    for (const auto &s : _streams) {
        if (s.size() < 1) return NO_SEED_PARTICLE_YET;
    }

    return 0;
}

int Advection::AdvectSteps(Field *velocity, double deltaT, size_t maxSteps, ADVECTION_METHOD method)
{
    int ready = CheckReady();
    if (ready != 0) return ready;
    bool happened = false;

    // Observation: user parameters are not gonna change while this function executes.
    // Action: lock these parameters.
    if (velocity->LockParams() != 0) 
      return PARAMS_ERROR;

    // The particle advection process can be parallelized per particle
    // Each stream represents a trajectory for a single particle
    #pragma omp parallel for
    for (size_t streamIdx = 0; streamIdx < _streams.size(); streamIdx++) {
        auto& s = _streams[streamIdx];
        size_t numberOfSteps = s.size() - _separatorCount[streamIdx];
        while (numberOfSteps < maxSteps) {
            auto &past0 = s.back();
            if (past0.IsSpecial())    // If the last particle is marked "special,"
                break;                // terminate stream immediately.

            double dt = deltaT;
            if (s.size() > 2)    // If there are at least 3 particles in the stream and
            {                    // neither is a separator, we also adjust *dt*
                const auto &past1 = s[s.size() - 2];
                const auto &past2 = s[s.size() - 3];
                if ((!past1.IsSpecial()) && (!past2.IsSpecial())) {
                    // We enforce a factor of 20.0f as a limit of how much the step size
                    // can be adjusted by _calcAdjustFactor().
                    // I.e., the adjusted value can be at most 20X larger or 20X smaller.
                    // The choice of 20.0f is just an empirical value that seems to work well.
                    double mindt = deltaT / 20.0, maxdt = deltaT * 20.0;
                    dt = past0.time - past1.time;    // step size used by last integration
                    dt *= _calcAdjustFactor(past2, past1, past0);
                    if (dt > 0)    // integrate forward
                        dt = glm::clamp(dt, mindt, maxdt);
                    else    // integrate backward
                        dt = glm::clamp(dt, maxdt, mindt);
                }
            }

            Particle p1;
            int      rv = 0;
            switch (method) {
            case ADVECTION_METHOD::EULER:
                rv = _advectEuler(velocity, past0, dt, p1);
                _printNonZero(rv, __FILE__, __func__, __LINE__);
                break;
            case ADVECTION_METHOD::RK4:
                rv = _advectRK4(velocity, past0, dt, p1);
                _printNonZero(rv, __FILE__, __func__, __LINE__);
                break;
            }

            if (rv == SUCCESS) {
                // Bookmark_1
                // The new particle *may* be the same as the old particle in case
                // there's a sink, meaning the velocity is zero.
                // In that case, we mark p1 as "special" and terminate the current stream.
                if (p1.location == past0.location) {
                    p1.SetSpecial(true);
                    s.emplace_back(p1);
                    _separatorCount[streamIdx]++;
                    break;
                } else {
                    happened = true;
                    s.emplace_back(p1);
                    numberOfSteps++;
                }
            } else if (rv == MISSING_VAL) {
                // Bookmark_2
                // This is the annoying part: there are multiple possiblities.
                // 1) past0 is really located at a missing value location;
                // 2) past0 is inside the volume, but really close to the boundary,
                //    causing RK4 method to fail;
                // 3) past0 is not at a missing location, but out of the volume.
                //
                // Note that we need to detect and deal with each of these possibilities
                //   here instead of using the periodic capabilities of a grid class,
                //   because the advection code needs to have knowledge when a pathline
                //   exits from one side and comes back from another sice, and record
                //   this event by inserting a separator. The separator will later be used
                //   by the rendering code to break a pathline into segments.

                glm::vec3 vel;
                bool isMissing = (velocity->GetVelocity(past0.time, past0.location, vel) == MISSING_VAL);
                bool isInside = velocity->InsideVolumeVelocity(past0.time, past0.location);

                if (isInside && isMissing) {    // Case 1)
                    // We identified a particle at a bad location.
                    // We mark it as special, and terminate the current stream.
                    past0.SetSpecial(true);
                    _separatorCount[streamIdx]++;
                    break;
                } else if (isInside && (!isMissing)) {    // Case 2)
                    // Use Euler advection for this particle.
                    rv = _advectEuler(velocity, past0, dt, p1);
                    assert(rv == 0);
                    s.emplace_back(p1);
                    numberOfSteps++;
                } else {    // Case 3)
                    // We identified a particle that's out of the volume.
                    // We treat it depending on field periodicity.
                    // In case of no periodicity, we mark this particle special and
                    //    terminate the current stream.
                    // In case of periodicity enabled, we apply it!
                    if ((!_isPeriodic[0]) && (!_isPeriodic[1]) && (!_isPeriodic[2])) {
                        past0.SetSpecial(true);
                        _separatorCount[streamIdx]++;
                        break;
                    } else {
                        auto loc = past0.location;
                        for (int i = 0; i < 3; i++) {
                            if (_isPeriodic[i]) 
                              loc[i] = _applyPeriodic(loc[i], _periodicBounds[i][0], _periodicBounds[i][1]);
                        }

                        // Notice that loc isn't guaranteed to be inside the volume right now,
                        // since periodic ain't enabled for all directions.
                        // As a result, we need to test again
                        if (velocity->InsideVolumeVelocity(past0.time, loc)) {
                            past0.location = loc;
                            Particle separator;
                            separator.SetSpecial(true);
                            auto it = s.end();
                            --it;
                            s.insert(it, separator);
                            _separatorCount[streamIdx]++;
                        } else {
                            past0.SetSpecial(true);
                            _separatorCount[streamIdx]++;
                            break;
                        }
                    }
                }

            }       // end (rv == MISSING_VAL) condition
            else    // Advection wasn't successful for other reasons
                break;

        }    // end loop for particle
    }        // end loop for streams

    velocity->UnlockParams();

    if (happened)
        return SUCCESS;
    else
        return NO_ADVECT_HAPPENED;
}

int Advection::AdvectTillTime(Field *velocity, double startT, double deltaT, double targetT, ADVECTION_METHOD method)
{
    int ready = CheckReady();
    if (ready != 0) return ready;

    bool   happened = false;
    size_t streamIdx = 0;
    size_t maxSteps = 10000;
    for (auto &s : _streams)    // Process one stream at a time
    {
        Particle p0 = s.back();    // Start from the last particle in this stream
        if (p0.time < startT)      // Skip this stream if it didn't advance to startT
            continue;

        if (p0.IsSpecial())       // Also skip this tream if it was marked special.
            continue;

        size_t thisStep = 0;

        while (p0.time < targetT) {

            // Check if the particle is inside of the volume.
            // Wrap it along periodic dimensions if applicable.
            if (!velocity->InsideVolumeVelocity(p0.time, p0.location)) {
                bool locChanged = false;
                auto itr = s.end();
                --itr;    // pointing to the last element
                auto loc = itr->location;
                for (int i = 0; i < 3; i++) {
                    if (_isPeriodic[i]) {
                        loc[i] = _applyPeriodic(loc[i], _periodicBounds[i][0], _periodicBounds[i][1]);
                        locChanged = true;
                    }
                }
                if (!locChanged) {  // no dimension is periodic, append a separator
                    Particle separator;
                    separator.SetSpecial(true);
                    s.push_back(separator);
                    _separatorCount[streamIdx]++;
                    break;          // break the while loop
                }

                // See if the new location is inside of the volume
                if (velocity->InsideVolumeVelocity(itr->time, loc)) {
                    itr->location = loc;
                    p0 = *itr;    // p0 is equal to the wrapped particle

                    Particle separator;
                    separator.SetSpecial(true);
                    s.insert(itr, separator);
                    _separatorCount[streamIdx]++;
                } else {  // Still outside, so we terminate the stream!
                    Particle separator;
                    separator.SetSpecial(true);
                    s.push_back(separator);
                    _separatorCount[streamIdx]++;
                    break;    // break the while loop
                }
            } // Finish the out-of-volume condition

            double dt = deltaT;
            if (s.size() > 2)    // If there are at least 3 particles in the stream,
            {                    // we also adjust *dt*
                double mindt = deltaT / 20.0, maxdt = deltaT * 20.0;
                maxdt = glm::min(maxdt, targetT - p0.time);
                const auto &past1 = s[s.size() - 2];
                const auto &past2 = s[s.size() - 3];
                if ((!past1.IsSpecial()) && (!past2.IsSpecial())) {
                    dt = p0.time - past1.time;    // step size used by last integration
                    dt *= _calcAdjustFactor(past2, past1, p0);
                    dt = glm::clamp(dt, mindt, maxdt);
                }
            }

            Particle p1;
            int      rv = 0;
            switch (method) {
            case ADVECTION_METHOD::EULER:
                rv = _advectEuler(velocity, p0, dt, p1);
                _printNonZero(rv, __FILE__, __func__, __LINE__);
                break;
            case ADVECTION_METHOD::RK4:
                rv = _advectRK4(velocity, p0, dt, p1);
                _printNonZero(rv, __FILE__, __func__, __LINE__);
                break;
            }

            if (rv == SUCCESS) {
                // Check out Bookmark_1
                if (p1.location == p0.location) {
                    p1.SetSpecial(true);
                    s.push_back(p1);
                    _separatorCount[streamIdx]++;
                    break;
                } else {
                    happened = true;
                    s.push_back(p1);
                    p0 = p1;
                }
            } else if (rv == MISSING_VAL) {
                // Check out Bookmark_2
                glm::vec3 vel;
                bool isMissing = (velocity->GetVelocity(p0.time, p0.location, vel) == MISSING_VAL);
                bool isInside = velocity->InsideVolumeVelocity(p0.time, p0.location);

                if (isInside && isMissing) {
                    p1.SetSpecial(true);
                    s.push_back(p1);
                    _separatorCount[streamIdx]++;
                    break;
                } else if (isInside && (!isMissing)) {
                    rv = _advectEuler(velocity, p0, dt, p1);
                    assert(rv == 0);
                    s.push_back(p1);
                    p0 = p1;
                } else {
                    auto loc = p0.location;
                    for (int i = 0; i < 3; i++) {
                        if (_isPeriodic[i])
                            loc[i] = _applyPeriodic(loc[i], _periodicBounds[i][0], _periodicBounds[i][1]);
                    }

                    if (velocity->InsideVolumeVelocity(p0.time, loc)) {
                        p1.SetSpecial(true);
                        auto it = s.end();
                        --it;
                        s.insert(it, p1);
                        it = s.end();
                        --it;
                        it->location = loc;
                        _separatorCount[streamIdx]++;
                    } else {
                        p1.SetSpecial(true);
                        s.push_back(p1);
                        _separatorCount[streamIdx]++;
                        break;
                    }
                }
            } // finish handling missing value

            // Another termination criterion: when advecting at least 10,000 steps and
            // more than 10X more than the previous max num of steps.
            //
            if (++thisStep == maxSteps) {
                thisStep = maxSteps / 10;
                p1.SetSpecial(true);
                s.push_back(p1);
                _separatorCount[streamIdx]++;
                break;
            }
        }    // Finish advecting one particle

        maxSteps = std::max(maxSteps, thisStep * 10);

        streamIdx++;

    }    // Finish advecting all particles

    if (happened)
        return ADVECT_HAPPENED;
    else
        return 0;
}

int Advection::CalculateParticleValues(Field *scalar, bool skipNonZero)
{
    // For steady fields, we calculate values one stream at a time
    if (scalar->IsSteady) {
        if (scalar->LockParams() != 0) return PARAMS_ERROR;

        _valueVarName = scalar->ScalarName;

        for (auto &s : _streams) {
            for (auto &p : s) {
                // Skip this particle if it's a separator
                if (p.IsSpecial()) continue;

                // Do not evaluate this particle if its value is non-zero
                if (skipNonZero && p.value != 0.0f) continue;

                float value;
                int   rv = scalar->GetScalar(p.time, p.location, value);
                if (rv == 0)            // The end of a stream could be outside of the volume,
                    p.value = value;    // so let's only color it when the return value is 0.
            }
        }

        scalar->UnlockParams();
    }
    // For unsteady fields, we calculate values at one timestep at a time
    else {
        size_t mostSteps = 0;
        for (auto &s : _streams) {
            if (s.size() > mostSteps) mostSteps = s.size();
        }

        _valueVarName = scalar->ScalarName;

        for (size_t i = 0; i < mostSteps; i++) {
            for (auto &s : _streams) {
                if (i < s.size()) {
                    auto &p = s[i];
                    if (p.IsSpecial()) continue;

                    // Do not evaluate this particle if its value is non-zero
                    if (skipNonZero && p.value != 0.0f) continue;

                    float val;
                    int   rv = scalar->GetScalar(p.time, p.location, val);
                    if (rv == 0) p.value = val;
                }
            }    // end of a stream
        }        // end of all steps
    }

    return 0;
}


int Advection::CalculateParticleIntegratedValues(Field *scalar, const bool skipNonZero, const float distScale, const std::vector<double> &integrateWithinVolumeMin,
                                                 const std::vector<double> &integrateWithinVolumeMax)
{
    // For steady fields, we calculate values one stream at a time
    if (scalar->IsSteady) {
        if (scalar->LockParams() != 0) return PARAMS_ERROR;

        _valueVarName = scalar->ScalarName;

        for (auto &s : _streams) {
            if (s.size() && !s[0].IsSpecial()) s[0].value = 0;

            for (int i = 1; i < s.size(); i++) {
                auto &prev = s[i - 1];
                auto &p = s[i];
                _calculateParticleIntegratedValue(p, prev, scalar, skipNonZero, distScale, integrateWithinVolumeMin, integrateWithinVolumeMax);
            }
        }

        scalar->UnlockParams();
    }
    // For unsteady fields, we calculate values at one timestep at a time
    else {
        size_t mostSteps = 0;
        for (auto &s : _streams) {
            if (s.size() > mostSteps) mostSteps = s.size();
        }

        _valueVarName = scalar->ScalarName;

        for (auto &s : _streams)
            if (s.size() && !s[0].IsSpecial()) s[0].value = 0;

        for (size_t i = 1; i < mostSteps; i++) {
            for (auto &s : _streams) {
                if (i < s.size()) {
                    auto &prev = s[i - 1];
                    auto &p = s[i];
                    _calculateParticleIntegratedValue(p, prev, scalar, skipNonZero, distScale, integrateWithinVolumeMin, integrateWithinVolumeMax);
                }
            }    // end of a stream
        }        // end of all steps
    }

    return 0;
}

void Advection::_calculateParticleIntegratedValue(Particle &p, const Particle &prev, const Field *scalarField, const bool skipNonZero, const float distScale,
                                                  const std::vector<double> &integrateWithinVolumeMin, const std::vector<double> &integrateWithinVolumeMax) const
{
    // Skip this particle if it is a separator
    if (p.IsSpecial()) return;
    if (prev.IsSpecial()) {
        p.value = 0;
        return;
    }

    // Do not evaluate this particle if its value is non-zero
    if (skipNonZero && p.value != 0.0f) return;

    if (!_isParticleInsideVolume(p, integrateWithinVolumeMin, integrateWithinVolumeMax)) {
        p.value = prev.value;
        return;
    }

    float value;
    int   rv = scalarField->GetScalar(p.time, p.location, value);
    if (rv != 0) {    // If non-0, then outside the volume
        p.value = prev.value;
        return;
    }

    float dist = glm::distance(prev.location, p.location);
    p.value = prev.value + value * dist * distScale;
}

void Advection::SetAllStreamValuesToFinalValue(int realNSamples)
{
    for (auto &s : _streams) {
        float finalValue = 0;

        int sampleCount = 0;
        for (auto &p : s) {
            if (!p.IsSpecial()) {
                finalValue = p.value;
                sampleCount++;
            }
            if (sampleCount == realNSamples) break;
        }

        int setCount = 0;
        for (auto &p : s) {
            if (!p.IsSpecial()) {
                p.value = finalValue;
                setCount++;
            }
            if (setCount == sampleCount) break;
        }
    }
}

int Advection::CalculateParticleProperties(Field *scalar)
{
    // Test if this scalar property is already calculated.
    if (std::find(_propertyVarNames.cbegin(), _propertyVarNames.cend(), scalar->ScalarName) != _propertyVarNames.cend()) return 0;

    // Proceed if there is no current scalar property
    _propertyVarNames.emplace_back(scalar->ScalarName);

    // Test if this scalar field is the same as the one used to calculate particle values,
    //   if so, copy over the values.
    if (scalar->ScalarName == _valueVarName) {
        for (auto &s : _streams) {
            for (auto &p : s) { p.AttachProperty(p.value); }
        }

        return 0;
    }

    // In case this property field is a brand new variable, we do the actual sampling work.
    if (scalar->IsSteady) {
        if (scalar->LockParams() != 0) return PARAMS_ERROR;

        for (auto &s : _streams) {
            for (auto &p : s) {
                if (p.IsSpecial()) continue;

                // At the end of a flow line, a particle might be outside of the volume.
                // We attach something in that case as well.
                float val = std::nanf("1");
                scalar->GetScalar(p.time, p.location, val);
                p.AttachProperty(val);
            }
        }
    } else {
        size_t mostSteps = 0;
        for (const auto &s : _streams)
            if (s.size() > mostSteps) mostSteps = s.size();

        for (size_t i = 0; i < mostSteps; i++) {
            for (auto &s : _streams) {
                if (i < s.size()) {
                    auto &p = s[i];
                    if (p.IsSpecial()) continue;

                    float value = std::nanf("1");
                    scalar->GetScalar(p.time, p.location, value);
                    p.AttachProperty(value);
                }
            }
        }
    }

    return 0;
}

void Advection::CalculateParticleHistogram(std::vector<double> &outBounds, std::vector<long> &bins)
{
    std::vector<float> samples;

    for (const auto &s : _streams)
        for (const auto &p : s)
            if (!p.IsSpecial()) samples.push_back(p.value);

    auto  bounds = std::minmax_element(samples.begin(), samples.end());
    float minValue = *bounds.first;
    float maxValue = *bounds.second;
    float range = maxValue - minValue;

    int nBins = bins.size();
    assert(nBins != 0);
    std::fill(bins.begin(), bins.end(), 0);

    for (const auto &s : samples) { bins[std::min(nBins - 1, std::max(0, (int)((nBins - 1) * (s - minValue) / range)))]++; }

    outBounds.resize(2);
    outBounds[0] = minValue;
    outBounds[1] = maxValue;
}

int Advection::_advectEuler(Field *velocity, const Particle &p0, double dt, Particle &p1) const
{
    glm::vec3 v0;
    int       rv = velocity->GetVelocity(p0.time, p0.location, v0);
    if (rv != 0) return rv;
    float dt32 = float(dt);    // glm is strict about data types (which is a good thing).
    p1.location = p0.location + dt32 * v0;
    p1.time = p0.time + dt;
    return 0;
}

int Advection::_advectRK4(Field *velocity, const Particle &p0, double dt, Particle &p1) const
{
    glm::vec3    k1, k2, k3, k4;
    const double dt_half = dt * 0.5;
    const float  dt32 = float(dt);              // glm is strict about data types (which is a good thing).
    const float  dt_half32 = float(dt_half);    // glm is strict about data types (which is a good thing).
    int          rv = 0;
    rv = velocity->GetVelocity(p0.time, p0.location, k1);
    _printNonZero(rv, __FILE__, __func__, __LINE__);
    if (rv != 0) return rv;
    rv = velocity->GetVelocity(p0.time + dt_half, p0.location + dt_half32 * k1, k2);
    _printNonZero(rv, __FILE__, __func__, __LINE__);
    if (rv != 0) return rv;
    rv = velocity->GetVelocity(p0.time + dt_half, p0.location + dt_half32 * k2, k3);
    _printNonZero(rv, __FILE__, __func__, __LINE__);
    if (rv != 0) return rv;
    rv = velocity->GetVelocity(p0.time + dt, p0.location + dt32 * k3, k4);
    _printNonZero(rv, __FILE__, __func__, __LINE__);
    if (rv != 0) return rv;
    p1.location = p0.location + dt32 / 6.0f * (k1 + 2.0f * (k2 + k3) + k4);
    p1.time = p0.time + dt;

    return 0;
}

float Advection::_calcAdjustFactor(const Particle &p2, const Particle &p1, const Particle &p0) const
{
    glm::vec3 p2p1 = p1.location - p2.location;
    glm::vec3 p1p0 = p0.location - p1.location;
    float     denominator = glm::length(p2p1) * glm::length(p1p0);
    float     cosine;
    if (denominator < 1e-7)
        return 1.0f;
    else
        cosine = glm::dot(p2p1, p1p0) / denominator;

    if (cosine > _lowerAngleCos)    // Less than "_lowerAngle" degrees
        return 1.25f;
    else if (cosine < _upperAngleCos)    // More than "_upperAngle" degrees
        return 0.5f;
    else
        return 1.0f;
}

size_t Advection::GetNumberOfStreams() const { return _streams.size(); }

const std::vector<Particle> &Advection::GetStreamAt(size_t i) const
{
    // Since this function is almost always used together with GetNumberOfStreams(),
    // I'm offloading the range check to std::vector.
    return _streams.at(i);
}

size_t Advection::GetMaxNumOfPart() const
{
    size_t max = 0;
    size_t idx = 0;
    for (const auto &s : _streams) {
        size_t num = s.size() - _separatorCount[idx];
        if (num > max) max = num;
        idx++;
    }
    return max;
}

void Advection::ClearParticleProperties()
{
    _propertyVarNames.clear();
    for (auto &stream : _streams)
        for (auto &part : stream) part.ClearProperty();
}

void Advection::RemoveParticleProperty(const std::string &varToRemove)
{
    auto itr = std::find(_propertyVarNames.begin(), _propertyVarNames.end(), varToRemove);

    // Do nothing if `varToRemove` does not exist
    if (itr == _propertyVarNames.end())
        return;
    else {
        auto rmI = std::distance(_propertyVarNames.begin(), itr);
        _propertyVarNames.erase(itr);
        for (auto &stream : _streams)
            for (auto &part : stream) part.RemoveProperty(rmI);
    }
}

void Advection::ResetParticleValues()
{
    for (auto &stream : _streams) {
        std::for_each(stream.begin(), stream.end(), [](Particle &p) {
            if (!p.IsSpecial()) p.value = 0.0f;
        });
    }
}

void Advection::SetXPeriodicity(bool isPeri, float min, float max)
{
    _isPeriodic[0] = isPeri;
    if (isPeri)
        _periodicBounds[0] = glm::vec2(min, max);
    else
        _periodicBounds[0] = glm::vec2(0.0f);
}

void Advection::SetYPeriodicity(bool isPeri, float min, float max)
{
    _isPeriodic[1] = isPeri;
    if (isPeri)
        _periodicBounds[1] = glm::vec2(min, max);
    else
        _periodicBounds[1] = glm::vec2(0.0f);
}

void Advection::SetZPeriodicity(bool isPeri, float min, float max)
{
    _isPeriodic[2] = isPeri;
    if (isPeri)
        _periodicBounds[2] = glm::vec2(min, max);
    else
        _periodicBounds[2] = glm::vec2(0.0f);
}

float Advection::_applyPeriodic(float val, float min, float max) const
{
    if (min >= max)    // ill params, return val
        return val;
    else if (val >= min && val <= max)    // nothing needs to change
        return val;

    // Let's do some serious work
    float span = max - min;
    float pval = val;
    if (val < min) {
        while (pval < min) pval += span;
        return pval;
    } else    // val > max
    {
        while (pval > max) pval -= span;
        return pval;
    }
}

void Advection::_printNonZero(int rtn, const char *file, const char *func, int line) const
{
#ifdef VPRINT
    if (rtn != 0) {    // only print non-zero values
        printf("Rtn == %d: %s:(%s):%d\n", rtn, file, func, line);
    }
#endif
}

auto Advection::GetValueVarName() const -> std::string { return _valueVarName; }

auto Advection::GetPropertyVarNames() const -> std::vector<std::string> { return _propertyVarNames; }

bool Advection::_isParticleInsideVolume(const Particle &p, const std::vector<double> &min, const std::vector<double> &max)
{
    if (p.location[0] < min[0] || p.location[1] < min[1] || p.location[0] > max[0] || p.location[1] > max[1]) { return false; }
    if (min.size() > 2 && (p.location[2] < min[2] || p.location[2] > max[2])) { return false; }
    return true;
}
