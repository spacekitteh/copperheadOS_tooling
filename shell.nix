{ pkgs ? import <nixpkgs> {}, pkgs_i686 ?  pkgs.pkgsi686Linux }:

let
   jdk = pkgs.jdk8.overrideDerivation (attrs: {
      enableGnome2 = false;
      NIX_LDFLAGS="";}); 

    androidsdk = pkgs.androidenv.androidsdk {platformVersions = [ "25" ]; abiVersions = ["armv8a"]; useGoogleAPIs = false;};
      
  current-repo = "nougat";

     wantedPkgs =  pkgs: with pkgs;
      [ git
        gitRepo
        gnupg1compat
        python2
        tmux
        curl
	autoconf
        strace
        file
        valgrind
        cscope        
        clang
        lldb
	expat.dev
        procps
        nano
        libressl
        gnumake
        patch
        binutils
        nettools
        rsync
        androidenv.platformTools
        androidsdk
        androidenv.buildTools
        androidenv.androidndk
#        androidenv.supportRepository
        androidenv.platforms.platform_25
        ncurses5
        jdk
        schedtool
        utillinux
        m4
        apktool
        gperf
        perl	
        libxml2
        zip
        unzip
        bison
        flex
        pythonPackages.pyflakes
        pythonPackages.flake8
        lzop
        bc
        which
        gcc
        gdb
   ];
  fhs = pkgs.buildFHSUserEnv {
    name = "android-env";
    targetPkgs = wantedPkgs;
    multiPkgs = pkgs: with pkgs;
      [ zlib
      ];
    extraOutputsToInstall = [ "dev" ];
/*    extraInstallCommands = ''
      cd ${current-repo}
      source build/envsetup.sh
      chrt -b -p 0 $$
      choosecombo release aosp_marlin user
      make target-files-package -j3
    ''; */

    profile = ''
      export ANDROID_JAVA_HOME=${jdk.home}
      export ANDROID_HOME=/home/spacekitteh/.android
      export LANG=C
      unset _JAVA_OPTIONS
      unset JAVA_HOME
      export BUILD_NUMBER=$(date --utc +%Y.%m.%d.%H.%M.%S)
      export DISPLAY_BUILD_NUMBER=true
      export USER=copperhead-os-build
      chrt -b -p 0 $$
    '';
  };
in fhs.env 
