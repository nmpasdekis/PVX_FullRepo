# Install script for directory: D:/mine/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "D:/mine/PVX_FullRepo/External/eigen-eigen-323c052e1731/out/install/x64-Debug")
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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/eigen3/Eigen" TYPE FILE FILES
    "D:/mine/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/Cholesky"
    "D:/mine/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/CholmodSupport"
    "D:/mine/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/Core"
    "D:/mine/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/Dense"
    "D:/mine/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/Eigen"
    "D:/mine/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/Eigenvalues"
    "D:/mine/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/Geometry"
    "D:/mine/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/Householder"
    "D:/mine/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/IterativeLinearSolvers"
    "D:/mine/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/Jacobi"
    "D:/mine/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/LU"
    "D:/mine/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/MetisSupport"
    "D:/mine/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/OrderingMethods"
    "D:/mine/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/PaStiXSupport"
    "D:/mine/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/PardisoSupport"
    "D:/mine/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/QR"
    "D:/mine/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/QtAlignedMalloc"
    "D:/mine/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/SPQRSupport"
    "D:/mine/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/SVD"
    "D:/mine/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/Sparse"
    "D:/mine/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/SparseCholesky"
    "D:/mine/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/SparseCore"
    "D:/mine/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/SparseLU"
    "D:/mine/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/SparseQR"
    "D:/mine/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/StdDeque"
    "D:/mine/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/StdList"
    "D:/mine/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/StdVector"
    "D:/mine/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/SuperLUSupport"
    "D:/mine/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/UmfPackSupport"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xDevelx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/eigen3/Eigen" TYPE DIRECTORY FILES "D:/mine/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/src" FILES_MATCHING REGEX "/[^/]*\\.h$")
endif()

