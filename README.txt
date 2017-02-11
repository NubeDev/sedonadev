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
