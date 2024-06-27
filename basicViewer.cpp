#include "Samples.h"
#include "CameraDemo.hpp"
#include "callback.hpp"
#include <QApplication>
#include <QThread>
#include <iostream>
#include <csignal>

// External configuration
extern PSampleConfiguration gSampleConfiguration;
GstElement *senderPipeline = NULL;

GstFlowReturn on_new_sample(GstElement *sink, gpointer data, UINT64 trackid)
{
    GstBuffer *buffer;
    STATUS retStatus = STATUS_SUCCESS;
    BOOL isDroppable, delta;
    GstFlowReturn ret = GST_FLOW_OK;
    GstSample *sample = NULL;
    GstMapInfo info;
    GstSegment *segment;
    GstClockTime buf_pts;
    Frame frame;
    STATUS status;
    PSampleConfiguration pSampleConfiguration = (PSampleConfiguration)data;
    PSampleStreamingSession pSampleStreamingSession = NULL;
    PRtcRtpTransceiver pRtcRtpTransceiver = NULL;
    UINT32 i;
    guint bitrate;

    CHK_ERR(pSampleConfiguration != NULL, STATUS_NULL_ARG, "NULL sample configuration");

    info.data = NULL;
    sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));

    buffer = gst_sample_get_buffer(sample);
    isDroppable = GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_CORRUPTED) || GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DECODE_ONLY) ||
                  (GST_BUFFER_FLAGS(buffer) == GST_BUFFER_FLAG_DISCONT) ||
                  (GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DISCONT) && GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT)) ||
                  // drop if buffer contains header only and has invalid timestamp
                  !GST_BUFFER_PTS_IS_VALID(buffer);

    if (!isDroppable)
    {
        delta = GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT);

        frame.flags = delta ? FRAME_FLAG_NONE : FRAME_FLAG_KEY_FRAME;

        // convert from segment timestamp to running time in live mode.
        segment = gst_sample_get_segment(sample);
        buf_pts = gst_segment_to_running_time(segment, GST_FORMAT_TIME, buffer->pts);
        if (!GST_CLOCK_TIME_IS_VALID(buf_pts))
        {
            DLOGE("[KVS GStreamer Master] Frame contains invalid PTS dropping the frame");
        }

        if (!(gst_buffer_map(buffer, &info, GST_MAP_READ)))
        {
            DLOGE("[KVS GStreamer Master] on_new_sample(): Gst buffer mapping failed");
            goto CleanUp;
        }

        frame.trackId = trackid;
        frame.duration = 0;
        frame.version = FRAME_CURRENT_VERSION;
        frame.size = (UINT32)info.size;
        frame.frameData = (PBYTE)info.data;

        MUTEX_LOCK(pSampleConfiguration->streamingSessionListReadLock);
        for (i = 0; i < pSampleConfiguration->streamingSessionCount; ++i)
        {
            pSampleStreamingSession = pSampleConfiguration->sampleStreamingSessionList[i];
            frame.index = (UINT32)ATOMIC_INCREMENT(&pSampleStreamingSession->frameIndex);

            if (trackid == DEFAULT_AUDIO_TRACK_ID)
            {
                if (pSampleStreamingSession->pSampleConfiguration->enableTwcc && senderPipeline != NULL)
                {
                    GstElement *encoder = gst_bin_get_by_name(GST_BIN(senderPipeline), "sampleAudioEncoder");
                    if (encoder != NULL)
                    {
                        g_object_get(G_OBJECT(encoder), "bitrate", &bitrate, NULL);
                        MUTEX_LOCK(pSampleStreamingSession->twccMetadata.updateLock);
                        pSampleStreamingSession->twccMetadata.currentAudioBitrate = (UINT64)bitrate;
                        if (pSampleStreamingSession->twccMetadata.newAudioBitrate != 0)
                        {
                            bitrate = (guint)(pSampleStreamingSession->twccMetadata.newAudioBitrate);
                            pSampleStreamingSession->twccMetadata.newAudioBitrate = 0;
                            g_object_set(G_OBJECT(encoder), "bitrate", bitrate, NULL);
                        }
                        MUTEX_UNLOCK(pSampleStreamingSession->twccMetadata.updateLock);
                    }
                }
                pRtcRtpTransceiver = pSampleStreamingSession->pAudioRtcRtpTransceiver;
                frame.presentationTs = pSampleStreamingSession->audioTimestamp;
                frame.decodingTs = frame.presentationTs;
                pSampleStreamingSession->audioTimestamp +=
                    SAMPLE_AUDIO_FRAME_DURATION; // assume audio frame size is 20ms, which is default in opusenc
            }
            else
            {
                if (pSampleStreamingSession->pSampleConfiguration->enableTwcc && senderPipeline != NULL)
                {
                    GstElement *encoder = gst_bin_get_by_name(GST_BIN(senderPipeline), "sampleVideoEncoder");
                    if (encoder != NULL)
                    {
                        g_object_get(G_OBJECT(encoder), "bitrate", &bitrate, NULL);
                        MUTEX_LOCK(pSampleStreamingSession->twccMetadata.updateLock);
                        pSampleStreamingSession->twccMetadata.currentVideoBitrate = (UINT64)bitrate;
                        if (pSampleStreamingSession->twccMetadata.newVideoBitrate != 0)
                        {
                            bitrate = (guint)(pSampleStreamingSession->twccMetadata.newVideoBitrate);
                            pSampleStreamingSession->twccMetadata.newVideoBitrate = 0;
                            g_object_set(G_OBJECT(encoder), "bitrate", bitrate, NULL);
                        }
                        MUTEX_UNLOCK(pSampleStreamingSession->twccMetadata.updateLock);
                    }
                }
                pRtcRtpTransceiver = pSampleStreamingSession->pVideoRtcRtpTransceiver;
                frame.presentationTs = pSampleStreamingSession->videoTimestamp;
                frame.decodingTs = frame.presentationTs;
                pSampleStreamingSession->videoTimestamp += SAMPLE_VIDEO_FRAME_DURATION; // assume video fps is 25
            }
            status = writeFrame(pRtcRtpTransceiver, &frame);
            if (status != STATUS_SRTP_NOT_READY_YET && status != STATUS_SUCCESS)
            {
#ifdef VERBOSE
                DLOGE("[KVS GStreamer Master] writeFrame() failed with 0x%08x", status);
#endif
            }
            else if (status == STATUS_SUCCESS && pSampleStreamingSession->firstFrame)
            {
                PROFILE_WITH_START_TIME(pSampleStreamingSession->offerReceiveTime, "Time to first frame");
                pSampleStreamingSession->firstFrame = FALSE;
            }
            else if (status == STATUS_SRTP_NOT_READY_YET)
            {
                DLOGI("[KVS GStreamer Master] SRTP not ready yet, dropping frame");
            }
        }
        MUTEX_UNLOCK(pSampleConfiguration->streamingSessionListReadLock);
    }

