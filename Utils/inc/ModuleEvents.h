/**
 * @file ModuleEvents.h
 * @author Matthew Da Silva (matthew.dasilva@wosler.ca)
 * @brief 
 * @version 0.1
 * @date 2023-02-24
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#pragma once

namespace wosler {
    namespace utilities {
        enum Events {
            INPUT_DEVICE_QueryEvents = (int)Module::INPUT_DEVICE*100,
            INPUT_DEVICE_GetCurrentPosition,
            INPUT_DEVICE_SetDesiredPosition,
            INPUT_DEVICE_GetCurrentForce,
            INPUT_DEVICE_SetDesiredForce,
            INPUT_DEVICE_SetTransformation,
            INPUT_DEVICE_DragAndDrop,
            INPUT_DEVICE_GetDeviceInfo,
            INPUT_DEVICE_GetTraversalBounds,
            INPUT_DEVICE_StartCalibration,
            MOTION_CONTROL_QueryEvents = (int)Module::MOTION_CONTROL*100,
            MOTION_CONTROL_SetOffset,
            MOTION_CONTROL_FlipControl,
            MOTION_CONTROL_ToggleHaptics,
            MOTION_CONTROL_SetScale,
            MOTION_CONTROL_DragAndDropStatus,
            SONOLINK_QueryEvents = (int)Module::SONOLINK_INTERFACE*100,
            SONOLINK_SendMotion,
            SONOLINK_SendUltrasound,
            SONOLINK_SendError,
            SONOLINK_SendAction,
            SONOLINK_PostDesiredPosForce,
            SONOLINK_PostMaxForce,
            SONOLINK_Login,
            SONOLINK_SessionConnect,
            SONOLINK_HandshakeStatus,
            SONOSTATION_GUI_ShowPrompt = (int)Module::SONOSTATION_GUI*100,
            ULTRASOUND_KeyMouseControlStatus = (int)Module::ULTRASOUND_INPUT*100,
        };
    }
}