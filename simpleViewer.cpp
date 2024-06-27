#include "Samples.h"
#include "CameraDemo.hpp"
#include "callback.hpp"
#include <QApplication>
#include <QThread>
#include <iostream>
#include <csignal>

// External configuration
extern PSampleConfiguration gSampleConfiguration;

// Main function
INT32 main(INT32 argc, CHAR *argv[]) {
    // Initialize Qt application
    QApplication app(argc, argv);
    gst_init(&argc, &argv);

    // Create the CameraDemo instance
    wosler::demo::CameraDemo demo;
    registerSetupPipelineCallback([&demo](std::shared_ptr<GstElement> &pipeline) {
        demo.SetupPipeline(pipeline);
    });
    demo.show();

    // Perform WebRTC and other tasks in separate thread
    QThread::create([argc, argv]() {
        STATUS retStatus = STATUS_SUCCESS;
        RtcSessionDescriptionInit offerSessionDescriptionInit;
        UINT32 buffLen = 0;
        SignalingMessage message;
        PSampleConfiguration pSampleConfiguration = NULL;
        PSampleStreamingSession pSampleStreamingSession = NULL;
        RTC_CODEC audioCodec = RTC_CODEC_OPUS;
        RTC_CODEC videoCodec = RTC_CODEC_H264_PROFILE_42E01F_LEVEL_ASYMMETRY_ALLOWED_PACKETIZATION_MODE;
        BOOL locked = FALSE;
        PCHAR pChannelName;
        CHAR clientId[256];

        SET_INSTRUMENTED_ALLOCATORS();
        UINT32 logLevel = setLogLevel();

#ifndef _WIN32
        signal(SIGINT, sigintHandler);
#endif

        pChannelName = argc > 1 ? argv[1] : SAMPLE_CHANNEL_NAME;

        CHK_STATUS(createSampleConfiguration(pChannelName, SIGNALING_CHANNEL_ROLE_TYPE_VIEWER, TRUE, TRUE, logLevel, &pSampleConfiguration));
        gSampleConfiguration = pSampleConfiguration; // Set the global configuration
        pSampleConfiguration->mediaType = SAMPLE_STREAMING_AUDIO_VIDEO;
        pSampleConfiguration->receiveAudioVideoSource = receiveGstreamerAudioVideo;
        pSampleConfiguration->audioCodec = audioCodec;
        pSampleConfiguration->videoCodec = videoCodec;
        DLOGI("[KVS Gstreamer Viewer] Streaming audio and video");

        // Initialize KVS WebRTC
        CHK_STATUS(initKvsWebRtc());
        DLOGI("[KVS Gstreamer Viewer] KVS WebRTC initialization completed successfully");

        SPRINTF(clientId, "%s_%u", SAMPLE_VIEWER_CLIENT_ID, RAND() % MAX_UINT32);
        CHK_STATUS(initSignaling(pSampleConfiguration, clientId));
        DLOGI("[KVS Gstreamer Viewer] Signaling client connection established");

        // Initialize streaming session
        MUTEX_LOCK(pSampleConfiguration->sampleConfigurationObjLock);
        locked = TRUE;
        CHK_STATUS(createSampleStreamingSession(pSampleConfiguration, NULL, FALSE, &pSampleStreamingSession));
        DLOGI("[KVS Gstreamer Viewer] Creating streaming session...completed");
        pSampleConfiguration->sampleStreamingSessionList[pSampleConfiguration->streamingSessionCount++] = pSampleStreamingSession;

        MUTEX_UNLOCK(pSampleConfiguration->sampleConfigurationObjLock);
        locked = FALSE;

        MEMSET(&offerSessionDescriptionInit, 0x00, SIZEOF(RtcSessionDescriptionInit));

        offerSessionDescriptionInit.useTrickleIce = pSampleStreamingSession->remoteCanTrickleIce;
        CHK_STATUS(setLocalDescription(pSampleStreamingSession->pPeerConnection, &offerSessionDescriptionInit));
        DLOGI("[KVS Gstreamer Viewer] Completed setting local description");

        if (!pSampleConfiguration->trickleIce) {
            DLOGI("[KVS Gstreamer Viewer] Non trickle ice. Wait for Candidate collection to complete");
            MUTEX_LOCK(pSampleConfiguration->sampleConfigurationObjLock);
            locked = TRUE;

            while (!ATOMIC_LOAD_BOOL(&pSampleStreamingSession->candidateGatheringDone)) {
                CHK_WARN(!ATOMIC_LOAD_BOOL(&pSampleStreamingSession->terminateFlag), STATUS_OPERATION_TIMED_OUT,
                         "application terminated and candidate gathering still not done");
                CVAR_WAIT(pSampleConfiguration->cvar, pSampleConfiguration->sampleConfigurationObjLock, 5 * HUNDREDS_OF_NANOS_IN_A_SECOND);
            }

            MUTEX_UNLOCK(pSampleConfiguration->sampleConfigurationObjLock);
            locked = FALSE;

            DLOGI("[KVS Gstreamer Viewer] Candidate collection completed");
        }

        CHK_STATUS(createOffer(pSampleStreamingSession->pPeerConnection, &offerSessionDescriptionInit));
        DLOGI("[KVS Gstreamer Viewer] Offer creation successful");

        DLOGI("[KVS Gstreamer Viewer] Generating JSON of session description....");
        CHK_STATUS(serializeSessionDescriptionInit(&offerSessionDescriptionInit, NULL, &buffLen));

        if (buffLen >= SIZEOF(message.payload)) {
            DLOGE("[KVS Gstreamer Viewer] serializeSessionDescriptionInit(): operation returned status code: 0x%08x ", STATUS_INVALID_OPERATION);
            retStatus = STATUS_INVALID_OPERATION;
            goto CleanUp;
        }

        CHK_STATUS(serializeSessionDescriptionInit(&offerSessionDescriptionInit, message.payload, &buffLen));

        message.version = SIGNALING_MESSAGE_CURRENT_VERSION;
        message.messageType = SIGNALING_MESSAGE_TYPE_OFFER;
        STRCPY(message.peerClientId, SAMPLE_MASTER_CLIENT_ID);
        message.payloadLen = (buffLen / SIZEOF(CHAR)) - 1;
        message.correlationId[0] = '\0';

        CHK_STATUS(signalingClientSendMessageSync(pSampleConfiguration->signalingClientHandle, &message));

        // Block until interrupted
        while (!ATOMIC_LOAD_BOOL(&pSampleConfiguration->interrupted) && !ATOMIC_LOAD_BOOL(&pSampleStreamingSession->terminateFlag)) {
            THREAD_SLEEP(HUNDREDS_OF_NANOS_IN_A_SECOND);
        }

    CleanUp:

        if (retStatus != STATUS_SUCCESS) {
            DLOGE("[KVS Gstreamer Viewer] Terminated with status code 0x%08x", retStatus);
        }

        DLOGI("[KVS Gstreamer Viewer] Cleaning up....");

        if (locked) {
            MUTEX_UNLOCK(pSampleConfiguration->sampleConfigurationObjLock);
        }

        if (pSampleConfiguration->enableFileLogging) {
            freeFileLogger();
        }
        if (pSampleConfiguration != NULL) {
            retStatus = freeSignalingClient(&pSampleConfiguration->signalingClientHandle);
            if (retStatus != STATUS_SUCCESS) {
                DLOGE("[KVS Gstreamer Viewer] freeSignalingClient(): operation returned status code: 0x%08x ", retStatus);
            }

            retStatus = freeSampleConfiguration(&pSampleConfiguration);
            if (retStatus != STATUS_SUCCESS) {
                DLOGE("[KVS Gstreamer Viewer] freeSampleConfiguration(): operation returned status code: 0x%08x ", retStatus);
            }
        }
        DLOGI("[KVS Gstreamer Viewer] Cleanup done");

        RESET_INSTRUMENTED_ALLOCATORS();
    })->start();

    // Start the Qt event loop in the main thread
    return app.exec();
}
