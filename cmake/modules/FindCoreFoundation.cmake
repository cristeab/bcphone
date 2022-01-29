#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# - Try to find CoreFoundation
# Once done, this will define
#
#  CoreFoundation_FOUND - system has CoreFoundation
#  CoreFoundation_INCLUDE_DIRS - the CoreFoundation include directories 
#  CoreFoundation_LIBRARIES - link these to use CoreFoundation

include(FindPkgMacros)
findpkg_begin(CoreFoundation)

# construct search paths
set(CoreFoundation_PREFIX_PATH ${CoreFoundation_HOME} $ENV{CoreFoundation_HOME})
create_search_paths(CoreFoundation)
# redo search if prefix path changed
clear_if_changed(CoreFoundation_PREFIX_PATH
  CoreFoundation_LIBRARY_FWK
  CoreFoundation_LIBRARY_REL
  CoreFoundation_LIBRARY_DBG
  CoreFoundation_INCLUDE_DIR
)

set(CoreFoundation_LIBRARY_NAMES CoreFoundation)
get_debug_names(CoreFoundation_LIBRARY_NAMES)

use_pkgconfig(CoreFoundation_PKGC CoreFoundation)

findpkg_framework(CoreFoundation)

find_path(CoreFoundation_INCLUDE_DIR NAMES CoreFoundation.h HINTS ${CoreFoundation_INC_SEARCH_PATH} ${CoreFoundation_PKGC_INCLUDE_DIRS} PATH_SUFFIXES CoreFoundation)
find_library(CoreFoundation_LIBRARY_REL NAMES ${CoreFoundation_LIBRARY_NAMES} HINTS ${CoreFoundation_LIB_SEARCH_PATH} ${CoreFoundation_PKGC_LIBRARY_DIRS})
find_library(CoreFoundation_LIBRARY_DBG NAMES ${CoreFoundation_LIBRARY_NAMES_DBG} HINTS ${CoreFoundation_LIB_SEARCH_PATH} ${CoreFoundation_PKGC_LIBRARY_DIRS})
make_library_set(CoreFoundation_LIBRARY)
findpkg_finish(CoreFoundation)
add_parent_dir(CoreFoundation_INCLUDE_DIRS CoreFoundation_INCLUDE_DIR)

