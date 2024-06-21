#include "Samples.h"
#include <opencv2/opencv.hpp>
#include <gst/gst.h>
#include <gst/app/app.h>
#include <gst/app/gstappsink.h>
#include "callback.hpp"
static UINT64 presentationTsIncrement = 0;
static BOOL eos = FALSE;

static CallbackFn setupPipelineCallback = nullptr;
void registerSetupPipelineCallback(CallbackFn fn) {
    setupPipelineCallback = fn;
}

// This function is a callback for the transceiver for every single video frame it receives
// It writes these frames to a buffer and pushes it to the `appsrcVideo` element of the
// GStreamer pipeline created in `receiveGstreamerAudioVideo`. Any logic to modify / discard the frames would go here
VOID onGstVideoFrameReady(UINT64 customData, PFrame pFrame)
{
    STATUS retStatus = STATUS_SUCCESS;
    GstFlowReturn ret;
    GstBuffer *buffer;
    GstElement *appsrcVideo = (GstElement *)customData;

    CHK_ERR(appsrcVideo != NULL, STATUS_NULL_ARG, "appsrcVideo is null");
    CHK_ERR(pFrame != NULL, STATUS_NULL_ARG, "Video frame is null");

    if (!eos)
    {
        buffer = gst_buffer_new_allocate(NULL, pFrame->size, NULL);
        CHK_ERR(buffer != NULL, STATUS_NULL_ARG, "Buffer allocation failed");

        DLOGV("Video frame size: %d, presentationTs: %llu", pFrame->size, presentationTsIncrement);

        GST_BUFFER_DTS(buffer) = presentationTsIncrement;
        GST_BUFFER_PTS(buffer) = presentationTsIncrement;
        GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale(1, GST_SECOND, DEFAULT_FPS_VALUE);
        presentationTsIncrement += gst_util_uint64_scale(1, GST_SECOND, DEFAULT_FPS_VALUE);

        if (gst_buffer_fill(buffer, 0, pFrame->frameData, pFrame->size) != pFrame->size)
        {
            DLOGE("Buffer fill did not complete correctly");
            gst_buffer_unref(buffer);
            return;
        }
        g_signal_emit_by_name(appsrcVideo, "push-buffer", buffer, &ret);
        if (ret != GST_FLOW_OK)
        {
            DLOGE("Error pushing buffer: %s", gst_flow_get_name(ret));
        }
        gst_buffer_unref(buffer);
    }

CleanUp:
    return;
}

// This function is a callback for the transceiver for every single audio frame it receives
// It writes these frames to a buffer and pushes it to the `appsrcAudio` element of the
// GStreamer pipeline created in `receiveGstreamerAudioVideo`. Any logic to modify / discard the frames would go here
VOID onGstAudioFrameReady(UINT64 customData, PFrame pFrame)
{
    STATUS retStatus = STATUS_SUCCESS;
    GstFlowReturn ret;
    GstBuffer *buffer;
    GstElement *appsrcAudio = (GstElement *)customData;

    CHK_ERR(appsrcAudio != NULL, STATUS_NULL_ARG, "appsrcAudio is null");
    CHK_ERR(pFrame != NULL, STATUS_NULL_ARG, "Audio frame is null");

    if (!eos)
    {
        buffer = gst_buffer_new_allocate(NULL, pFrame->size, NULL);
        CHK_ERR(buffer != NULL, STATUS_NULL_ARG, "Buffer allocation failed");

        DLOGV("Audio frame size: %d, presentationTs: %llu", pFrame->size, presentationTsIncrement);

        GST_BUFFER_DTS(buffer) = presentationTsIncrement;
        GST_BUFFER_PTS(buffer) = presentationTsIncrement;
        GST_BUFFER_DURATION(buffer) = gst_util_uint64_scale(pFrame->size, GST_SECOND, DEFAULT_AUDIO_OPUS_BYTE_RATE);

        // TODO: check for other codecs once the pipelines are added

        if (gst_buffer_fill(buffer, 0, pFrame->frameData, pFrame->size) != pFrame->size)
        {
            DLOGE("Buffer fill did not complete correctly");
            gst_buffer_unref(buffer);
            return;
        }
        g_signal_emit_by_name(appsrcAudio, "push-buffer", buffer, &ret);
        if (ret != GST_FLOW_OK)
        {
            DLOGE("Error pushing buffer: %s", gst_flow_get_name(ret));
        }
        gst_buffer_unref(buffer);
    }
CleanUp:
    return;
}

// This function is a callback for the streaming session shutdown event. We send an eos to the pipeline to exit the
// application using this.
VOID onSampleStreamingSessionShutdown(UINT64 customData, PSampleStreamingSession pSampleStreamingSession)
{
    (void)(pSampleStreamingSession);
    eos = TRUE;
    GstElement *pipeline = (GstElement *)customData;
    gst_element_send_event(pipeline, gst_event_new_eos());
}

