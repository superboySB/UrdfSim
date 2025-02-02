// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef msr_airlib_DistanceBase_hpp
#define msr_airlib_DistanceBase_hpp


#include "sensors/SensorBase.hpp"


namespace msr { namespace airlib {

class DistanceBase  : public SensorBase {
public:
    DistanceBase(const std::string& sensor_name = "", const std::string& attach_link_name = "")
        : SensorBase(sensor_name, attach_link_name)
    {}

public: //types
    struct Output { //same fields as ROS message
        real_T distance;    //meters
        real_T min_distance;//m
        real_T max_distance;//m
        Pose relative_pose;
    };


public:
    virtual void reportState(StateReporter& reporter) override
    {
        //call base
        UpdatableObject::reportState(reporter);

        reporter.writeValue("Dist-Curr", output_.distance);
    }

    virtual const std::map<std::string, double> read() const override
    {
        std::map<std::string, double> values;

        values["Dist-Curr"] = output_.distance;
        values["Max-Distance"] = output_.max_distance;
        values["Min-Distance"] = output_.min_distance;

        return values;
    }

    const Output& getOutput() const
    {
        return output_;
    }

protected:
    void setOutput(const Output& output)
    {
        output_ = output;
    }


private:
    Output output_;
};


}} //namespace
#endif
