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
    in
    {
      packages.${system}.default = llvm.stdenv.mkDerivation {
        pname = "greenline";
        version = "0.0.1";

        src = ./.;

        nativeBuildInputs = with pkgs; [
          addDriverRunpath
          cmake
          ninja
          pkg-config
        ];

        buildInputs = with pkgs; [
          cudaPackages.cuda_nvml_dev
          (linuxPackages.nvidia_x11.override { libsOnly = true; })
        ];

        # Rewrite RPATH so libnvidia-ml.so resolves to the driver's library at runtime.
        postFixup = "addDriverRunpath $out/bin/greenline";
      };

      devShells.${system}.default = pkgs.mkShell.override { stdenv = llvm.stdenv; } {
        inputsFrom = [ self.packages.${system}.default ];
        packages = [
          llvm.clang-tools
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
