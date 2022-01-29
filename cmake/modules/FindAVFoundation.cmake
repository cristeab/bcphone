#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# - Try to find AVFoundation
# Once done, this will define
#
#  AVFoundation_FOUND - system has AVFoundation
#  AVFoundation_INCLUDE_DIRS - the AVFoundation include directories
#  AVFoundation_LIBRARIES - link these to use AVFoundation

include(FindPkgMacros)
findpkg_begin(AVFoundation)

# construct search paths
set(AVFoundation_PREFIX_PATH ${AVFoundation_HOME} $ENV{AVFoundation_HOME})
create_search_paths(AVFoundation)
# redo search if prefix path changed
clear_if_changed(AVFoundation_PREFIX_PATH
  AVFoundation_LIBRARY_FWK
  AVFoundation_LIBRARY_REL
  AVFoundation_LIBRARY_DBG
  AVFoundation_INCLUDE_DIR
)

set(AVFoundation_LIBRARY_NAMES AVFoundation)
get_debug_names(AVFoundation_LIBRARY_NAMES)

use_pkgconfig(AVFoundation_PKGC AVFoundation)

findpkg_framework(AVFoundation)

find_path(AVFoundation_INCLUDE_DIR NAMES AVFoundation.h HINTS ${AVFoundation_INC_SEARCH_PATH} ${AVFoundation_PKGC_INCLUDE_DIRS} PATH_SUFFIXES AVFoundation)
find_library(AVFoundation_LIBRARY_REL NAMES ${AVFoundation_LIBRARY_NAMES} HINTS ${AVFoundation_LIB_SEARCH_PATH} ${AVFoundation_PKGC_LIBRARY_DIRS})
find_library(AVFoundation_LIBRARY_DBG NAMES ${AVFoundation_LIBRARY_NAMES_DBG} HINTS ${AVFoundation_LIB_SEARCH_PATH} ${AVFoundation_PKGC_LIBRARY_DIRS})
make_library_set(AVFoundation_LIBRARY)
findpkg_finish(AVFoundation)
add_parent_dir(AVFoundation_INCLUDE_DIRS AVFoundation_INCLUDE_DIR)

