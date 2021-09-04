# Install script for directory: D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/unsupported/Eigen

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/out/install/x64-Debug")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xDevelx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/eigen3/unsupported/Eigen" TYPE FILE FILES
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/unsupported/Eigen/AdolcForward"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/unsupported/Eigen/AlignedVector3"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/unsupported/Eigen/ArpackSupport"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/unsupported/Eigen/AutoDiff"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/unsupported/Eigen/BVH"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/unsupported/Eigen/EulerAngles"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/unsupported/Eigen/FFT"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/unsupported/Eigen/IterativeSolvers"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/unsupported/Eigen/KroneckerProduct"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/unsupported/Eigen/LevenbergMarquardt"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/unsupported/Eigen/MatrixFunctions"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/unsupported/Eigen/MoreVectorization"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/unsupported/Eigen/MPRealSupport"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/unsupported/Eigen/NonLinearOptimization"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/unsupported/Eigen/NumericalDiff"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/unsupported/Eigen/OpenGLSupport"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/unsupported/Eigen/Polynomials"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/unsupported/Eigen/Skyline"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/unsupported/Eigen/SparseExtra"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/unsupported/Eigen/SpecialFunctions"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/unsupported/Eigen/Splines"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xDevelx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/eigen3/unsupported/Eigen" TYPE DIRECTORY FILES "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/unsupported/Eigen/src" FILES_MATCHING REGEX "/[^/]*\\.h$")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/out/build/x64-Debug/unsupported/Eigen/CXX11/cmake_install.cmake")

endif()

