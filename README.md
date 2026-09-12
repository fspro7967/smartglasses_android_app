# Smartglasses Android APP
## Introduction
An APP that can receive bluetooth audio from a specific device and transcribe it into words with Whisper. To make it compatible with our smartglasses project in progress, we'll keep updating its code, so that it can communicate with our AI model and make proper responses, finally returning the results to the bluetooth device.
## How to build APK ?
### With Qt Creator
The APP is written with Qt in C++, so you need to install Qt Creator(Recommended) and Android SDK to build the project, the version required are as settings.json file. The detailed introduction can be easily reached on the Internet.

    Tips: When build in the release mode, use the "android_release.keystore" file in the project as the singature file, whose password is "smartglasses".

### With Visual Studio Code
Yes, I know many developers like coding in VSCode, but it's a pity that we can not completly say goodbye to Qt Creator and Android SDK. 

- First, you need to install Qt and Android SDK as what I mentioned above.

__MAKE SURE THAT EVERYTHING WORK WELL IN QT CREATOR__, it can save a lot of efforts when configuring in VSCode.

- Then, install these extensions in VSCode: C/C++, Qt C++, CMake Tools, Gradle for Java, Language Support for Java(TM) by Red Hat.

- Next, edit the paths in settings.json according to your own environment.

- Finally, press F1 to open the command panel and type CMake: Delete cache, Reconfigure and Build. Choose Qt-\[your-version\]-android_arm64_v8a-arm64 as toolkit.

## If you want to try our app......
APK built in release mode is recommended since we enable optimization in processing speed and accuracy.
## Todo
- ~~ Upload the ASR results to our AI model and get its response via Ollama API ~~
- ~~ Send the AI response to the bluetooth device ~~
- (Maybe)Integrate a simple model on mobile phone to anwser some simple questions when offline, and improve the responding speed.
-  ~~ Improve the GUI, rebuild the app with QML ~~

## Thanks
- My other 2 teammates, (  ) and (  )
- DeepSeek
- ChatGPT
- Gemini
- OpenRouter
