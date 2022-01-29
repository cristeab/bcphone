#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# - Try to find CoreMedia
# Once done, this will define
#
#  CoreMedia_FOUND - system has CoreMedia
#  CoreMedia_INCLUDE_DIRS - the CoreMedia include directories
#  CoreMedia_LIBRARIES - link these to use CoreMedia

include(FindPkgMacros)
findpkg_begin(CoreMedia)

# construct search paths
set(CoreMedia_PREFIX_PATH ${CoreMedia_HOME} $ENV{CoreMedia_HOME})
create_search_paths(CoreMedia)
# redo search if prefix path changed
clear_if_changed(CoreMedia_PREFIX_PATH
  CoreMedia_LIBRARY_FWK
  CoreMedia_LIBRARY_REL
  CoreMedia_LIBRARY_DBG
  CoreMedia_INCLUDE_DIR
)

set(CoreMedia_LIBRARY_NAMES CoreMedia)
get_debug_names(CoreMedia_LIBRARY_NAMES)

use_pkgconfig(CoreMedia_PKGC CoreMedia)

findpkg_framework(CoreMedia)

find_path(CoreMedia_INCLUDE_DIR NAMES CoreMedia.h HINTS ${CoreMedia_INC_SEARCH_PATH} ${CoreMedia_PKGC_INCLUDE_DIRS} PATH_SUFFIXES CoreMedia)
find_library(CoreMedia_LIBRARY_REL NAMES ${CoreMedia_LIBRARY_NAMES} HINTS ${CoreMedia_LIB_SEARCH_PATH} ${CoreMedia_PKGC_LIBRARY_DIRS})
find_library(CoreMedia_LIBRARY_DBG NAMES ${CoreMedia_LIBRARY_NAMES_DBG} HINTS ${CoreMedia_LIB_SEARCH_PATH} ${CoreMedia_PKGC_LIBRARY_DIRS})
make_library_set(CoreMedia_LIBRARY)
findpkg_finish(CoreMedia)
add_parent_dir(CoreMedia_INCLUDE_DIRS CoreMedia_INCLUDE_DIR)

