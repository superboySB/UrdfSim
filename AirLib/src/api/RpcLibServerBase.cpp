// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

//in header only mode, control library is not available
#ifndef AIRLIB_HEADER_ONLY
//RPC code requires C++14. If build system like Unreal doesn't support it then use compiled binaries
#ifndef AIRLIB_NO_RPC
//if using Unreal Build system then include precompiled header file first

#include "api/RpcLibServerBase.hpp"


#include "common/Common.hpp"
STRICT_MODE_OFF

#ifndef RPCLIB_MSGPACK
#define RPCLIB_MSGPACK clmdep_msgpack
#endif // !RPCLIB_MSGPACK
#include "common/common_utils/MinWinDefines.hpp"
#undef NOUSER

#include "common/common_utils/WindowsApisCommonPre.hpp"
#undef FLOAT
#undef check
#include "rpc/server.h"
//TODO: HACK: UE4 defines macro with stupid names like "check" that conflicts with msgpack library
#ifndef check
#define check(expr) (static_cast<void>((expr)))
#endif
#include "common/common_utils/WindowsApisCommonPost.hpp"

#include "api/RpcLibAdapatorsBase.hpp"

STRICT_MODE_ON


namespace msr { namespace airlib {

struct RpcLibServerBase::impl {
    impl(string server_address, uint16_t port)
        : server(server_address, port)
    {}

    impl(uint16_t port)
        : server(port)
    {}

    ~impl() {
    }

