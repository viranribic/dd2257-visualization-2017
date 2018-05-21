
/*********************************************************************
 *  Author  : Himangshu Saikia
 *  Init    : Wednesday, September 20, 2017 - 12:04:15
 *
 *  Project : KTH Inviwo Modules
 *
 *  License : Follows the Inviwo BSD license model
 *********************************************************************
 */

#pragma once

#include <dd2257lab3/dd2257lab3moduledefine.h>
#include <inviwo/core/common/inviwo.h>
#include <inviwo/core/datastructures/geometry/basicmesh.h>
#include <inviwo/core/datastructures/volume/volumeram.h>

#define INTEGRATOR_DIRECTION_FORWARD 0
#define INTEGRATOR_DIRECTION_BACKWARD 1

#define INTEGRATOR_STOP_STEPS 0
#define INTEGRATOR_STOP_ARC 1
#define INTEGRATOR_STOP_BOUNDARY 2
#define INTEGRATOR_STOP_ZEROES 3
#define INTEGRATOR_STOP_SLOW 4

#define INTEGRATOR_EULER 0
#define INTEGRATOR_RK4 1

namespace inviwo
{

class IVW_MODULE_DD2257LAB3_API Integrator
{ 
//Friends
//Types
public:

//Construction / Deconstruction
public:
    Integrator();
    virtual ~Integrator() = default;

//Methods
public:
    static vec2 sampleFromField(const VolumeRAM* vr, size3_t dims, const vec2& position);


	// TODO: Implement the methods below (one integration step with either Euler or 
    // Runge-Kutte of 4th order integration method)
    // Pass any other properties that influence the integration process
    static vec2 RK4(const VolumeRAM* vr, size3_t dims, const vec2& position, double delta);
    static vec2 Euler(const VolumeRAM* vr, size3_t dims, const vec2& position, double delta);
	
	static vec2 Euler_(const VolumeRAM * vr, size3_t dims, const vec2 & position, double delta, int direction, bool inDirField);
	static vec2 RK4_(const VolumeRAM* vr, size3_t dims, const vec2& position, double delta, int direction, bool inDirField);

	static std::vector<vec2> RK4_steps(const VolumeRAM* vr, size3_t dims, const vec2& position, double delta, int direction, bool inDirField, int steps);
	static std::vector<vec2> RK4_arc(const VolumeRAM* vr, size3_t dims, const vec2& position, double delta, int direction, bool inDirField, double arcLength);
	static std::vector<vec2> RK4_boundary(const VolumeRAM* vr, size3_t dims, const vec2& position, double delta, int direction, bool inDirField, double minX, double  minY, double maxX, double maxY);
	static std::vector<vec2> RK4_zeroes(const VolumeRAM* vr, size3_t dims, const vec2& position, double delta, int direction, bool inDirField);
	static std::vector<vec2> RK4_slow(const VolumeRAM* vr, size3_t dims, const vec2& position, double delta, int direction, bool inDirField, double minSpeed);

	std::vector<vec2> Euler_steps(const VolumeRAM * vr, size3_t dims, const vec2 & position, double delta, int direction, bool inDirField, int steps);

	std::vector<vec2> Euler_arc(const VolumeRAM * vr, size3_t dims, const vec2 & position, double delta, int direction, bool inDirField, double arcLength);

	std::vector<vec2> Euler_boundary(const VolumeRAM * vr, size3_t dims, const vec2 & position, double delta, int direction, bool inDirField, double minX, double minY, double maxX, double maxY);

	std::vector<vec2> Euler_zeroes(const VolumeRAM * vr, size3_t dims, const vec2 & position, double delta, int direction, bool inDirField);

	std::vector<vec2> Euler_slow(const VolumeRAM * vr, size3_t dims, const vec2 & position, double delta, int direction, bool inDirField, double minSpeed);

};

} // namespace
