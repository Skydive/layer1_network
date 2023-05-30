{ pkgs ? import <nixpkgs> {}, system }:
let
    /*iir1 = pkgs.stdenv.mkDerivation {
        name = "iir1";
        src = pkgs.fetchFromGitHub {
            owner = "berndporr";
            repo = "iir1";
            rev = "9ef2a04ac3a44a8762b6a209c18e3bdb00394e5b";
            sha256 = "sha256-T8gl51IkZIGq+6D5ge4Kb3wm5aw7Rhphmnf6TTGwHbs=";
        };
        nativeBuildInputs = with pkgs; [ pkg-config cmake ];
    };
    ezpwd-reed-solomon = pkgs.stdenv.mkDerivation rec {
        name = "ezpwd-reed-solomon";
        src = pkgs.fetchFromGitHub {
            owner = "pjkundert";
            repo = "ezpwd-reed-solomon";
            rev = "adc9c5b2bfe98be3a74c9495e0ec37776c6d0695";
            sha256 = "sha256-Un0wpTCEHTPlposhKiYS6mAa9r6KH8wegRdCSp2mRaY=";
            deepClone = true;
        };
        nativeBuildInputs = with pkgs; [ pkg-config ];
        buildInputs = with pkgs; [
            git
            perl
            bash
            boost
            ncurses
        ];

        buildPhase = ''
            make env libraries
        '';

        installPhase = ''
            mkdir -p          $out/include
            cp -RL c++/*      $out/include
            mkdir -p          $out/lib
            mv libezpwd-*     $out/lib
        '';
    };*/
in
  pkgs.stdenv.mkDerivation {
    name = "layer1-network";
    src = ./.;

    outputs = [ "out" ];
    enableParallelBuilding = true;

    nativeBuildInputs = with pkgs; [
        pkg-config
        git
    ];

    buildInputs = (with pkgs; [
        python310

        #ezpwd-reed-solomon
        #iir1
        
        gcc
        gdb
        valgrind
        zeromq
        cppzmq

        #ezpwd-reed-solomon
    ]) ++ (with pkgs.python310Packages; [
        numpy
        scipy
        matplotlib

        bitarray
        crccheck
        pyzmq
    ]);

    propagatedBuildInputs = [
        # Is this necessary
        # pkgs.stdenv.cc.cc.lib
    ];
  }
