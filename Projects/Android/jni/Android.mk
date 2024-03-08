LOCAL_PATH := $(call my-dir)

BEEFRAIDERXR_BASE_CFLAGS = -DHAVE_GLES -DFINAL_BUILD -fexceptions  -Wall -Wno-write-strings -Wno-comment   -fno-caller-saves -fno-tree-vectorize -Wno-unused-but-set-variable -fvisibility=hidden
BEEFRAIDERXR_BASE_CPPFLAGS = -fvisibility-inlines-hidden -Wno-invalid-offsetof -fvisibility=hidden

# Optimisation flags required for release build so weapon selector beam is visible
ifeq ($(NDK_DEBUG), 0)
BEEFRAIDERXR_BASE_CFLAGS += -O1
BEEFRAIDERXR_BASE_CPPFLAGS += -O1
endif

BEEFRAIDERXR_BASE_LDLIBS =

BEEFRAIDERXR_BASE_LDLIBS += -Wl

BEEFRAIDERXR_BASE_C_INCLUDES :=   $(BEEFRAIDERXR_PATH)
BEEFRAIDERXR_BASE_C_INCLUDES +=   ${OPENLARA_PATH}/src
BEEFRAIDERXR_BASE_C_INCLUDES +=   ${TOP_DIR}/OpenXR-SDK/include
BEEFRAIDERXR_BASE_C_INCLUDES +=   ${TOP_DIR}/OpenXR-SDK/src/common


include $(TOP_DIR)/Android_beefraiderxr.mk



