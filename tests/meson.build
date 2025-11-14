test_stringutils = executable('test_stringutils',
    'test_stringutils.cc',
    include_directories : [incdir, include_directories('..')],
    link_with : stringutils_lib,
)

test('stringutils', test_stringutils)

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
