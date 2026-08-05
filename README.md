# Smartglasses Android APP
## Introduction
An APP that can receive bluetooth audio from a specific device and transcribe it into words with Whisper. To make it compatible with our smartglasses project in progress, we'll keep updating its code, so that it can communicate with our AI model and make proper responses, finally returning the results to the bluetooth device.
## How to build APK ?
The APP is written with Qt in C++, so you need to install Qt Creator(Recommended) and Android SDK to build the project. The detailed introduction can be easily reached on the Internet.

    Tips: When build in the release mode, use the "android_release.keystore" file in the project as the singature file, whose password is "smartglasses".

## Todo
- Upload the ASR results to our AI model and get its response via Tailscale API
- Send the AI response to the bluetooth device.
- (Maybe)Integrate a simple model on mobile phone to anwser some simple questions when offline, and improve the responding speed.
- Improve the GUI, rebuild the app with QML