{
  description = "FZ15ESPConnected firmware development environment";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs =
    { nixpkgs, ... }:
    let
      supportedSystems = [
        "x86_64-linux"
        "aarch64-linux"
      ];
      forAllSystems = nixpkgs.lib.genAttrs supportedSystems;
      pkgsFor = system: import nixpkgs { inherit system; };

      mkPioApp =
        pkgs: name: command: description:
        let
          package = pkgs.writeShellApplication {
            name = "fz15-${name}";
            runtimeInputs = [ pkgs.platformio ];
            text = ''
              exec pio ${command} "$@"
            '';
          };
        in
        {
          type = "app";
          program = "${package}/bin/fz15-${name}";
          meta = { inherit description; };
        };
    in
    {
      devShells = forAllSystems (
        system:
        let
          pkgs = pkgsFor system;
        in
        {
          default = pkgs.mkShellNoCC {
            packages = with pkgs; [
              clang-tools
              cmake
              git
              ninja
              platformio
            ];
          };
        }
      );

      apps = forAllSystems (
        system:
        let
          pkgs = pkgsFor system;
        in
        {
          build = mkPioApp pkgs "build" "run" "Build the ESP32 firmware";
          upload = mkPioApp pkgs "upload" "run --target upload" "Build and upload the ESP32 firmware";
          monitor = mkPioApp pkgs "monitor" "device monitor" "Open the ESP32 serial monitor";
          default = mkPioApp pkgs "build" "run" "Build the ESP32 firmware";
        }
      );

      formatter = forAllSystems (system: (pkgsFor system).nixfmt);
    };
}
