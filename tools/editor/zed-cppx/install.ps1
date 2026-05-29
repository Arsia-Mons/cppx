# Prepare the local sidecar git repo that Zed's dev extension installer
# clones the cppx grammar from. Windows port of install.sh.
#
# Zed's extension manifest requires [grammars.X] to point at a git
# repository (no `path` option). This script mirrors grammar-source/ into
# a sibling git repo, commits, and writes the commit SHA + file:// URL
# back into extension.toml.
#
# Run after editing grammar-source/grammar.js (or regenerating parser.c),
# then in Zed: `zed: install dev extension` -> pick this directory.
#
# Override the sidecar location with $env:CPPX_SIDECAR_DIR.

$ErrorActionPreference = 'Stop'

$ExtDir      = Split-Path -Parent $MyInvocation.MyCommand.Path
$GrammarSrc  = Join-Path $ExtDir 'grammar-source'
$SidecarDir  = if ($env:CPPX_SIDECAR_DIR) { $env:CPPX_SIDECAR_DIR } else { Join-Path $env:LOCALAPPDATA 'zed-cppx-grammar' }
$ExtToml     = Join-Path $ExtDir 'extension.toml'
$ZedCloneDir = Join-Path $ExtDir 'grammars\cppx'

if (-not (Test-Path (Join-Path $GrammarSrc 'grammar.js'))) {
    Write-Error "grammar.js not found at $GrammarSrc"
}

# Zed clones the grammar into <ext>/grammars/<name>/ and refuses if that
# directory exists with non-matching contents. Wipe it on every sync.
if (Test-Path $ZedCloneDir) {
    Remove-Item -Recurse -Force $ZedCloneDir
}

if (-not (Test-Path $SidecarDir)) {
    New-Item -ItemType Directory -Path $SidecarDir | Out-Null
}

# Mirror grammar-source -> sidecar (preserving the sidecar's .git/).
# robocopy's /MIR mirrors and /XD/.XF exclude unwanted paths/files. Exit
# codes 0-7 are non-fatal for robocopy; only >=8 is a real failure.
$robocopyArgs = @(
    $GrammarSrc, $SidecarDir,
    '/MIR',
    '/XD', '.git', 'node_modules', 'build', 'prebuilds',
    '/XF', '*.wasm',
    '/NFL', '/NDL', '/NJH', '/NJS', '/NP', '/NS', '/NC'
)
& robocopy @robocopyArgs | Out-Null
if ($LASTEXITCODE -ge 8) {
    Write-Error "robocopy failed with exit code $LASTEXITCODE"
}

Push-Location $SidecarDir
try {
    if (-not (Test-Path '.git')) {
        git init -q
        $email = (git -C $ExtDir config user.email 2>$null)
        if (-not $email) { $email = 'zed-cppx@local' }
        $name  = (git -C $ExtDir config user.name 2>$null)
        if (-not $name)  { $name  = 'zed-cppx' }
        git config user.email $email
        git config user.name  $name
    }

    git add -A
    git diff --cached --quiet
    $needsCommit = ($LASTEXITCODE -ne 0)

    if ($needsCommit) {
        $stamp = (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ')
        git commit -q -m "Sync from $stamp"
        $sha = (git rev-parse HEAD).Trim()
        Write-Host "synced grammar -> $SidecarDir @ $sha"
    } else {
        $sha = (git rev-parse HEAD).Trim()
        Write-Host "sidecar already up to date at $sha"
    }
} finally {
    Pop-Location
}

# Build the file:// URL. libgit2 / Zed wants forward-slash paths and
# URL-encoded characters (notably: spaces in the user profile path).
#   file:///C:/Users/Foo%20Bar/.cache/zed-cppx-grammar
$sidecarFull = (Resolve-Path $SidecarDir).Path -replace '\\','/'
# Encode path segments individually so the slashes stay literal.
$encoded = ($sidecarFull -split '/' | ForEach-Object { [Uri]::EscapeDataString($_) }) -join '/'
if ($encoded -notmatch '^[A-Za-z]%3A') {
    $repoUrl = "file://$encoded"
} else {
    # Restore the drive-letter colon (EscapeDataString turns ':' into %3A).
    $encoded = $encoded -replace '^([A-Za-z])%3A','$1:'
    $repoUrl = "file:///$encoded"
}

# Rewrite the [grammars.cppx] block in extension.toml. Pure regex on
# section boundaries so we don't take a TOML library dependency.
$lines = Get-Content -LiteralPath $ExtToml
$out   = New-Object System.Collections.Generic.List[string]
$inSection = $false
foreach ($line in $lines) {
    if ($line -match '^\[grammars\.cppx\]') {
        $inSection = $true
        $out.Add($line)
        continue
    }
    if ($inSection -and $line -match '^\[') {
        $inSection = $false
    }
    if ($inSection -and $line -match '^\s*repository\s*=') {
        $out.Add("repository = `"$repoUrl`"")
        continue
    }
    if ($inSection -and $line -match '^\s*rev\s*=') {
        $out.Add("rev = `"$sha`"")
        continue
    }
    $out.Add($line)
}
Set-Content -LiteralPath $ExtToml -Value $out -Encoding utf8

Write-Host "updated $ExtToml"
Get-Content -LiteralPath $ExtToml | Select-String -Pattern '^\[grammars\.cppx\]' -Context 0,2 |
    ForEach-Object { $_.Line; $_.Context.PostContext } | ForEach-Object { Write-Host $_ }
Write-Host ""
Write-Host "next: in Zed, run 'zed: install dev extension' and pick $ExtDir"
