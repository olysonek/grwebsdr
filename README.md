GrWebSDR
========

GrWebSDR is a web SDR receiver.

Build requirements
------------------
On Fedora, you will need the following packages:
* gnuradio-devel
* boost-devel
* libvorbis-devel
* libogg-devel
* libwebsockets-devel
* json-c-devel
* gr-osmosdr-devel
* openssl-devel

Build instructions
------------------
```
$ autoreconf --install
$ ./configure
$ make
```

How to run it
-------------
```
$ cd src/cpp
$ ./grwebsdr
```
You will be asked to enter some parameters for your tuner. If you don't want
to fill in the parameters each time you run the server, you can create
a configuration file (see the file `sample_config.json`) and run it as
```
$ ./grwebsdr -f <config file>
```

You will also be asked to enter a new admin user name + password for the web UI.

Now visit http://localhost:8080/ in your browser.

For more options see the output of
```
$ ./grwebsdr -h
```

Known bugs
----------
1. It takes a while for the browser to start playing (initially and when changing
   the demodulation or frequency of the receiver). I believe this is due to
   browser-side buffering of the audio stream.
2. When using a RTL-SDR tuner then sometimes upon loading the web UI, or when
   changing some receiver parameter, the server hangs, printing 'O' characters
   to stderr. This is caused by a race condition originating in rtl-sdr and
   gr-osmosdr. I'm working on the fix.

License
-------
Copyright (C) 2017 Ondřej Lysoněk

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program (see the file COPYING).  If not,
see <http://www.gnu.org/licenses/>.