CleanUp:

    if (info.data != NULL)
    {
        gst_buffer_unmap(buffer, &info);
    }

    if (sample != NULL)
    {
        gst_sample_unref(sample);
    }

    if (ATOMIC_LOAD_BOOL(&pSampleConfiguration->appTerminateFlag))
    {
        ret = GST_FLOW_EOS;
    }

    return ret;
}

GstFlowReturn on_new_sample_video(GstElement *sink, gpointer data)
{
    return on_new_sample(sink, data, DEFAULT_VIDEO_TRACK_ID);
}

GstFlowReturn on_new_sample_audio(GstElement *sink, gpointer data)
{
    return on_new_sample(sink, data, DEFAULT_AUDIO_TRACK_ID);
}

PVOID sendGstreamerAudioVideo(PVOID args)
{
    STATUS retStatus = STATUS_SUCCESS;
    GstElement *appsinkVideo = NULL, *appsinkAudio = NULL;
    GstBus *bus;
    GstMessage *msg;
    GError *error = NULL;
    PSampleConfiguration pSampleConfiguration = (PSampleConfiguration)args;

    CHK_ERR(pSampleConfiguration != NULL, STATUS_NULL_ARG, "[KVS Gstreamer Master] Streaming session is NULL");

    CHAR rtspPipeLineBuffer[RTSP_PIPELINE_MAX_CHAR_COUNT];

#ifdef _WIN32
    senderPipeline =
        gst_parse_launch("ksvideosrc ! queue ! videoconvert ! video/x-raw,format=NV12,width=1280,height=720,framerate=10/1 ! x264enc "
                         "bframes=0 speed-preset=veryfast bitrate=512 byte-stream=TRUE tune=zerolatency ! "
                         "video/x-h264,stream-format=byte-stream,alignment=au,profile=baseline ! appsink sync=TRUE emit-signals=TRUE "
                         "name=appsink-video autoaudiosrc ! "
                         "queue leaky=2 max-size-buffers=400 ! audioconvert ! audioresample ! opusenc name=sampleAudioEncoder ! "
                         "audio/x-opus,rate=48000,channels=2 ! appsink sync=TRUE emit-signals=TRUE name=appsink-audio",
                         &error);

#elif __linux__
    senderPipeline =
        gst_parse_launch("v4l2src device=/dev/video0 ! queue ! videoconvert ! video/x-raw,format=NV12,width=1280,height=720,framerate=10/1 ! x264enc "
                         "bframes=0 speed-preset=veryfast bitrate=512 byte-stream=TRUE tune=zerolatency ! "
                         "video/x-h264,stream-format=byte-stream,alignment=au,profile=baseline ! appsink sync=TRUE emit-signals=TRUE "
                         "name=appsink-video autoaudiosrc ! "
                         "queue leaky=2 max-size-buffers=400 ! audioconvert ! audioresample ! opusenc name=sampleAudioEncoder ! "
                         "audio/x-opus,rate=48000,channels=2 ! appsink sync=TRUE emit-signals=TRUE name=appsink-audio",
                         &error);
#endif
    CHK_ERR(senderPipeline != NULL, STATUS_NULL_ARG, "[KVS Gstreamer Master] Pipeline is NULL");

    // Retrieve the video appsink element by name from the senderPipeline
    appsinkVideo = gst_bin_get_by_name(GST_BIN(senderPipeline), "appsink-video");

    // Retrieve the audio appsink element by name from the senderPipeline
    appsinkAudio = gst_bin_get_by_name(GST_BIN(senderPipeline), "appsink-audio");

    // Check if neither the video nor audio appsink elements were found
    if (!(appsinkVideo != NULL || appsinkAudio != NULL))
    {
        // Log an error message indicating the appsink elements could not be found
        DLOGE("[KVS GStreamer Master] sendGstreamerAudioVideo(): cant find appsink, operation returned status code: 0x%08x", STATUS_INTERNAL_ERROR);
        // Jump to the cleanup section to free resources and exit the function
        goto CleanUp;
    }

    // If the video appsink element was found
    if (appsinkVideo != NULL)
    {
        // Connect the "new-sample" signal to the on_new_sample_video callback function
        // This will be called whenever a new video sample is available
        g_signal_connect(appsinkVideo, "new-sample", G_CALLBACK(on_new_sample_video), (gpointer)pSampleConfiguration);
    }

    // If the audio appsink element was found
    if (appsinkAudio != NULL)
    {
        // Connect the "new-sample" signal to the on_new_sample_audio callback function
        // This will be called whenever a new audio sample is available
        g_signal_connect(appsinkAudio, "new-sample", G_CALLBACK(on_new_sample_audio), (gpointer)pSampleConfiguration);
    }

    // Set the state of the senderPipeline to PLAYING to start processing
    gst_element_set_state(senderPipeline, GST_STATE_PLAYING);

    // Retrieve the bus from the senderPipeline to listen for messages
    bus = gst_element_get_bus(senderPipeline);

    // Block and wait for an error or EOS (End of Stream) message from the pipeline
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    // Free resources if a message was received
    if (msg != NULL)
    {
        gst_message_unref(msg);
    }

    // Free the bus object
    if (bus != NULL)
    {
        gst_object_unref(bus);
    }

    // Set the state of the senderPipeline to NULL to stop it and free the pipeline object
    if (senderPipeline != NULL)
    {
        gst_element_set_state(senderPipeline, GST_STATE_NULL);
        gst_object_unref(senderPipeline);
    }

    // Free the audio appsink element if it was retrieved
    if (appsinkAudio != NULL)
    {
        gst_object_unref(appsinkAudio);
    }

    // Free the video appsink element if it was retrieved
    if (appsinkVideo != NULL)
    {
        gst_object_unref(appsinkVideo);
    }

CleanUp:

    if (error != NULL)
    {
        DLOGE("[KVS GStreamer Master] %s", error->message);
        g_clear_error(&error);
    }

    return (PVOID)(ULONG_PTR)retStatus;
}
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
    pSampleConfiguration->mediaType = SAMPLE_STREAMING_AUDIO_VIDEO;
    pSampleConfiguration->receiveAudioVideoSource = receiveGstreamerAudioVideo;
    pSampleConfiguration->audioCodec = audioCodec;
    pSampleConfiguration->videoCodec = videoCodec;
    pSampleConfiguration->videoSource = sendGstreamerAudioVideo;
    pSampleConfiguration->srcType = DEVICE_SOURCE;
    DLOGI("[KVS Gstreamer Viewer] Streaming audio and video");
    gst_init(&argc, &argv);
    // Initialize KVS WebRTC. This must be done before anything else, and must only be done once.
    CHK_STATUS(initKvsWebRtc());
    DLOGI("[KVS Gstreamer Viewer] KVS WebRTC initialization completed successfully");

    SPRINTF(clientId, "%s_%u", SAMPLE_VIEWER_CLIENT_ID, RAND() % MAX_UINT32); // results in clientId being ConsumerViewer_random
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

    if (!pSampleConfiguration->trickleIce) // quicker connection than regular ice
    {
        DLOGI("[KVS Gstreamer Viewer] Non trickle ice. Wait for Candidate collection to complete");
        MUTEX_LOCK(pSampleConfiguration->sampleConfigurationObjLock);
        locked = TRUE;

        while (!ATOMIC_LOAD_BOOL(&pSampleStreamingSession->candidateGatheringDone))
        {
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

    if (buffLen >= SIZEOF(message.payload))
    {
        DLOGE("[KVS Gstreamer Viewer] serializeSessionDescriptionInit(): operation returned status code: 0x%08x ", STATUS_INVALID_OPERATION);
        retStatus = STATUS_INVALID_OPERATION;
        goto CleanUp;
    }

    CHK_STATUS(serializeSessionDescriptionInit(&offerSessionDescriptionInit, message.payload, &buffLen));

    message.version = SIGNALING_MESSAGE_CURRENT_VERSION;
    message.messageType = SIGNALING_MESSAGE_TYPE_OFFER;
    STRCPY(message.peerClientId, SAMPLE_MASTER_CLIENT_ID); // the master id its connecting to
    message.payloadLen = (buffLen / SIZEOF(CHAR)) - 1;
    message.correlationId[0] = '\0';
    DLOGI("[KVS Gstreamer Viewer] sending offer message now");
    CHK_STATUS(signalingClientSendMessageSync(pSampleConfiguration->signalingClientHandle, &message)); // sending the offer message
    DLOGI("[KVS Gstreamer Viewer] sent offer message");
    // Block until interrupted
    while (!ATOMIC_LOAD_BOOL(&pSampleConfiguration->interrupted) && !ATOMIC_LOAD_BOOL(&pSampleStreamingSession->terminateFlag))
    {
        THREAD_SLEEP(HUNDREDS_OF_NANOS_IN_A_SECOND);
    }

CleanUp:

    if (retStatus != STATUS_SUCCESS)
    {
        DLOGE("[KVS Gstreamer Viewer] Terminated with status code 0x%08x", retStatus);
    }

    DLOGI("[KVS Gstreamer Viewer] Cleaning up....");

    if (locked)
    {
        MUTEX_UNLOCK(pSampleConfiguration->sampleConfigurationObjLock);
    }

    if (pSampleConfiguration->enableFileLogging)
    {
        freeFileLogger();
    }
    if (pSampleConfiguration != NULL)
    {
        retStatus = freeSignalingClient(&pSampleConfiguration->signalingClientHandle);
        if (retStatus != STATUS_SUCCESS)
        {
            DLOGE("[KVS Gstreamer Viewer] freeSignalingClient(): operation returned status code: 0x%08x ", retStatus);
        }

        retStatus = freeSampleConfiguration(&pSampleConfiguration);
        if (retStatus != STATUS_SUCCESS)
        {
            DLOGE("[KVS Gstreamer Viewer] freeSampleConfiguration(): operation returned status code: 0x%08x ", retStatus);
        }
    }
    DLOGI("[KVS Gstreamer Viewer] Cleanup done");

    RESET_INSTRUMENTED_ALLOCATORS();

    })->start();

    // Start the Qt event loop in the main thread
    return app.exec();
}
