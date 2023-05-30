{
  description = "Layer 1 network testing example";
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
    flake-utils.inputs.nixpkgs.follows = "nixpkgs";
  };

  outputs = { self, nixpkgs, flake-utils, ... }:
    flake-utils.lib.eachDefaultSystem (system:
    let
      pkgs = import nixpkgs { inherit system; };
      layer1 = pkgs.callPackage ./default.nix { inherit pkgs; };
    in {
      devShell = layer1;
      defaultPackage = layer1;
    });
}
