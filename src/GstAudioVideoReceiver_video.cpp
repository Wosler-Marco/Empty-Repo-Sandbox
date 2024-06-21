#include "Samples.h"
#include <opencv2/opencv.hpp>
#include <gst/gst.h>
#include <gst/app/app.h>
#include <gst/app/gstappsink.h>

static UINT64 presentationTsIncrement = 0;
static BOOL eos = FALSE;

GstFlowReturn ReceiverCallback(GstElement *sink, gpointer user_data)
{
    DLOGI("Receiving function");
    GstSample *sample;
    GstBuffer *buffer;
    GstMapInfo map;

    sample = gst_app_sink_pull_sample(GST_APP_SINK(sink));
    if (!sample)
    {
        return GST_FLOW_ERROR;
    }

    buffer = gst_sample_get_buffer(sample);
    if (!buffer)
    {
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }

    if (!gst_buffer_map(buffer, &map, GST_MAP_READ))
    {
        gst_sample_unref(sample);
        return GST_FLOW_ERROR;
    }
    cv::Mat frame(DEFAULT_VIDEO_HEIGHT_PIXELS, DEFAULT_VIDEO_WIDTH_PIXELS, CV_8UC3, (char *)map.data);

    // Display the frame using OpenCV
    cv::imshow("GStreamer Video", frame);
    cv::waitKey(1); // Display the frame for 1 ms

    gst_buffer_unmap(buffer, &map);
    gst_sample_unref(sample);

    return GST_FLOW_OK;
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

void SetupPipelineCallback(GstElement **pipeline, GError **error, GstCaps **videocaps)
{
    if (!gst_init_check(NULL, NULL, error))
    {
        g_printerr("GStreamer initialization failed: %s\n", (*error)->message);
        return;
    }

    gchar *videoDescription = "appsrc name=appsrc-video ! queue ! h264parse ! avdec_h264 ! videoconvert ! video/x-raw,format=BGR ! appsink sync=TRUE emit-signals=TRUE name=appsink-video";

    *videocaps = gst_caps_new_simple("video/x-h264",
                                     "stream-format", G_TYPE_STRING, "byte-stream",
                                     "alignment", G_TYPE_STRING, "au",
                                     "profile", G_TYPE_STRING, "baseline",
                                     "height", G_TYPE_INT, DEFAULT_VIDEO_HEIGHT_PIXELS,
                                     "width", G_TYPE_INT, DEFAULT_VIDEO_WIDTH_PIXELS,
                                     NULL);

    *pipeline = gst_parse_launch(videoDescription, error); // create the actual pipeline

    if (*pipeline == NULL)
    {
        g_printerr("Pipeline could not be created: %s\n", (*error)->message);
        return;
    }
    GstElement *appsink = gst_bin_get_by_name(GST_BIN(*pipeline), "appsink-video");
    if (!appsink)
    {
        g_printerr("Failed to get appsink from pipeline\n");
        return;
    }

    // Configure appsink
    g_object_set(G_OBJECT(appsink), "emit-signals", TRUE, "sync", FALSE, NULL);
    g_signal_connect(appsink, "new-sample", G_CALLBACK(ReceiverCallback), NULL);

    gst_element_set_state(*pipeline, GST_STATE_PLAYING); // start the pipeline
    g_print("Pipeline created and set to PLAYING state!!!.\n");
}
PVOID receiveGstreamerAudioVideo(PVOID args)
{
    STATUS retStatus = STATUS_SUCCESS;
    GstElement *pipeline = NULL, *appsrcAudio = NULL, *appsrcVideo = NULL;
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

    g_print("*********** In receiveGstreamerAudioVideo.\n");
    // CHK_ERR(gst_init_check(NULL, NULL, &error), STATUS_INTERNAL_ERROR, "[KVS GStreamer %s] GStreamer initialization failed");

    CHK_ERR(pSampleStreamingSession != NULL, STATUS_NULL_ARG, "[KVS Gstreamer %s] Sample streaming session is NULL", roleType);

    // It is advised to modify the pipeline and the caps as per the source of the media. Customers can also modify this pipeline to
    // use any other sinks instead of `filesink` like `autovideosink` and `autoaudiosink`. The existing pipelines are not complex enough to
    // change caps and properties dynamically, more complex logic may be needed to support the same.

    // This part can be in our GUI code, we can pass the track.codec and media-type and recieve the set up gst pipeline,
    // this part also creates a new GstCaps that contains one GstStructure with name appsrc-video
    // if (callbackFn)
    // {
    //     callbackFn(pipeline);
    // }
    SetupPipelineCallback(&pipeline, &error, &videocaps); // also add audio caps
    CHK_ERR(pipeline != NULL, STATUS_INTERNAL_ERROR, "[KVS GStreamer %s] Pipeline is NULL", roleType);

    appsrcVideo = gst_bin_get_by_name(GST_BIN(pipeline), "appsrc-video"); // retrieves element from bin
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

    CHK_STATUS(streamingSessionOnShutdown(pSampleStreamingSession, (UINT64)pipeline, onSampleStreamingSessionShutdown)); // callback on shutdown

    /* block until error or EOS */
    bus = gst_element_get_bus(pipeline);
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
        gst_element_set_state(pipeline, GST_STATE_NULL);
        gst_object_unref(pipeline);
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
