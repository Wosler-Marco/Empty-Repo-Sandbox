
#include <iostream>
#include <filesystem>
#include <QApplication>
#include <QMainWindow>
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QPushButton>
#include <QtCore/QtCore>
#include <QtWidgets/QtWidgets>
#include <QPointer>
#include <QMessageBox>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
#include <gst/gst.h>
#include <gst/app/app.h>
#include <gst/app/gstappsink.h>
#include <functional>
#include <memory>

using namespace cv;

namespace wosler
{
    namespace demo
    {

        class CameraStreamingWidget : public QWidget
        {
            Q_OBJECT

        private:
            QLabel *camLabel;
            QPixmap pixmap;
            QTimer camTimer;
            QPointer<QVBoxLayout> mainLayout;
            QPushButton *exitButton;
            std::shared_ptr<GstElement> newpipeline;
            GstElement *appsink;
            static GstFlowReturn
            onNewSampleFromSink(GstAppSink *sink, gpointer data)
            {
                CameraStreamingWidget *self = static_cast<CameraStreamingWidget *>(data);
                GstSample *sample = gst_app_sink_pull_sample(sink);
                if (sample)
                {
                    // std::cout << "SonoStationGUI: Received a Frame" << std::endl;
                    GstBuffer *buffer = gst_sample_get_buffer(sample);
                    GstMapInfo map;
                    gst_buffer_map(buffer, &map, GST_MAP_READ);

                    Mat frame(Size(1280, 720), CV_8UC3, (char *)map.data, Mat::AUTO_STEP);
                    // Display the frame using OpenCV

                    // Mat frame(Size(720, 1280), CV_8UC3, (char *)map.data, map.size / 720);
                    // cv::Mat frame(1280,720, CV_8UC3, (char *)map.data);
                    // cv::Mat rgb_frame;
                    if (!frame.empty())
                    {
                        // std::cout << "SonoStationGUI: Frame is not empty so displaying" << std::endl;
                        // Display the frame for 1 ms
                        // Display the frame using OpenCV
                        // std::cout << "Frame size: " << frame.cols << "x" << frame.rows << std::endl;
                        // cvtColor(frame, rgb_frame, COLOR_BGR2RGB); // Convert the color space to match Qt
                        QImage qt_image((const unsigned char *)(frame.data), frame.cols, frame.rows, QImage::Format_RGB888);

                        // std::cout << "QImage size: " << qt_image.width() << "x" << qt_image.height() << std::endl;
                        QPixmap pixmap = QPixmap::fromImage(qt_image);
                        pixmap = pixmap.scaled(1280, 720, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                        self->camLabel->setPixmap(pixmap);
                    }

                    gst_buffer_unmap(buffer, &map);
                    gst_sample_unref(sample);
                    return GST_FLOW_OK;
                }
                else
                {
                    std::cout << "No sample" << "\n";
                }
                return GST_FLOW_ERROR;
            }

            void onExit()
            {
                QApplication::quit(); // close the application onExit button/X button clicked
            }

        public:
            CameraStreamingWidget(QWidget *parent = nullptr) : QWidget(parent)
            {
                mainLayout = new QVBoxLayout(this);

                setupCams();                                                                      // triggering setupCams functikn
                exitButton = new QPushButton("Exit", this);                                       // creating exit button
                mainLayout->addWidget(exitButton);                                                // adding exit button to layout
                connect(exitButton, &QPushButton::clicked, this, &CameraStreamingWidget::onExit); // connecting button to exit function
            }

            void setupCams()
            {
                camLabel = new QLabel();                                // This "label" holds the image contents
                camLabel->setAlignment(Qt::AlignTop | Qt::AlignCenter); // Align it top and center
                mainLayout->addWidget(camLabel);                        // Add it to the mainLayout
            }

            // public slots:
            void createPipeline(std::shared_ptr<GstElement> &newpipeline)
            {
                appsink = gst_bin_get_by_name(GST_BIN(newpipeline.get()), "appsink-video");
                std::cout << "SonoStationGUI: in CreatePipeline in Widget!" << std::endl;
                gst_app_sink_set_emit_signals((GstAppSink *)appsink, true);
                g_signal_connect(appsink, "new-sample", G_CALLBACK(onNewSampleFromSink), this);
                gst_element_set_state(newpipeline.get(), GST_STATE_PLAYING);
            }
        };

    }
}