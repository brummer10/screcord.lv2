<h1>screcord</h1>

<p>a simple Lv2 capture plugin</p>
-    Include a mono and a stereo capture plugin.
-    using libsndfile <a href="http://www.mega-nerd.com/libsndfile/">http://www.mega-nerd.com/libsndfile/</a>
-    save audio stream as wav or ogg files to ~/lv2record/lv2_sessionX.wav/ogg were X is replaced by numbers
-    No Gui, the host need to provide the UI.

<p>build:</p>
-    no build dependency check, just make
-    libsndfile is needed

<p>install:</p>
-    as user make install will install to ~./lv2/
-    as root make install will install to /usr/lib/lv2/
