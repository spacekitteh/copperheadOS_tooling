{ pkgs ? import <nixpkgs> {}, stdenv ? 
pkgs.stdenv}:

let
   jdk = pkgs.jdk8.overrideDerivation (attrs: {
      enableGnome2 = false;
      NIX_LDFLAGS="";}); 

    androidsdk = pkgs.androidenv.androidsdk {
	platformVersions = [ "25" ];
	abiVersions = ["armv8a"];
	useGoogleAPIs = false;
    };
      
  current-repo = "nougat";

     wantedPkgs =  pkgs: with pkgs;
      [ git
        gitRepo
        python2
	bash-completion
        curl
	autoconf
#	expat.dev
        procps
        libressl
        gnumake
        patch
        binutils
#        nettools
        rsync
        androidenv.platformTools
        androidsdk
        androidenv.buildTools
        androidenv.platforms.platform_25
        ncurses5
        jdk
        schedtool
        utillinux
	gradle
        m4
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
   ];
  fhs = pkgs.buildFHSUserEnv {
    name = "android-env";
    targetPkgs = wantedPkgs;
    multiPkgs = pkgs: with pkgs;
      [ zlib
      ];
    extraOutputsToInstall = [ "dev" ];

    profile = ''

      export ANDROID_JAVA_HOME=${jdk.home}
#      export ANDROID_HOME=./.android
      export LANG=C
      unset _JAVA_OPTIONS
      unset JAVA_HOME
      export BUILD_NUMBER=$(date --utc +%Y.%m.%d.%H.%M.%S)
      export DISPLAY_BUILD_NUMBER=true
      export USER=copperhead-os-build
      export USE_CCACHE=1
      export CCACHE_HARDLINK=1
      chrt -b -p 0 $$
#      source ./nougat/build/envsetup.sh
    '';
  };
in   #fhs.env 
 stdenv.mkDerivation { 
  name="test-shell";
  nativeBuildInputs = [fhs];
  builder = ./builder.sh;
}

