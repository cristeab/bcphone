#-------------------------------------------------------------------
# This file is part of the CMake build system for OGRE
#     (Object-oriented Graphics Rendering Engine)
# For the latest info, see http://www.ogre3d.org/
#
# The contents of this file are placed in the public domain. Feel
# free to make use of it in any way you like.
#-------------------------------------------------------------------

# - Try to find Foundation
# Once done, this will define
#
#  Foundation_FOUND - system has Foundation
#  Foundation_INCLUDE_DIRS - the Foundation include directories
#  Foundation_LIBRARIES - link these to use Foundation

include(FindPkgMacros)
findpkg_begin(Foundation)

# construct search paths
set(Foundation_PREFIX_PATH ${Foundation_HOME} $ENV{Foundation_HOME})
create_search_paths(Foundation)
# redo search if prefix path changed
clear_if_changed(Foundation_PREFIX_PATH
  Foundation_LIBRARY_FWK
  Foundation_LIBRARY_REL
  Foundation_LIBRARY_DBG
  Foundation_INCLUDE_DIR
)

set(Foundation_LIBRARY_NAMES Foundation)
get_debug_names(Foundation_LIBRARY_NAMES)

use_pkgconfig(Foundation_PKGC Foundation)

findpkg_framework(Foundation)

find_path(Foundation_INCLUDE_DIR NAMES Foundation.h HINTS ${Foundation_INC_SEARCH_PATH} ${Foundation_PKGC_INCLUDE_DIRS} PATH_SUFFIXES Foundation)
find_library(Foundation_LIBRARY_REL NAMES ${Foundation_LIBRARY_NAMES} HINTS ${Foundation_LIB_SEARCH_PATH} ${Foundation_PKGC_LIBRARY_DIRS})
find_library(Foundation_LIBRARY_DBG NAMES ${Foundation_LIBRARY_NAMES_DBG} HINTS ${Foundation_LIB_SEARCH_PATH} ${Foundation_PKGC_LIBRARY_DIRS})
make_library_set(Foundation_LIBRARY)
findpkg_finish(Foundation)
add_parent_dir(Foundation_INCLUDE_DIRS Foundation_INCLUDE_DIR)

