# ZeVA - Central Administration and Automation
From the German **Ze**ntrale **V**erwaltung und **A**utomatisierung.

## Overview
ZeVA (or ZeVa) is kind of like an IoT platform written in C++ to tie 
your microcontrollers and other computers together. You install it 
on a server like a Raspberry Pi and access it through a browser.

### Features
* Scripting system using the *awesome* scripting language 
[wren](https://github.com/wren-lang/wren):
  * Write scripts directly in the browser.
  * React to events from **all parts** of ZeVA with scripts.
  * Each script runs in a separate process which allows timeouts & crashes.
* Write data from your IoT sensors directly to the database.
  * Display graphs from database tables.
* Builtin time-tracker used for automating your day-to-day life.
* Very basic music player & SFTP client.
* Browser extension for tracking site usage.
* A small utility for tracking system usage statistics.
* A central hub for all your other servers/platforms (WIP).

## Use-Case

I built this project as a way to automate my home network and to have
a central system that all my systems can interface with. I use it to
control my wireless-controlled plugs, my self-built clock, my server-led,
my server-hard drives, my speakers, log my system statistics, play music
and react accordingly etc. For example, it allows me to turn off every 
electronic device & led as soon as I start the sleeping-timer in the
time tracker.

The primary feature of ZeVA is the ability to react to anything using
script callbacks. The scripts are written in wren, a classy little 
scripting language made for embedding. It's very easy to learn and use.

## Getting Started / Documentation

Sadly, I haven't written a lot of documentation yet. Compilation should
be very self-explanatory. The server operates on port 8080; make sure
to configure assets/config.json (or create assets/config.local).

That said, I have to admit that at its current stage 
this is a very personal project and it only
contains features that I need. I'd be very happy if this is of use to you,
but I can totally understand that it probably isn't. If you've got any
questions or feature requests, feel free to open an issue or reach me via 
vayudev@protonmail.com (I try to look in there from time to time).

## ZeVA wouldn't have been possible without theses other *awesome* projects
A huge thank you to:
* The web framework [drogon](https://github.com/an-tao/drogon) 
* The scripting language [wren](https://github.com/wren-lang/wren)
* The http library [cpp-httplib](https://github.com/yhirose/cpp-httplib)
* The image encoder [stb_image_write](https://github.com/nothings/stb)
* The frontend table 
[csv-to-html-table](https://github.com/derekeder/csv-to-html-table.git)

and all their contributors.
