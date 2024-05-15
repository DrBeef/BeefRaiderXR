
LOCAL_PATH := $(call my-dir)


include $(CLEAR_VARS)


LOCAL_MODULE    := beefraiderxr


LOCAL_CFLAGS :=  $(BEEFRAIDERXR_BASE_CFLAGS)
LOCAL_CPPFLAGS := $(BEEFRAIDERXR_BASE_CPPFLAGS)

LOCAL_LDLIBS := $(BEEFRAIDERXR_BASE_LDLIBS)


LOCAL_LDLIBS +=  -lGLESv3 -landroid -lEGL -llog -lz -lOpenSLES

#Needed so lib can be loaded (_exit error)
#LOCAL_LDLIBS += -fuse-ld=bfd

# LOCAL_STATIC_LIBRARIES := sigc libzip libpng libminizip
LOCAL_SHARED_LIBRARIES := openxr_loader


LOCAL_C_INCLUDES :=  $(BEEFRAIDERXR_BASE_C_INCLUDES) $(TOP_DIR)


#############################################################################
# CLIENT/SERVER
#############################################################################




BEEFRAIDERXR_SRC_FILES :=  ${BEEFRAIDERXR_PATH}/BeefRaiderXR_Main.cpp \
       ${BEEFRAIDERXR_PATH}/android/TBXR_Common.cpp \
       ${BEEFRAIDERXR_PATH}/android/argtable3.c \
       ${BEEFRAIDERXR_PATH}/OpenXrInput.cpp \
       ${OPENLARA_PATH}/src/libs/tinf/tinflate.c \
       ${OPENLARA_PATH}/src/libs/stb_vorbis/stb_vorbis.c \
       ${OPENLARA_PATH}/src/libs/minimp3/minimp3.cpp

LOCAL_SRC_FILES += $(BEEFRAIDERXR_SRC_FILES)


include $(BUILD_SHARED_LIBRARY)


$(call import-module,AndroidPrebuilt/jni)
