# Smartglasses Android APP
## Introduction
An APP that can receive bluetooth audio from a specific device and transcribe it into words with Whisper. To make it compatible with our smartglasses project in progress, we'll keep updating its code, so that it can communicate with our AI model and make proper responses, finally returning the results to the bluetooth device.
## How to build APK ?
### With Qt Creator
The APP is written with Qt in C++, so you need to install Qt Creator(Recommended) and Android SDK to build the project, the version required are as settings.json file. The detailed introduction can be easily reached on the Internet.

    Tips: Release builds are signed with a keystore; its path, store/key passwords and alias are configured in `.vscode/settings.json`. The `android_release.keystore` and the `.vscode/settings.json` committed in this repository are a test/demo signing key that is public on purpose: fine for trying the app, never for publishing it (see `docs/adr/0001-committed-test-signing-key.md`). For a real release, generate your own keystore and keep it and its passwords out of version control; `.vscode/settings.example.json` is the template.

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
