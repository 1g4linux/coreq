test_stringutils = executable('test_stringutils',
    'test_stringutils.cc',
    include_directories : [incdir, include_directories('..')],
    link_with : stringutils_lib,
)

test('stringutils', test_stringutils)

test_ansicolor = executable('test_ansicolor',
    'test_ansicolor.cc',
    include_directories : [incdir, include_directories('..')],
    link_with : [coreqtk_lib],
)

test('ansicolor', test_ansicolor)

test_argsreader = executable('test_argsreader',
    'test_argsreader.cc',
    include_directories : [incdir, include_directories('..')],
    link_with : [coreqtk_lib],
)

test('argsreader', test_argsreader)

test_compare = executable('test_compare',
    'test_compare.cc',
    include_directories : [incdir, include_directories('..')],
    link_with : stringutils_lib,
)

test('compare', test_compare)

test_filenames = executable('test_filenames',
    'test_filenames.cc',
    include_directories : [incdir, include_directories('..')],
    link_with : [filenames_lib, sysutils_lib],
)

test('filenames', test_filenames)

test_formated = executable('test_formated',
    'test_formated.cc',
    include_directories : [incdir, include_directories('..')],
    link_with : stringutils_lib,
)

test('formated', test_formated)

test_md5 = executable('test_md5',
    'test_md5.cc',
    join_paths('..', 'src', 'coreqTk', 'md5.cc'),
    include_directories : [incdir, include_directories('..')],
)

test('md5', test_md5)

test_outputstring = executable('test_outputstring',
    'test_outputstring.cc',
    join_paths('..', 'src', 'coreqTk', 'outputstring.cc'),
    include_directories : [incdir, include_directories('..')],
    link_with : stringutils_lib,
)

test('outputstring', test_outputstring)

test_parseerror = executable('test_parseerror',
    'test_parseerror.cc',
    include_directories : [incdir, include_directories('..')],
    link_with : [parseerror_lib, stringutils_lib],
)

test('parseerror', test_parseerror)

test_percentage = executable('test_percentage',
    'test_percentage.cc',
    join_paths('..', 'src', 'coreqTk', 'percentage.cc'),
    include_directories : [incdir, include_directories('..')],
    link_with : stringutils_lib,
)

test('percentage', test_percentage)

test_regexp = executable('test_regexp',
    'test_regexp.cc',
    include_directories : [incdir, include_directories('..')],
    link_with : [coreqtk_lib],
)

test('regexp', test_regexp)

test_stringlist = executable('test_stringlist',
    'test_stringlist.cc',
    join_paths('..', 'src', 'coreqTk', 'stringlist.cc'),
    join_paths('..', 'src', 'coreqTk', 'stringlist_output.cc'),
    join_paths('..', 'src', 'coreqTk', 'outputstring.cc'),
    include_directories : [incdir, include_directories('..')],
    link_with : stringutils_lib,
)

test('stringlist', test_stringlist)

test_sysutils = executable('test_sysutils',
    'test_sysutils.cc',
    include_directories : [incdir, include_directories('..')],
    link_with : [sysutils_lib],
)

test('sysutils', test_sysutils)

test_varsreader = executable('test_varsreader',
    'test_varsreader.cc',
    include_directories : [incdir, include_directories('..')],
    link_with : [coreqtk_lib],
)

test('varsreader', test_varsreader)

test_version = executable('test_version',
    'test_version.cc',
    include_directories : [incdir, include_directories('..')],
    link_with : [basicversion_lib, stringutils_lib],
)

test('version', test_version)

test_package = executable('test_package',
    'test_package.cc',
    include_directories : [incdir, include_directories('..')],
    link_with : [corepkg_lib, version_lib, basicversion_lib, stringutils_lib],
)

test('package', test_package)

test_depend = executable('test_depend',
    'test_depend.cc',
    include_directories : [incdir, include_directories('..')],
    link_with : [depend_lib, stringutils_lib],
)

test('depend', test_depend)

test_eapi = executable('test_eapi',
    'test_eapi.cc',
    include_directories : [incdir, include_directories('..')],
    link_with : [corepkg_lib, stringutils_lib],
)

test('eapi', test_eapi)

