                          ============================================
                          Compiling Sedona on a Raspberry Pi2 and Rpi3
                          ============================================
                          
  If you have a Raspberry Pi2 or Pi3, you can make it a Sedona Device in just a few minutes!  

  See <a href='http://www.raspberrypi.org/faqs'>http://www.raspberrypi.org</a> to find out more about the Raspberry Pi, where 
  to get one, and how to do the initial setup.

  This archive includes native support for accessing the Raspberry Pi2/Pi3 GPIO pins.
  Also it has support for BACnet and MQTT.


  1. Assumptions
  --------------
  It is assumed that the Raspberry Pi2/Pi3:

   * is running ArchLinux (may also work for other flavors of Linux)

   * has command line access, most commonly by ssh or telnet

   * has IP connectivity, and route exists to Niagara station or Sedona Workbench

   * can receive files over network (or copy to SD card)

   * is running a time service that syncs the device with network time (recommended)


  2. Required Software to compile (Raspbian or ArchLinux)
  -------------------------------------------------------
    a. python2 (not python3)
    b. Java SE 7 or latest
    c. gcc

  3. Commands (user root)
  -----------------------
    a. tar -xvzf jdk-7u60-linux-arm-vfp-sflt.tar.gz (Linux ARM v6/v7 Soft Float ABI)
    b. mv jdk1.7.0_60 /usr/lib/jvm
    c. export PATH=$PATH:/usr/lib/jvm/bin
    d. export JAVA_HOME=/usr/lib/jvm
    e. cd /root/sedonadev-RPi2/adm
    f. bash --rcfile unix/init.sh
    g. makeunixdev
    h. cd ..
    i. sedonac apps/platRpi.sax
    j. sedonac scode/platRpi.xml
    k. svm scode/platRpi.scode apps/platRpi.sab


 Once the SVM is up and running, the device should be discoverable from your Niagara station 
 or Sedona Workbench through SedonaNetwork.
 
 ------------------------------------------------------------------------------------------------------------------------------------------------------------------
 
 ﻿We need java for Sedona
> sudo apt-get update && sudo apt-get install oracle-java7-jdk
> java -version
"java version "1.7.0_60""

Next step, get Sedona up and running…
> wget http://hg.sedonadev.org/repos/sedona-1.2/archive/tip.tar.gz
> tar -xzvf tip.tar.gz
> mv ~/sedona-1-2-007b9be5f1e1 ~/sedonadev
> cd ~/sedonadev/adm
> nano compilejar.py

We need to switch to use the javac compiler instead of jikes and we need to remove the params that javac does not know about.  Update this…
# compile using jikes.exe
print "  Javac [" + srcDir + "]"
cmd = env.jikes
cmd += " +E +Pno-shadow"

  … to this …
# compile using javac
print "  Javac [" + srcDir + "]"
# ensure javac is on the path, otherwise put the full path here
cmd = "javac"

…CTRL-X and save the fileSince we are not using jikes, we need to update the init script to look for javac instead…

> nano unix/init.sh
Update the line that points to java to point to the installation directory instead of /usr/lib/jvm/jdk-7-oracle-arm-vfp-hflt
Immediately after that definition is the definition of programs that must be found in path, change jikes to javac.
CTRL-X and save the file
> bash --rcfile unix/init.sh
> makeunixdev
> cd ../
> sedonac apps/test.sax
> sedonac scode/x86-test.xml
> exit

Run Sedona in shell
>  sudo ./bin/svm scode/x86-test.scode apps/test.sab

Extract source to folder /home/pi/sedonadev/src/rpiDigitalIo
rpiDigitalIo.rar

Add RPi
Add string to /home/pi/sedona1/src/dir.xml
<target name="rpiDigitalIo" />

Add string to /home/pi/sedona1/platforms/db/tridium/generic/unix/1.2.29/.par/platformManifest.xml
<nativeKit depend="rpiDigitalIo 1.0+" />

<nativeMethod qname="datetimeStd::DateTimeServiceStd.doNow" id="9::0" />
<nativeMethod qname="datetimeStd::DateTimeServiceStd.doSetClock" id="9::1" />
<nativeMethod qname="datetimeStd::DateTimeServiceStd.doGetUtcOffset" id="9::2" />

Add string to /home/pi/sedonadev/platforms/src/generic/unix/generic-unix.xml
<nativeKit depend="rpiDigitalIo 1.0+" />
<nativeSource path="/src/rpiDigitalIo/native" />

For use Rpi GPIO kit need add string to file  /home/pi/sedonadev/scode/x86-test.xml
<!-- RPi GPIO kit -->
<depend on="rpiDigitalIo 1.2" />

Run compile
~/sedonadev/adm $
> bash --rcfile unix/init.sh
> cd ..
> sedonac scode/x86-test.xml
> exit
> cd ..
> sudo ./bin/svm scode/x86-test.scode apps/test.sab
 
 
 
 
