{
  description = "Robotica ESP32 calendar";

  inputs.flake-utils.url = "github:numtide/flake-utils";
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-23.05";

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let pkgs = nixpkgs.legacyPackages.${system};

      in {

        devShells.default =
          pkgs.mkShell { packages = with pkgs; [ platformio ]; };
      });
}