    rpc::server server;
};

typedef msr::airlib_rpclib::RpcLibAdapatorsBase RpcLibAdapatorsBase;

RpcLibServerBase::RpcLibServerBase(ApiProvider* api_provider, const std::string& server_address, uint16_t port)
    : api_provider_(api_provider)
{
    if (server_address == "")
        pimpl_.reset(new impl(port));
    else
        pimpl_.reset(new impl(server_address, port));
    pimpl_->server.bind("ping", [&]() -> bool { return true; });
    pimpl_->server.bind("getServerVersion", []() -> int {
        return 1;
    });
    pimpl_->server.bind("getMinRequiredClientVersion", []() -> int {
        return 1;
    });
       
    pimpl_->server.bind("simPause", [&](bool is_paused) -> void { 
        getWorldSimApi()->pause(is_paused); 
    });
    pimpl_->server.bind("simIsPaused", [&]() -> bool { 
        return getWorldSimApi()->isPaused(); 
    });
    pimpl_->server.bind("simContinueForTime", [&](double seconds) -> void { 
        getWorldSimApi()->continueForTime(seconds); 
    });

    pimpl_->server.bind("enableApiControl", [&](bool is_enabled, const std::string& vehicle_name) -> void { 
        getVehicleApi(vehicle_name)->enableApiControl(is_enabled);
    });
    pimpl_->server.bind("isApiControlEnabled", [&](const std::string& vehicle_name) -> bool { 
        return getVehicleApi(vehicle_name)->isApiControlEnabled();
    });
    pimpl_->server.bind("armDisarm", [&](bool arm, const std::string& vehicle_name) -> bool { 
        return getVehicleApi(vehicle_name)->armDisarm(arm);
    });

    pimpl_->server.bind("simGetImages", [&](const std::vector<RpcLibAdapatorsBase::ImageRequest>& request_adapter, const std::string& vehicle_name) -> 
        vector<RpcLibAdapatorsBase::ImageResponse> {
            const auto& response = getVehicleSimApi(vehicle_name)->getImages(RpcLibAdapatorsBase::ImageRequest::to(request_adapter));
            return RpcLibAdapatorsBase::ImageResponse::from(response);
    });

    pimpl_->server.bind("simGetImage", [&](const std::string& camera_name, ImageCaptureBase::ImageType type, const std::string& vehicle_name) -> vector<uint8_t> {
        auto result = getVehicleSimApi(vehicle_name)->getImage(camera_name, type);
        if (result.size() == 0) {
            // rpclib has a bug with serializing empty vectors, so we return a 1 byte vector instead.
            result.push_back(0);
        }
        return result;
    });

    pimpl_->server.bind("simSetCameraPose", [&](const RpcLibAdapatorsBase::CameraPose camera_pose, const std::string& vehicle_name) -> void {
        getVehicleSimApi(vehicle_name)->setCameraPose(camera_pose.to());
    });

    pimpl_->server.bind("readSensors", [&](const std::string& vehicle_name) ->
        RpcLibAdapatorsBase::SensorReadings {
        RpcLibAdapatorsBase::SensorReadings sr(getVehicleApi(vehicle_name)->getSensors());
        return sr;
    });

    pimpl_->server.bind("simRayCast", [&](const RpcLibAdapatorsBase::RayCastRequest request, const std::string& vehicle_name) -> RpcLibAdapatorsBase::RayCastResponse {
        return RpcLibAdapatorsBase::RayCastResponse(getVehicleSimApi(vehicle_name)->rayCast(request.to()));
    });

    pimpl_->server.
        bind("simSetVehiclePose", [&](const RpcLibAdapatorsBase::Pose &pose, bool ignore_collision, const std::string& vehicle_name) -> void {
        getVehicleSimApi(vehicle_name)->setPose(pose.to(), ignore_collision);
    });
    pimpl_->server.bind("simGetVehiclePose", [&](const std::string& vehicle_name) -> RpcLibAdapatorsBase::Pose {
        const auto& pose = getVehicleSimApi(vehicle_name)->getPose();
        return RpcLibAdapatorsBase::Pose(pose);
    });

    pimpl_->server.
        bind("simXyzToGeoPoints", [&](const std::vector<RpcLibAdapatorsBase::Vector3r> &xyz_points, const std::string vehicle_name) -> std::vector<RpcLibAdapatorsBase::GeoPoint> {

        //TODO: this is a lot of copying...
        std::vector<msr::airlib::Vector3r> xyz_points_converted;
        xyz_points_converted.reserve(xyz_points.size());
        for (const auto &point : xyz_points)
        {
            xyz_points_converted.emplace_back(point.to());
        }

        std::vector<msr::airlib::GeoPoint> result_points = getVehicleSimApi(vehicle_name)->xyzToGeoPoints(xyz_points_converted);

        std::vector<RpcLibAdapatorsBase::GeoPoint> output;
        output.reserve(result_points.size());
        for (const auto &point : result_points)
        {
            output.emplace_back(RpcLibAdapatorsBase::GeoPoint(point));
        }

        return output;
    });

    pimpl_->server.
        bind("simSetSegmentationObjectID", [&](const std::string& mesh_name, int object_id, bool is_name_regex) -> bool {
        return getWorldSimApi()->setSegmentationObjectID(mesh_name, object_id, is_name_regex);
    });
    pimpl_->server.
        bind("simGetSegmentationObjectID", [&](const std::string& mesh_name) -> int {
        return getWorldSimApi()->getSegmentationObjectID(mesh_name);
    });

    pimpl_->server
        .bind("simSetDrawableShapes", [&](const RpcLibAdapatorsBase::DrawableShapeRequest request, const std::string& vehicle_name) -> void {
        auto converted = request.to();
        getVehicleSimApi(vehicle_name)->setDrawShapes(converted.shapes, converted.persist_unmentioned);
    });

    pimpl_->server.bind("reset", [&]() -> void {
        auto* sim_world_api = getWorldSimApi();
        if (sim_world_api)
            sim_world_api->reset();
        else
            getVehicleApi("")->reset();
    });

    pimpl_->server.bind("simPrintLogMessage", [&](const std::string& message, const std::string& message_param, unsigned char severity) -> void {
        getWorldSimApi()->printLogMessage(message, message_param, severity);
    });

    pimpl_->server.bind("getHomeGeoPoint", [&](const std::string& vehicle_name) -> RpcLibAdapatorsBase::GeoPoint {
        const auto& geo_point = getVehicleApi(vehicle_name)->getHomeGeoPoint();
        return RpcLibAdapatorsBase::GeoPoint(geo_point);
    });

    pimpl_->server.bind("getLidarData", [&](const std::string& lidar_name, const std::string& vehicle_name) -> RpcLibAdapatorsBase::LidarData {
        const auto& lidar_data = getVehicleApi(vehicle_name)->getLidarData(lidar_name);
        return RpcLibAdapatorsBase::LidarData(lidar_data);
    });

    pimpl_->server.bind("simGetCameraInfo", [&](const std::string& camera_name, const std::string& vehicle_name) -> RpcLibAdapatorsBase::CameraInfo {
        const auto& camera_info = getVehicleSimApi(vehicle_name)->getCameraInfo(camera_name);
        return RpcLibAdapatorsBase::CameraInfo(camera_info);
    });

    pimpl_->server.bind("simSetCameraOrientation", [&](const std::string& camera_name, const RpcLibAdapatorsBase::Quaternionr& orientation, 
        const std::string& vehicle_name) -> void {
        getVehicleSimApi(vehicle_name)->setCameraOrientation(camera_name, orientation.to());
    });

    pimpl_->server.bind("simGetCollisionInfo", [&](const std::string& vehicle_name) -> RpcLibAdapatorsBase::CollisionInfo {
        const auto& collision_info = getVehicleSimApi(vehicle_name)->getCollisionInfo(); 
        return RpcLibAdapatorsBase::CollisionInfo(collision_info);
    });

    pimpl_->server.bind("simGetObjectPose", [&](const std::string& object_name) -> RpcLibAdapatorsBase::Pose {
        const auto& pose = getWorldSimApi()->getObjectPose(object_name); 
        return RpcLibAdapatorsBase::Pose(pose);
    });
    pimpl_->server.bind("simSetObjectPose", [&](const std::string& object_name, const RpcLibAdapatorsBase::Pose& pose, bool teleport) -> bool {
        return getWorldSimApi()->setObjectPose(object_name, pose.to(), teleport);
    });
    pimpl_->server.bind("simSpawnStaticMeshObject", [&](const std::string& object_class_name, const std::string& object_name, const RpcLibAdapatorsBase::Pose& pose) -> bool {
        return getWorldSimApi()->spawnStaticMeshObject(object_class_name, object_name, pose.to());
    });
    pimpl_->server.bind("simDeleteObject", [&](const std::string& object_name) -> bool {
        return getWorldSimApi()->deleteObject(object_name);
    });

    pimpl_->server.bind("simGetGroundTruthKinematics", [&](const std::string& vehicle_name) -> RpcLibAdapatorsBase::KinematicsState {
        const Kinematics::State& result = *getVehicleSimApi(vehicle_name)->getGroundTruthKinematics();
        return RpcLibAdapatorsBase::KinematicsState(result);
    });

    pimpl_->server.bind("simGetGroundTruthEnvironment", [&](const std::string& vehicle_name) -> RpcLibAdapatorsBase::EnvironmentState {
        const Environment::State& result = (*getVehicleSimApi(vehicle_name)->getGroundTruthEnvironment()).getState();
        return RpcLibAdapatorsBase::EnvironmentState(result);
    });

    pimpl_->server.bind("cancelLastTask", [&](const std::string& vehicle_name) -> void {
        getVehicleApi(vehicle_name)->cancelLastTask();
    });

    //----------- APIs to control ACharacter in scene ----------/
    pimpl_->server.bind("simCharSetFaceExpression", [&](const std::string& expression_name, float value, const std::string& character_name) -> void {
        getWorldSimApi()->charSetFaceExpression(expression_name, value, character_name);
    });
    pimpl_->server.bind("simCharGetFaceExpression", [&](const std::string& expression_name, const std::string& character_name) -> float {
        return getWorldSimApi()->charGetFaceExpression(expression_name, character_name);
    });
    pimpl_->server.bind("simCharGetAvailableFaceExpressions", [&]() -> std::vector<std::string> {
        return getWorldSimApi()->charGetAvailableFaceExpressions();
    });
    pimpl_->server.bind("simCharSetSkinDarkness", [&](float value, const std::string& character_name) -> void {
        getWorldSimApi()->charSetSkinDarkness(value, character_name);
    });
    pimpl_->server.bind("simCharGetSkinDarkness", [&](const std::string& character_name) -> float {
        return getWorldSimApi()->charGetSkinDarkness(character_name);
    });
    pimpl_->server.bind("simCharSetSkinAgeing", [&](float value, const std::string& character_name) -> void {
        getWorldSimApi()->charSetSkinAgeing(value, character_name);
    });
    pimpl_->server.bind("simCharGetSkinAgeing", [&](const std::string& character_name) -> float {
        return getWorldSimApi()->charGetSkinAgeing(character_name);
    });
    pimpl_->server.bind("simCharSetHeadRotation", [&](const RpcLibAdapatorsBase::Quaternionr& q, const std::string& character_name) -> void {
        getWorldSimApi()->charSetHeadRotation(q.to(), character_name);
    });
    pimpl_->server.bind("simCharGetHeadRotation", [&](const std::string& character_name) -> RpcLibAdapatorsBase::Quaternionr {
        msr::airlib::Quaternionr q = getWorldSimApi()->charGetHeadRotation(character_name);
        return RpcLibAdapatorsBase::Quaternionr(q);
    });
    pimpl_->server.bind("simCharSetBonePose", [&](const std::string& bone_name, const RpcLibAdapatorsBase::Pose& pose, const std::string& character_name) -> void {
        getWorldSimApi()->charSetBonePose(bone_name, pose.to(), character_name);
    });
    pimpl_->server.bind("simCharGetBonePose", [&](const std::string& bone_name, const std::string& character_name) -> RpcLibAdapatorsBase::Pose {
        msr::airlib::Pose pose = getWorldSimApi()->charGetBonePose(bone_name, character_name);
        return RpcLibAdapatorsBase::Pose(pose);
    });
    pimpl_->server.bind("simCharResetBonePose", [&](const std::string& bone_name, const std::string& character_name) -> void {
        getWorldSimApi()->charResetBonePose(bone_name, character_name);
    });
    pimpl_->server.bind("simCharSetFacePreset", [&](const std::string& preset_name, float value, const std::string& character_name) -> void {
        getWorldSimApi()->charSetFacePreset(preset_name, value, character_name);
    });
    pimpl_->server.bind("simSetFacePresets", [&](const std::unordered_map<std::string, float>& presets, const std::string& character_name) -> void {
        getWorldSimApi()->charSetFacePresets(presets, character_name);
    });
    pimpl_->server.bind("simSetBonePoses", [&](const std::unordered_map<std::string, RpcLibAdapatorsBase::Pose>& poses, const std::string& character_name) -> void {
        std::unordered_map<std::string, msr::airlib::Pose> r;
        for (const auto& p : poses)
            r[p.first] = p.second.to();

        getWorldSimApi()->charSetBonePoses(r, character_name);
    });
    pimpl_->server.bind("simGetBonePoses", [&](const std::vector<std::string>& bone_names, const std::string& character_name) 
        -> std::unordered_map<std::string, RpcLibAdapatorsBase::Pose> {

        std::unordered_map<std::string, msr::airlib::Pose> poses = getWorldSimApi()->charGetBonePoses(bone_names, character_name);
        std::unordered_map<std::string, RpcLibAdapatorsBase::Pose> r;
        for (const auto& p : poses)
            r[p.first] = RpcLibAdapatorsBase::Pose(p.second);

        return r;
    });

    //if we don't suppress then server will bomb out for exceptions raised by any method
    pimpl_->server.suppress_exceptions(true);
}

//required for pimpl
RpcLibServerBase::~RpcLibServerBase()
{
    stop();
}

void RpcLibServerBase::start(bool block)
{
    if (block)
        pimpl_->server.run();
    else
        pimpl_->server.async_run(4);   //4 threads
}

void RpcLibServerBase::stop()
{
    pimpl_->server.stop();
}

void* RpcLibServerBase::getServer() const
{
    return &pimpl_->server;
}

}} //namespace
#endif
#endif
