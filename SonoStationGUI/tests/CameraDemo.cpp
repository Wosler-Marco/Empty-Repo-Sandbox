#include <QApplication>

#include "CameraDemo.hpp"
#include "Context.hpp"

typedef std::function<void(std::shared_ptr<GstElement> &)> CallbackFn;
static CallbackFn setupPipelineCallback = nullptr;
void registerSetupPipelineCallback(CallbackFn fn)
{
    g_print("Callback FN: Registered \n");
    setupPipelineCallback = fn;
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    gst_init(&argc, &argv);
    // Create the CameraDemo instance
    wosler::demo::CameraDemo demo;

    // Register the callback using a lambda function
    registerSetupPipelineCallback([&demo](std::shared_ptr<GstElement> &pipeline)
                                  { demo.SetupPipeline(pipeline); });



    // Show the CameraDemo window in the main thread
    demo.show();
    // Optionally, trigger the callback to test it
    std::shared_ptr<GstElement> pipeline;
    if (setupPipelineCallback)
    {
        setupPipelineCallback(pipeline);
    }
    // Perform any additional setup or background tasks in separate threads if needed
    QThread::create([]()
                    {
        // Example background task
        std::cout << "Background thread running..." << std::endl; })
        ->start();

    // Start the Qt event loop
    return app.exec();
}
