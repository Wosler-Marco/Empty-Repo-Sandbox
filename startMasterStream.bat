@echo off
rem Get the directory of the batch script
set scriptdir=%~dp0

rem Remove the trailing backslash if there is one
set scriptdir=%scriptdir:~0,-1%

rem Set your environment variables here
set AWS_ACCESS_KEY_ID=
set AWS_SECRET_ACCESS_KEY=
set AWS_DEFAULT_REGION=ca-central-1
set "AWS_KVS_CACERT_PATH=%scriptdir%\external\AWS\certs\cert.pem"
set AWS_KVS_LOG_LEVEL=5

rem Run the executable from the relative path
"%scriptdir%\build\bin\kvs_webrtc_test.exe" testchannel