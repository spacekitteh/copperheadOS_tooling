{ pkgs ? import <nixpkgs> {}, stdenv ? 
pkgs.stdenv, fetchgit ? pkgs.fetchgit}:

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
    '';
  };
in   #fhs.env 
 stdenv.mkDerivation { 
  name="test-shell";
  nativeBuildInputs = [fhs];
  buildType = "release";
  repoSrc = fetchgit {
    url = https://github.com/spacekitteh/git-repo.git;
    rev = "abb5aac8eace33ff26dd376375a60be8d0096275";
    sha256 = "123cgay20cd5jsgmjcp1s6srh8gqq194ydqgjb3cvm3yqs14qmaf";
  };
  localManifests = fetchgit {
    url = "https://github.com/spacekitteh/copperheadOS_local_manifests";
    rev = "0f59342308cc444c80a7d389b2cc700ee63b6cd4";
    sha256 = "1yvj1yzhdfami093ms649k513yknz3c7vg8x2srwgkyin971x31i";
  };
#  copperheadOSsrc = "copperhead-os";
  userType = "user";
  buildTag = "N4F26T.2017.03.13.18.50.01";
  model = "marlin"; ### TODO
  builder = ./builder.sh;
}

