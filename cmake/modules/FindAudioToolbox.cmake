#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# - Try to find AudioToolbox
# Once done, this will define
#
#  AudioToolbox_FOUND - system has AudioToolbox
#  AudioToolbox_INCLUDE_DIRS - the AudioToolbox include directories
#  AudioToolbox_LIBRARIES - link these to use AudioToolbox

include(FindPkgMacros)
findpkg_begin(AudioToolbox)

# construct search paths
set(AudioToolbox_PREFIX_PATH ${AudioToolbox_HOME} $ENV{AudioToolbox_HOME})
create_search_paths(AudioToolbox)
# redo search if prefix path changed
clear_if_changed(AudioToolbox_PREFIX_PATH
  AudioToolbox_LIBRARY_FWK
  AudioToolbox_LIBRARY_REL
  AudioToolbox_LIBRARY_DBG
  AudioToolbox_INCLUDE_DIR
)

set(AudioToolbox_LIBRARY_NAMES AudioToolbox)
get_debug_names(AudioToolbox_LIBRARY_NAMES)

use_pkgconfig(AudioToolbox_PKGC AudioToolbox)

findpkg_framework(AudioToolbox)

find_path(AudioToolbox_INCLUDE_DIR NAMES AudioToolbox.h HINTS ${AudioToolbox_INC_SEARCH_PATH} ${AudioToolbox_PKGC_INCLUDE_DIRS} PATH_SUFFIXES AudioToolbox)
find_library(AudioToolbox_LIBRARY_REL NAMES ${AudioToolbox_LIBRARY_NAMES} HINTS ${AudioToolbox_LIB_SEARCH_PATH} ${AudioToolbox_PKGC_LIBRARY_DIRS})
find_library(AudioToolbox_LIBRARY_DBG NAMES ${AudioToolbox_LIBRARY_NAMES_DBG} HINTS ${AudioToolbox_LIB_SEARCH_PATH} ${AudioToolbox_PKGC_LIBRARY_DIRS})
make_library_set(AudioToolbox_LIBRARY)
findpkg_finish(AudioToolbox)
add_parent_dir(AudioToolbox_INCLUDE_DIRS AudioToolbox_INCLUDE_DIR)

