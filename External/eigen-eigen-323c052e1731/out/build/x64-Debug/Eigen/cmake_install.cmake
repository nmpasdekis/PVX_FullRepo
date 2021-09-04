# Install script for directory: D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen

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
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/eigen3/Eigen" TYPE FILE FILES
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/Cholesky"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/CholmodSupport"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/Core"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/Dense"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/Eigen"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/Eigenvalues"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/Geometry"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/Householder"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/IterativeLinearSolvers"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/Jacobi"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/LU"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/MetisSupport"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/OrderingMethods"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/PaStiXSupport"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/PardisoSupport"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/QR"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/QtAlignedMalloc"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/SPQRSupport"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/SVD"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/Sparse"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/SparseCholesky"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/SparseCore"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/SparseLU"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/SparseQR"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/StdDeque"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/StdList"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/StdVector"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/SuperLUSupport"
    "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/UmfPackSupport"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xDevelx" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/eigen3/Eigen" TYPE DIRECTORY FILES "D:/GitProjects2022/PVX_FullRepo/External/eigen-eigen-323c052e1731/Eigen/src" FILES_MATCHING REGEX "/[^/]*\\.h$")
endif()

