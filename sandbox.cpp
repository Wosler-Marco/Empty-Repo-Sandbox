#include <stdio.h>
#include <tchar.h>

#include <aws/core/Aws.h>
#include <aws/core/utils/memory/stl/AWSString.h>
#include <aws/core/utils/logging/DefaultLogSystem.h>
#include <aws/core/utils/logging/AWSLogging.h>
#include <aws/core/auth/AWSCredentialsProvider.h>
#include <aws/core/auth/AWSCredentialsProviderChain.h>
#include <aws/core/auth/AWSCredentials.h>

#include <aws/kinesisvideo/KinesisVideoClient.h>
#include <aws/kinesisvideo/model/DescribeSignalingChannelRequest.h>
#include <aws/kinesisvideo/model/GetSignalingChannelEndpointRequest.h>
#include <aws/kinesisvideo/model/SingleMasterChannelEndpointConfiguration.h>

#include <JSON/json.hpp> // Using a JSON library like JSON for Modern C++

// void onMessageReceived(const std::string& message) {
//     // Parse the JSON message
//     // Json::CharReaderBuilder readerBuilder;
//     nlohmann::json jsonMessage = nlohmann::json::parse(message);
//     // std::string errs;

//     // std::istringstream s(message);
//     // // std::string errs;
//     // if (!Json::parseFromStream(readerBuilder, s, &jsonMessage, &errs)) {
//     //     std::cerr << "Error parsing JSON: " << errs << std::endl;
//     //     return;
//     // }

//     // Extract data from the JSON message
//     std::string accessKeyId = jsonMessage["credentials"]["accessKeyId"].get<std::string>();
//     std::string secretAccessKey = jsonMessage["credentials"]["secretAccessKey"].get<std::string>();
//     std::string sessionToken = jsonMessage["credentials"]["sessionToken"].get<std::string>();
//     std::string channelArn = jsonMessage["channelData"]["ChannelArn"].get<std::string>();
//     std::string channelName = jsonMessage["channelData"]["ChannelName"].get<std::string>();
    
//     // ... Extract other necessary data ...
    
//     Aws::Auth::AWSCredentials credentials(accessKeyId, secretAccessKey, sessionToken);

//     // Initialize AWS SDK with the temporary credentials
//     Aws::Client::ClientConfiguration clientConfig;
//     clientConfig.region = "us-west-2"; // Set your region

//     Aws::KinesisVideo::KinesisVideoClient kvClient(credentials, clientConfig);

//     // Describe the signaling channel
//     Aws::KinesisVideo::Model::DescribeSignalingChannelRequest describeRequest;
//     describeRequest.SetChannelName(channelName);

//     auto describeOutcome = kvClient.DescribeSignalingChannel(describeRequest);
//     if (!describeOutcome.IsSuccess()) {
//         std::cerr << "Error describing signaling channel: " << describeOutcome.GetError().GetMessage() << std::endl;
//         return;
//     }

//     auto channelInfo = describeOutcome.GetResult().GetChannelInfo();

//     // Get signaling channel endpoint
//     Aws::KinesisVideo::Model::GetSignalingChannelEndpointRequest endpointRequest;
//     endpointRequest.SetChannelARN(channelInfo.GetChannelARN());
//     Aws::KinesisVideo::Model::SingleMasterChannelEndpointConfiguration endpointConfig;
//     endpointConfig.SetProtocols({Aws::KinesisVideo::Model::ChannelProtocol::WSS});
//     endpointConfig.SetRole(Aws::KinesisVideo::Model::ChannelRole::MASTER);
//     endpointRequest.SetSingleMasterChannelEndpointConfiguration(endpointConfig);

//     auto endpointOutcome = kvClient.GetSignalingChannelEndpoint(endpointRequest);
//     if (!endpointOutcome.IsSuccess()) {
//         std::cerr << "Error getting signaling channel endpoint: " << endpointOutcome.GetError().GetMessage() << std::endl;
//         return;
//     }

//     auto endpoints = endpointOutcome.GetResult().GetResourceEndpointList();
//     std::string wssEndpoint;
//     for (const auto& endpoint : endpoints) {
//         if (endpoint.GetProtocol() == Aws::KinesisVideo::Model::ChannelProtocol::WSS) {
//             wssEndpoint = endpoint.GetResourceEndpoint();
//             break;
//         }
//     }

//     if (wssEndpoint.empty()) {
//         std::cerr << "No WSS endpoint found for the signaling channel" << std::endl;
//         return;
//     }

//     // Proceed with WebRTC peer-to-peer connection setup using wssEndpoint
//     // ...
// }

int main(int argc, char **argv)
{
    Aws::SDKOptions options;
    Aws::InitAPI(options);
    {
        Aws::Client::ClientConfiguration clientConfig;
        Aws::KinesisVideo::KinesisVideoClient kinesisVideoClient(clientConfig);

        Aws::KinesisVideo::Model::SingleMasterChannelEndpointConfiguration endpointConfig;
        endpointConfig.SetProtocols({Aws::KinesisVideo::Model::ChannelProtocol::HTTPS});
        endpointConfig.SetRole(Aws::KinesisVideo::Model::ChannelRole::MASTER);

        Aws::KinesisVideo::Model::GetSignalingChannelEndpointRequest endpointRequest;
        endpointRequest.SetChannelARN(""); // Replace with your channel name
        endpointRequest.SetSingleMasterChannelEndpointConfiguration(endpointConfig);

        auto outcome = kinesisVideoClient.GetSignalingChannelEndpoint(endpointRequest);

        if (outcome.IsSuccess())
        {
            const auto& endpointList = outcome.GetResult().GetResourceEndpointList();
            for (const auto& endpoint : endpointList)
            {
                // std::cout << "Endpoint: " << endpoint.GetProtocol() << " - " << endpoint.GetResourceEndpoint() << std::endl;
            }
        }
        else
        {
            std::cerr << "Failed to get signaling channel endpoint: " << outcome.GetError().GetMessage() << std::endl;
        }
    }
    Aws::ShutdownAPI(options);
    return 0;
}