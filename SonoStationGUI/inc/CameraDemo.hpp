
#include <iostream>
#include <filesystem>
#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QTimer>
#include <QToolButton>
#include <QtCore/QtCore>
#include <QtWidgets/QtWidgets>
#include <QPointer>
#include <QGroupBox>
#include <gst/gst.h>
// #include <QSizePolicy>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
using namespace cv;

#include "CameraStreamingWidget.hpp"
namespace wosler
{
    namespace demo
    {

        class CameraDemo : public QMainWindow
        {
            Q_OBJECT
        public:
            CameraDemo(QWidget *parent = nullptr) : QMainWindow(parent)
            {
                setupWidget();
            }
            void SetupPipeline(std::shared_ptr<GstElement> &pipeline)
            {
                gst_init(nullptr, nullptr);
                std::string pipelineDesc = "appsrc name=appsrc-video ! queue ! h264parse ! avdec_h264 ! videoconvert ! video/x-raw,format=RGB ! appsink sync=TRUE emit-signals=TRUE name=appsink-video";
                GError *error = nullptr;
                newpipeline = std::shared_ptr<GstElement>(gst_parse_launch(pipelineDesc.c_str(), &error), gst_object_unref);
                if (!newpipeline)
                {
                    g_printerr("Failed to create pipeline: %s\n", error->message);
                    g_clear_error(&error);
                    return;
                }
                cameraStreamingWidget->createPipeline(newpipeline);
                std::cout << "SonoStationGUI: created pipeline " << std::endl;
                pipeline = newpipeline;
            }

        private:
            QPointer<QWidget> central;
            CameraStreamingWidget *cameraStreamingWidget;
            std::shared_ptr<GstElement> newpipeline;

            void setupWidget()
            {
                cameraStreamingWidget = new CameraStreamingWidget(this);
                central = cameraStreamingWidget;
                setCentralWidget(central);
                showMaximized();
            }

            // signals:
            //     void PiplineSignal(std::shared_ptr<GstElement> &pipeline);
        };
    };
}