PVOID receiveGstreamerAudioVideo(PVOID args)
{
    STATUS retStatus = STATUS_SUCCESS;
    GstElement *appsrcAudio = NULL, *appsrcVideo = NULL;
    std::shared_ptr<GstElement> pipeline;
    GError *error = NULL;
    GstBus *bus;
    GstMessage *msg;
    GstCaps *audiocaps, *videocaps;
    PSampleStreamingSession pSampleStreamingSession = (PSampleStreamingSession)args;
    PSampleConfiguration pSampleConfiguration = pSampleStreamingSession->pSampleConfiguration;
    PCHAR roleType = "Viewer";

    if (pSampleConfiguration->channelInfo.channelRoleType == SIGNALING_CHANNEL_ROLE_TYPE_MASTER)
    {
        roleType = "Master";
    }

    // CHK_ERR(gst_init_check(NULL, NULL, &error), STATUS_INTERNAL_ERROR, "[KVS GStreamer %s] GStreamer initialization failed");

    CHK_ERR(pSampleStreamingSession != NULL, STATUS_NULL_ARG, "[KVS Gstreamer %s] Sample streaming session is NULL", roleType);

    // It is advised to modify the pipeline and the caps as per the source of the media. Customers can also modify this pipeline to
    // use any other sinks instead of `filesink` like `autovideosink` and `autoaudiosink`. The existing pipelines are not complex enough to
    // change caps and properties dynamically, more complex logic may be needed to support the same.

    // This part can be in our GUI code, we can pass the track.codec and media-type and recieve the set up gst pipeline,
    // this part also creates a new GstCaps that contains one GstStructure with name appsrc-video
    if (setupPipelineCallback)
    {
        setupPipelineCallback(pipeline);
    }
     // also add audio caps
    CHK_ERR(pipeline != NULL, STATUS_INTERNAL_ERROR, "[KVS GStreamer %s] Pipeline is NULL", roleType);

    appsrcVideo = gst_bin_get_by_name(GST_BIN(pipeline.get()), "appsrc-video"); // retrieves element from bin
    CHK_ERR(appsrcVideo != NULL, STATUS_INTERNAL_ERROR, "[KVS GStreamer %s] Cannot find appsrc video", roleType);
    CHK_STATUS(transceiverOnFrame(pSampleStreamingSession->pVideoRtcRtpTransceiver, (UINT64)appsrcVideo, onGstVideoFrameReady)); // callback when new frame recieved from transciever
    g_object_set(G_OBJECT(appsrcVideo), "caps", videocaps, NULL);
    gst_caps_unref(videocaps);
    // audio part added
    // if (pSampleConfiguration->mediaType == SAMPLE_STREAMING_AUDIO_VIDEO)
    // {
    //     appsrcAudio = gst_bin_get_by_name(GST_BIN(pipeline), "appsrc-audio");
    //     CHK_ERR(appsrcAudio != NULL, STATUS_INTERNAL_ERROR, "[KVS GStreamer %s] Cannot find appsrc audio", roleType);
    //     CHK_STATUS(transceiverOnFrame(pSampleStreamingSession->pAudioRtcRtpTransceiver, (UINT64)appsrcAudio, onGstAudioFrameReady));
    //     g_object_set(G_OBJECT(appsrcAudio), "caps", audiocaps, NULL);
    //     gst_caps_unref(audiocaps);
    // }

    CHK_STATUS(streamingSessionOnShutdown(pSampleStreamingSession, (UINT64)pipeline.get(), onSampleStreamingSessionShutdown)); // callback on shutdown

    /* block until error or EOS */
    bus = gst_element_get_bus(pipeline.get());
    CHK_ERR(bus != NULL, STATUS_INTERNAL_ERROR, "[KVS GStreamer %s] Bus is NULL", roleType);
    msg = gst_bus_timed_pop_filtered(bus, GST_CLOCK_TIME_NONE, GST_MESSAGE_ERROR | GST_MESSAGE_EOS); // blocking until receiving an error or end of stream

    /* Free resources */
    if (msg != NULL)
    {
        switch (GST_MESSAGE_TYPE(msg))
        {
        case GST_MESSAGE_ERROR:
            gst_message_parse_error(msg, &error, NULL);
            DLOGE("Error received: %s", error->message);
            g_error_free(error);
            break;
        case GST_MESSAGE_EOS:
            DLOGI("End of stream");
            break;
        default:
            break;
        }
        gst_message_unref(msg);
    }
    if (bus != NULL)
    {
        gst_object_unref(bus);
    }
    if (pipeline != NULL)
    {
        gst_element_set_state(pipeline.get(), GST_STATE_NULL);
        gst_object_unref(pipeline.get());
    }
    if (appsrcAudio != NULL)
    {
        gst_object_unref(appsrcAudio);
    }
    if (appsrcVideo != NULL)
    {
        gst_object_unref(appsrcVideo);
    }

CleanUp:
    if (error != NULL)
    {
        DLOGE("[KVS GStreamer %s] %s", roleType, error->message);
        g_clear_error(&error);
    }

    gst_deinit();

    return (PVOID)(ULONG_PTR)retStatus;
}
