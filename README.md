stegparty
=========
_StegParty is a system for hiding information inside of plain-text files. Unlike similar tools currently available it does not use random gibberish to encode data -- it relies on small alterations to the message, like changes to spelling and punctuation. Because of this you can use any plain-text file as your carrier, and it will be more-or-less understandable after the secret message is embedded._

This is the original StegParty program created in 1999 by Steven Hugg. Unfortunately, it no longer compiles on newer systems. 
I took the time to fix a few trivial syntax changes and complete the Makefile and it compiles fine now. License is GPLv2.

To install it on Ubuntu:
- sudo apt-get install build-essential git make
- git clone https://github.com/countrygeek/stegparty.git
- cd stegparty
- make
- make install

To install it on Arch:
- sudo pacman -S base-devel git make
- git clone https://github.com/countrygeek/stegparty.git
- cd stegparty
- make
- make install

Have fun!
