# WRECKAGE

A JUCE synth built for WRECKEM. [Install guide](https://austnnnnnn.github.io/wreckage/).

## Shipping

Push `main` to build Windows and macOS packages. GitHub Pages publishes the static
site from `main` / `docs` using the repository's existing Pages configuration.

The `build` workflow pins Windows Server 2022 with Visual Studio 2022, builds the
VST3 and standalone app, and runs the factory-preset render checks. It downloads
Microsoft's WebView2 bootstrapper, verifies its Authenticode signature, and bundles
it into the Inno Setup installer. Setup installs the runtime only when missing;
that step requires internet access. The CI smoke check installs, reinstalls, and
uninstalls WRECKAGE, checking the VST3 bundle, standalone app, Start menu shortcut,
and preservation of a user preset.

Both platform jobs must pass before the workflow uploads `WRECKAGE-Setup.exe` and
`WRECKAGE-mac.zip` to the `latest` release. Existing release assets are replaced
without deleting the release. The website's download URLs stay the same.

After pushing, verify that all three jobs in **Actions → build** are green and
that both download links on the install page resolve. A successful Pages deploy
alone does not mean the installer is available.

## Local builds

CMake 3.22+, a C++17 compiler, and platform development tools are required. CMake
uses a local `JUCE` checkout if present, otherwise fetches JUCE 8.0.10.

On Windows, use Visual Studio 2022 with Desktop development with C++, NuGet, and
Inno Setup 6. The exact configure, build, bootstrapper download, installer compile,
and smoke-check commands are in [.github/workflows/build.yml](.github/workflows/build.yml).
The installer script requires `installer/deps/MicrosoftEdgeWebview2Setup.exe`;
this downloaded dependency is intentionally ignored by Git.

On macOS:

```sh
cmake -B build -G Xcode
cmake --build build --config Release --target Wreckage_VST3 Wreckage_AU Wreckage_Standalone
```

Release artifacts are unsigned. Before sharing, test the installer and opening
the plugin in FL Studio on a Windows machine; the CI packaging checks do not
exercise the interactive WebView or a real DAW.
