# Find Bullet includes and library
#
# This module defines
#  BULLET_INCLUDE_DIRS  The location of the Bullet header files
#  BULLET_LIBRARIES     The libraries to link against to use Bullet
#  BULLET_FOUND         If false, do not try to use Bullet
#
# To specify an additional directory to search, set BULLET_ROOT.
#
# If the absence of Bullet is a fatal error, set BULLET_FIND_REQUIRED to TRUE.
#
# Original copyright (c) 2007, Matt Williams
#
# Heavily modified to work on Unix, and with the Meru project conventions, by
# Siddhartha Chaudhuri, 2008.
#
# Redistribution and use is allowed according to the terms of the BSD license.

SET(BULLET_FOUND FALSE)

IF(WIN32) # Windows

  SET(BULLET_DIR ${BULLET_ROOT})
  IF(BULLET_DIR AND EXISTS "${BULLET_DIR}")
    SET(BULLET_INCLUDE_DIRS ${BULLET_DIR}/include)
    SET(BULLET_LIBRARIES
	debug ${BULLET_DIR}/lib/Debug/libBulletCollision_d.lib
	debug ${BULLET_DIR}/lib/Debug/libBulletDynamics_d.lib
	debug ${BULLET_DIR}/lib/Debug/libLinearMath_d.lib
        optimized ${BULLET_DIR}/lib/Release/libBulletCollision.lib
        optimized ${BULLET_DIR}/lib/Release/libBulletDynamics.lib
	optimized ${BULLET_DIR}/lib/Release/libLinearMath.lib)

    SET(BULLET_FOUND TRUE)
  ENDIF(BULLET_DIR AND EXISTS "${BULLET_DIR}")

ELSE(WIN32) # Unix

  SET(BULLET_DIR_DESCRIPTION "root directory of Bullet installation. E.g /usr/local or /opt")

  # Look for an installation, first in the user-specified location and then in the system locations
  FIND_PATH(BULLET_DIR NAMES lib/pkgconfig/bullet.pc PATHS ${BULLET_ROOT} DOC "The ${BULLET_DIR_DESCRIPTION}" NO_DEFAULT_PATH)
  IF(NOT BULLET_DIR)  # now look in system locations
    FIND_PATH(BULLET_DIR NAMES lib/pkgconfig/bullet.pc DOC "The ${BULLET_DIR_DESCRIPTION}")
  ENDIF(NOT BULLET_DIR)

  # Now try to get the include and library path.
  IF(BULLET_DIR)
    SET(BULLET_INCLUDE_DIRS ${BULLET_DIR}/include/bullet)

    IF(EXISTS ${BULLET_INCLUDE_DIRS})
      SET(BULLET_LIBRARY_DIR ${BULLET_DIR}/lib)
      FIND_LIBRARY(BULLET_LIB_DYNAMICS NAMES BulletDynamics PATHS ${BULLET_LIBRARY_DIR} NO_DEFAULT_PATH)
      FIND_LIBRARY(BULLET_LIB_COLLISION NAMES BulletCollision PATHS ${BULLET_LIBRARY_DIR} NO_DEFAULT_PATH)
      FIND_LIBRARY(BULLET_LIB_MATH NAMES LinearMath PATHS ${BULLET_LIBRARY_DIR} NO_DEFAULT_PATH)

      IF(BULLET_LIB_DYNAMICS AND BULLET_LIB_COLLISION AND BULLET_LIB_MATH)
        SET(BULLET_FOUND TRUE)
        SET(BULLET_LIBRARIES ${BULLET_LIB_DYNAMICS} ${BULLET_LIB_COLLISION} ${BULLET_LIB_MATH})
      ENDIF(BULLET_LIB_DYNAMICS AND BULLET_LIB_COLLISION AND BULLET_LIB_MATH)
    ENDIF(EXISTS ${BULLET_INCLUDE_DIRS})
  ENDIF(BULLET_DIR)

ENDIF(WIN32)

IF(BULLET_FOUND)
  IF(NOT BULLET_FIND_QUIETLY)
    MESSAGE(STATUS "Found Bullet at ${BULLET_DIR}")
  ENDIF(NOT BULLET_FIND_QUIETLY)
ELSE(BULLET_FOUND)
  IF(BULLET_FIND_REQUIRED)
    MESSAGE(FATAL_ERROR "Bullet not found")
  ENDIF(BULLET_FIND_REQUIRED)
ENDIF(BULLET_FOUND)
