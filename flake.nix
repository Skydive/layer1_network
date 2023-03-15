{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    flake-utils.inputs.nixpkgs.follows = "nixpkgs";
    mach-nix.url = "github:DavHau/mach-nix/3.5.0";
  };

  outputs = { self, nixpkgs, flake-utils, mach-nix, ... }:
    flake-utils.lib.eachDefaultSystem (system:
    let
      python = "python310";
      pkgs = import nixpkgs { inherit system; };
      mach-nix-wrapper = import mach-nix { inherit pkgs; };
      requirements = builtins.readFile ./requirements.txt;
      providers = { _default = "nixpkgs,wheel"; };
      pythonBuild = mach-nix-wrapper.mkPython {
        inherit requirements providers;
      };

    in {
      devShell = pkgs.mkShell {
          nativeBuildInputs = with pkgs; [
            pkg-config
          ];

          buildInputs = with pkgs; [
            pythonBuild
            
            gcc
            gdb
            valgrind
            zeromq
            cppzmq

            #ezpwd-reed-solomon
          ];

          propagatedBuildInputs = [
            pkgs.stdenv.cc.cc.lib
          ];
        };
    });
}
