radiant_plugin(camera
        camera.cpp camera.h
        dialogs.cpp dialogs.h
        dialogs_common.cpp
        funchandlers.cpp funchandlers.h
        listener.cpp listener.h
		misc.cpp misc.h
		renderer.cpp renderer.h
        )

target_include_directories(camera PRIVATE uilib)
target_link_libraries(camera PRIVATE uilib)