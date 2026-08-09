{
  inputs.nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

  outputs =
    { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs {
        inherit system;
        config.allowUnfree = true;
      };
      llvm = pkgs.llvmPackages_22;

      mkPackage =
        stdenv:
        stdenv.mkDerivation {
          pname = "greenline";
          version = pkgs.lib.trim (builtins.readFile ./VERSION);

          src = ./.;

          nativeBuildInputs = [
            pkgs.addDriverRunpath
            pkgs.cmake
            pkgs.ninja
            pkgs.pkg-config
          ];

          buildInputs = [
            (pkgs.linuxPackages.nvidia_x11.override { libsOnly = true; })
            pkgs.cudaPackages.cuda_nvml_dev
            pkgs.ncurses
          ];

          # Rewrite RPATH so libnvidia-ml.so resolves to the driver's library at runtime.
          postFixup = "addDriverRunpath $out/bin/greenline";
        };
    in
    {
      packages.${system} = {
        default = mkPackage llvm.stdenv;
        gccCheck = mkPackage pkgs.gcc16Stdenv;
      };

      devShells.${system}.default = pkgs.mkShell.override { stdenv = llvm.stdenv; } {
        inputsFrom = [ self.packages.${system}.default ];
        packages = [
          llvm.clang-tools
          llvm.llvm
          llvm.lldb
        ];

        shellHook = ''
          export CMAKE_BUILD_TYPE=Debug
          # Point the dynamic linker to the NixOS NVIDIA driver library path.
          export LD_LIBRARY_PATH=/run/opengl-driver/lib
        '';
      };
    };
}
