#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# - Try to find CoreAudio
# Once done, this will define
#
#  CoreAudio_FOUND - system has CoreAudio
#  CoreAudio_INCLUDE_DIRS - the CoreAudio include directories
#  CoreAudio_LIBRARIES - link these to use CoreAudio

include(FindPkgMacros)
findpkg_begin(CoreAudio)

# construct search paths
set(CoreAudio_PREFIX_PATH ${CoreAudio_HOME} $ENV{CoreAudio_HOME})
create_search_paths(CoreAudio)
# redo search if prefix path changed
clear_if_changed(CoreAudio_PREFIX_PATH
  CoreAudio_LIBRARY_FWK
  CoreAudio_LIBRARY_REL
  CoreAudio_LIBRARY_DBG
  CoreAudio_INCLUDE_DIR
)

set(CoreAudio_LIBRARY_NAMES CoreAudio)
get_debug_names(CoreAudio_LIBRARY_NAMES)

use_pkgconfig(CoreAudio_PKGC CoreAudio)

findpkg_framework(CoreAudio)

find_path(CoreAudio_INCLUDE_DIR NAMES CoreAudio.h HINTS ${CoreAudio_INC_SEARCH_PATH} ${CoreAudio_PKGC_INCLUDE_DIRS} PATH_SUFFIXES CoreAudio)
find_library(CoreAudio_LIBRARY_REL NAMES ${CoreAudio_LIBRARY_NAMES} HINTS ${CoreAudio_LIB_SEARCH_PATH} ${CoreAudio_PKGC_LIBRARY_DIRS})
find_library(CoreAudio_LIBRARY_DBG NAMES ${CoreAudio_LIBRARY_NAMES_DBG} HINTS ${CoreAudio_LIB_SEARCH_PATH} ${CoreAudio_PKGC_LIBRARY_DIRS})
make_library_set(CoreAudio_LIBRARY)
findpkg_finish(CoreAudio)
add_parent_dir(CoreAudio_INCLUDE_DIRS CoreAudio_INCLUDE_DIR)

