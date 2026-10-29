$ErrorActionPreference = 'Stop'

function Run-Setup([string] $Path, [string[]] $Arguments) {
    $process = Start-Process -FilePath $Path -ArgumentList $Arguments -Wait -PassThru
    if ($process.ExitCode -ne 0) { throw "$Path exited with $($process.ExitCode)" }
}

$app = Join-Path $env:ProgramFiles 'WRECKAGE'
$plugin = Join-Path $env:CommonProgramFiles 'VST3/WRECKAGE.vst3'
$presetDir = Join-Path ([Environment]::GetFolderPath('MyDocuments')) 'WRECKAGE/Presets'
New-Item -ItemType Directory -Force $presetDir | Out-Null
$preset = Join-Path $presetDir 'installer-smoke-test.wreck'
Set-Content $preset 'preserve this user preset'

try {
    $setup = (Resolve-Path 'dist/WRECKAGE-Setup.exe').Path
    $options = @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART', '/SP-')
    Run-Setup $setup $options
    if (!(Test-Path "$app/WRECKAGE.exe")) { throw 'Standalone app missing' }
    if (!(Test-Path "$plugin/Contents/x86_64-win/WRECKAGE.vst3")) { throw 'VST3 bundle missing' }
    if (!(Test-Path "$app/INSTALL.txt")) { throw 'Install guide missing' }
    $shortcut = Join-Path $env:ProgramData 'Microsoft/Windows/Start Menu/Programs/WRECKAGE/WRECKAGE.lnk'
    if (!(Test-Path $shortcut)) { throw 'Start menu shortcut missing' }

    # Installing over an existing copy must work, too.
    Run-Setup $setup $options
    Run-Setup "$app/unins000.exe" @('/VERYSILENT', '/SUPPRESSMSGBOXES', '/NORESTART')
    if (Test-Path "$app/WRECKAGE.exe") { throw 'Standalone app survived uninstall' }
    if (Test-Path $plugin) { throw 'VST3 bundle survived uninstall' }
    if ((Get-Content $preset -Raw).Trim() -ne 'preserve this user preset') {
        throw 'User preset was modified or removed'
    }
    Write-Host 'Install, reinstall, uninstall, and preset preservation passed.'
} finally {
    Remove-Item $preset -ErrorAction SilentlyContinue
}
