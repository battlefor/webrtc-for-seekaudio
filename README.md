# webrtc-for-seekaudio
This is a code demo to show how to call the seekaudio lib.

这是一个代码示例，展示如何调用seekaudio 库 


下载webrtc源代码，使用git checkout -b 7204 remotes/branch-heads/7204切换到7204分支，编译webrtc。参考该示例的修改记录修改相应文件的代码。然后在src目录下执行如下命令：./tools_webrtc/android/build_aar.py --arch 'arm64-v8a' --build-dir './' --extra-gn-args 'treat_warnings_as_errors=false'   执行时会在src目录下面生成一个新目录arm64-v8a，并且会提示找不到libseekaudio_aec.so文件，把这些库文件拷贝到新生成的arm64-v8a目录重新执行即可。

webrtc的稳定分支可以参考如下：https://chromiumdash.appspot.com/branches
