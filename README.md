screcord
========

a simple Lv2 capture plugin
    Include a mono and a stereo capture plugin.
    using libsndfile <http://www.mega-nerd.com/libsndfile/>
    save audio stream as wav or ogg files to 
    ~/lv2record/lv2_sessionX.wav/ogg
    were X is replaced by numbers
    No Gui, the host need to provide the UI.

build:
    no build dependency check, just 
    make
    libsndfile is needed

install:
    as user make install will install to ~./lv2/
    as root make install will install to /usr/lib/lv2/
