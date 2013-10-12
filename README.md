Alice
=====

Alice is a simple GUI widget library, powered by OpenGL (ES) and controlled from
Lua. It currently builds and runs on OS X and the Raspberry Pi, although as it
uses SDL, it should be easy to port to standard Linux environments and Windows.


Building
--------

### OS X

You will need the following frameworks:

- SDL2 2.0 <http://www.libsdl.org/download-2.0.php>
- SDL2_image 2.0 <http://www.libsdl.org/projects/SDL_image/>
- Lua 5.2 <https://github.com/derkyjadex/Lua-Framework>

After you have installed these to `/Library/Frameworks`, you will be able to
build the Xcode project, or use scons. This should work for 10.6+, but may also
work for earlier versions.


### Raspberry Pi

Alice builds and runs on the Raspbian distribution, but should probably work on
others, as long as you have the VideoCore libraries in `/opt/vc`. Use your
favourite package management interface to install the following packages:

- scons
- liblua5.2-dev

You will need to build and install SDL2 from source <http://hg.libsdl.org/SDL>.
Raspberry Pi support was initially added in changeset e4c38f17bfad (2013-09-28),
so you will need to build from at least this version, or a newer one.

You will also need to install SDL2_image. This is not available in Wheezy, so
you need to build this from source <http://hg.libsdl.org/SDL_image/>.

Once these are installed, simply run `scons` from the root of the project in
order to build the library.


Usage
-----

For some sample code using Alice, see the Alice Demo project, 
<https://github.com/derkyjadex/alice-demo>.


Licence
-------

Alice is released under the MIT licence. It uses the "Ubuntu Mono" font, which
is covered by the Ubuntu Font Licence.

See COPYING for more details.
