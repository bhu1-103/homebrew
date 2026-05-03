cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(WiFi-Mic)

include("${VITASDK}/share/vita.cmake" REQUIRED)

set(VITA_APP_NAME "WiFi Mic")
set(VITA_TITLEID  "BHU100002")
set(VITA_VERSION  "01.00")

# ---- C++17 required by imgui-vita ----
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---- Include imgui-vita headers ----
include_directories(
  imgui-vita
)

# ---- Tell linker where libimgui.a lives ----
link_directories(
  ${CMAKE_SOURCE_DIR}/imgui-vita
)

# ---- Build executable ----
add_executable(${PROJECT_NAME}
  src/main.cpp
)

# ---- Link libraries (mirrors official imgui-vita Makefile) ----
target_link_libraries(${PROJECT_NAME}
  imgui
  vitaGL
  c
  m

  SceCommonDialog_stub
  SceLibKernel_stub
  SceDisplay_stub
  SceGxm_stub
  SceSysmodule_stub
  SceCtrl_stub
  SceTouch_stub
  SceAppMgr_stub
  SceAppUtil_stub
  SceKernelDmacMgr_stub
  SceShaccCg_stub
  SceShaccCgExt
  taihen_stub

  vitashark
  mathneon
)

# ---- Create Vita package ----
vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME})

vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
)
