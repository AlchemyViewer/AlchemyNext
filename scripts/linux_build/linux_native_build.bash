#!/bin/bash

do_build() {
    # TODO: build inside a chroot?
    cd "${pkgname}" || exit 1
    virtualenv ".venv" -p python3
    source ".venv/bin/activate"
    if command -v autobuild; then
        abver="$(autobuild --version)"
        echo "Found ${abver}"
        if [ "${abver}" = "autobuild 2.1.0" ]; then
            echo "Reinstalling autobuild to work around some bugs"
            pip3 uninstall --yes autobuild
        fi
    fi
    pip3 install --upgrade autobuild -i https://git.alchemyviewer.org/api/v4/projects/54/packages/pypi/simple --extra-index-url https://pypi.org/simple
    # we have a lot of files, relax ulimit to help performance
    ulimit -n 20000
    # shellcheck disable=SC2153
    autobuild configure -A 64 -c ReleaseOS -- -DLL_TESTS:BOOL=OFF -DDISABLE_FATAL_WARNINGS=ON -DUSE_LTO:BOOL="$(grep -cq '[^!]lto' <<< "${OPTIONS}" && echo 'ON' || echo 'OFF')" -DVIEWER_CHANNEL="Alchemy Test" ${AL_EXTRA_CMAKE_FLAGS}
    cd "build-linux-64" || exit
    ninja -j"$(nproc)"
}
if [[ -z "${IS_PKGBUILD}" ]]
then
    if command -v pacman > /dev/null 2>&1; then
        package_list="dbus-glib glu gtk3 libgl libidn libjpeg-turbo libpng libxss libxml2 mesa nss openal sdl2 vlc zlib cmake gcc python-virtualenv python-pip git boost xz ninja sed"
        #shellcheck disable=2086
        if command -v pacman > /dev/null 2>&1; then
            # work around quoting weirdness
            echo "$package_list" | xargs pacman -Qq ${package_list} > /dev/null || pkexec pacman --sync --needed ${package_list}
        fi
    else
        eval "$(</etc/lsb-release)"
        echo "Distribution \'${DISTRIB_DESCRIPTION}\' is not currently supported by the native build script. Merge requests are welcome."
    fi
fi
do_build;
