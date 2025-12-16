{
  description = "C++ Game of Life Multiverse";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let pkgs = nixpkgs.legacyPackages.${system};
      in {
        devShells.default = pkgs.mkShell {
          buildInputs = with pkgs; [ gcc gnumake ];

          shellHook = ''
            echo "C++ development environment loaded"
            echo "Compile with: g++ -std=c++17 -O2 main.cpp -o multiverse"
            echo "Run with: ./multiverse"
          '';
        };

        packages.default = pkgs.stdenv.mkDerivation {
          pname = "multiverse";
          version = "0.1.0";

          src = ./.;

          buildInputs = with pkgs; [ gcc ];

          buildPhase = ''
            g++ -std=c++17 -O2 main.cpp -o multiverse
          '';

          installPhase = ''
            mkdir -p $out/bin
            cp multiverse $out/bin/
          '';
        };
      });
}
