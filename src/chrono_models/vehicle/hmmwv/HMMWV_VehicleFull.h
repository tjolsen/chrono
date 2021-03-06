// =============================================================================
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2014 projectchrono.org
// All right reserved.
//
// Use of this source code is governed by a BSD-style license that can be found
// in the LICENSE file at the top level of the distribution and at
// http://projectchrono.org/license-chrono.txt.
//
// =============================================================================
// Authors: Radu Serban, Justin Madsen, Daniel Melanz
// =============================================================================
//
// HMMWV full vehicle model...
//
// =============================================================================

#ifndef HMMWV_VEHICLE_FULL_H
#define HMMWV_VEHICLE_FULL_H

#include "chrono_models/ChApiModels.h"
#include "chrono_models/vehicle/hmmwv/HMMWV_Vehicle.h"
#include "chrono_models/vehicle/hmmwv/HMMWV_Chassis.h"
#include "chrono_models/vehicle/hmmwv/HMMWV_BrakeSimple.h"
#include "chrono_models/vehicle/hmmwv/HMMWV_DoubleWishbone.h"
#include "chrono_models/vehicle/hmmwv/HMMWV_Driveline2WD.h"
#include "chrono_models/vehicle/hmmwv/HMMWV_Driveline4WD.h"
#include "chrono_models/vehicle/hmmwv/HMMWV_SimpleDriveline.h"
#include "chrono_models/vehicle/hmmwv/HMMWV_PitmanArm.h"
#include "chrono_models/vehicle/hmmwv/HMMWV_Wheel.h"

namespace chrono {
namespace vehicle {
namespace hmmwv {

class CH_MODELS_API HMMWV_VehicleFull : public HMMWV_Vehicle {
  public:
    HMMWV_VehicleFull(const bool fixed = false,
                      DrivelineType driveType = DrivelineType::AWD,
                      ChMaterialSurfaceBase::ContactMethod contactMethod = ChMaterialSurfaceBase::DVI);

    HMMWV_VehicleFull(ChSystem* system, const bool fixed = false, DrivelineType driveType = DrivelineType::AWD);

    ~HMMWV_VehicleFull();

    double GetSpringForce(const WheelID& wheel_id) const;
    double GetSpringLength(const WheelID& wheel_id) const;
    double GetSpringDeformation(const WheelID& wheel_id) const;

    double GetShockForce(const WheelID& wheel_id) const;
    double GetShockLength(const WheelID& wheel_id) const;
    double GetShockVelocity(const WheelID& wheel_id) const;

    virtual void Initialize(const ChCoordsys<>& chassisPos, double chassisFwdVel = 0) override;

    // Log debugging information
    void LogHardpointLocations();  /// suspension hardpoints at design
    void DebugLog(int what);       /// shock forces and lengths, constraints, etc.

  private:
    void Create(bool fixed);
};

}  // end namespace hmmwv
}  // end namespace vehicle
}  // end namespace chrono

#endif
