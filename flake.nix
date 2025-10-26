{
  # A brief description of the project flake
  description = "Flake devshell for galaxy project in C++";

  inputs = {
    # Pinning nixpkgs to unstable for access to the latest packages
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";

    nixgl.url = "github:nix-community/nixGL";
    
    # Tool to handle cross-system configuration for flakes
    # We specify the main branch for better reliability over the default reference
    flake-utils.url = "github:numtide/flake-utils/main";

    # Compatibility layer for legacy Nix versions (often not needed for modern Nix)
    flake-compat = {
      url = "github:edolstra/flake-compat";
      flake = false;
    };
  };

  # Define the outputs of the flake, which include devShells
  outputs = {
    self,
    nixpkgs,
    flake-utils,
    nixgl,
    ...
  } @ inputs: let
    # Define custom package overrides (overlays) if needed
    overlays = [
      nixgl.overlay
    ];
  in
    # Use flake-utils to iterate over the default supported systems (x86_64-linux, aarch64-linux, etc.)
    flake-utils.lib.eachDefaultSystem (
      system: let
        # Import the Nix Package Collection for the current system and apply overlays
        pkgs = import nixpkgs {inherit overlays system;};
      in rec {
        # Define the default development environment
        devShells.default = pkgs.mkShell {

          buildInputs = with pkgs; [
            xorg.libXcursor
            xorg.libXi
            xorg.libXrandr
            alsa-lib
            udev
            pkg-config
            zlib
            icu
            libxml2
          ];
          
          # Tools required to build/develop the project (e.g., compilers, build systems)
          nativeBuildInputs = with pkgs; [
            xmake
            # Specific LLVM/Clang compiler and C++ standard library
            llvmPackages_21.libcxxClang
            lldb

          ];

          # Commands to run immediately upon entering the shell (e.g., setting environment variables or changing shell)
          shellHook = ''
            fish
          '';
        };
      }
    );
}