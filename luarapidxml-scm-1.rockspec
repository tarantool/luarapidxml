package = 'luarapidxml'
version = 'scm-1'
source  = {
    url    = 'git+ssh://git@gitlab.com:tarantool/ib-core/luarapidxml.git';
    branch = 'master';
}

description = {
    summary  = "Fast XML parsing module for Tarantool";
    homepage = 'https://gitlab.com/tarantool/watchdog',
    maintainer = "Yaroslav Dynnikov <yaroslav.dynnikov@gmail.com>",
    license  = 'BSD2'
}

dependencies = {
    'tarantool',
    'lua == 5.1'
}

external_dependencies = {
    TARANTOOL = {
        header = 'tarantool/module.h';
    };
}

build = {
    type = 'cmake';
    variables = {
        CMAKE_BUILD_TYPE="RelWithDebInfo";
        TARANTOOL_DIR="$(TARANTOOL_DIR)";
        TARANTOOL_INSTALL_LIBDIR="$(LIBDIR)";
        TARANTOOL_INSTALL_LUADIR="$(LUADIR)";
    };
}