test_extendedversion = executable('test_extendedversion',
    'test_extendedversion.cc',
    include_directories : [incdir, include_directories('..')],
    link_with : [corepkg_lib, version_lib, basicversion_lib, stringutils_lib],
)

test('extendedversion', test_extendedversion)

test_instversion = executable('test_instversion',
    'test_instversion.cc',
    include_directories : [incdir, include_directories('..')],
    link_with : [corepkg_lib, version_lib, basicversion_lib, stringutils_lib],
)

test('instversion', test_instversion)

test_keywords = executable('test_keywords',
    'test_keywords.cc',
    include_directories : [incdir, include_directories('..')],
    link_with : [corepkg_lib, stringutils_lib],
)

test('keywords', test_keywords)

test_overlay = executable('test_overlay',
    'test_overlay.cc',
    include_directories : [incdir, include_directories('..')],
    link_with : [corepkg_lib, coreqtk_lib, filenames_lib, sysutils_lib, stringutils_lib],
)

test('overlay', test_overlay)

test_packagesets = executable('test_packagesets',
    'test_packagesets.cc',
    include_directories : [incdir, include_directories('..')],
    link_with : [corepkg_lib],
)

test('packagesets', test_packagesets)

test_packagetree = executable('test_packagetree',
    'test_packagetree.cc',
    include_directories : [incdir, include_directories('..')],
    link_with : [corepkg_lib, version_lib, basicversion_lib, stringutils_lib],
)

test('packagetree', test_packagetree)

test_vardbpkg = executable('test_vardbpkg',
    'test_vardbpkg.cc',
    include_directories : [incdir, include_directories('..')],
    link_with : [corepkg_lib, coreqtk_lib, sysutils_lib, stringutils_lib, version_lib, basicversion_lib],
)

test('vardbpkg', test_vardbpkg)

test_db_header = executable('test_db_header',
    'test_db_header.cc',
    include_directories : [incdir, include_directories('..')],
    link_with : [header_lib, corepkg_lib, coreqtk_lib, filenames_lib, stringutils_lib],
)

test('db_header', test_db_header)

test_cache_base = executable('test_cache_base',
    'test_cache_base.cc',
    include_directories : [incdir, include_directories('..')],
    link_with : [cache_lib, corepkg_lib, coreqtk_lib, filenames_lib, sysutils_lib, stringutils_lib],
)

test('cache_base', test_cache_base)

test('eapi_no_heap_statics',
    sh,
    args : [
        files('test_eapi_no_heap_statics.sh'),
        join_paths(meson.current_source_dir(), '..', 'src', 'corepkg', 'eapi.cc'),
    ],
)

test('ebuild_exec_no_heap_statics',
    sh,
    args : [
        files('test_ebuild_exec_no_heap_statics.sh'),
        join_paths(meson.current_source_dir(), '..', 'src', 'cache', 'common', 'ebuild_exec.cc'),
    ],
)

test('corepkgsettings_no_heap_statics',
    sh,
    args : [
        files('test_corepkgsettings_no_heap_statics.sh'),
        join_paths(meson.current_source_dir(), '..', 'src', 'corepkg', 'conf', 'corepkgsettings.cc'),
    ],
)

test('mask_list_no_heap_statics',
    sh,
    args : [
        files('test_mask_list_no_heap_statics.sh'),
        join_paths(meson.current_source_dir(), '..', 'src', 'corepkg', 'mask_list.cc'),
    ],
)

test('coreqcache_no_heap_statics',
    sh,
    args : [
        files('test_coreqcache_no_heap_statics.sh'),
        join_paths(meson.current_source_dir(), '..', 'src', 'cache', 'coreqcache', 'coreqcache.cc'),
    ],
)

test('ebuild_exec_safe_buffers',
    sh,
    args : [
        files('test_ebuild_exec_safe_buffers.sh'),
        join_paths(meson.current_source_dir(), '..', 'src', 'cache', 'common', 'ebuild_exec.cc'),
    ],
)

test('q_dispatch',
    sh,
    args : [
        files('test_q_dispatch.sh'),
        join_paths(meson.current_build_dir(), '..', 'q'),
    ],
)
