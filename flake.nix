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
          buildInputs = with pkgs; [ gcc gnumake sqlite ];

          shellHook = ''
            echo "C++ development environment loaded"
            echo "Compile with: g++ -std=c++17 -O2 main.cpp board_id.cpp ruleset_id.cpp database.cpp -lsqlite3 -o multiverse"
            echo "Run with: ./multiverse"
          '';
        };
      });
}
