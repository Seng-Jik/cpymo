
function Parse-GameConfig($filename) {
    $src = Get-Content $filename
    $dst = [Ordered]@{}
    foreach ($line in $src) {
        $line = $line.Trim()
        $p = $line.IndexOf(',')
        if ($p -ge 0) {
            $key = $line.Substring(0, $p)
            $value = $line.Substring($p + 1)
            $dst[$key] = $value
        }
    }
    return $dst
}

function Write-Help() {
    Write-Host "PyMO Game Audio Converter"
    Write-Host ""
    Write-Host "You must ensure cpymo-tool and ffmpeg has installed."
    Write-Host "Usage:"
    Write-Host "    pymo-convert-audio.ps1 <ogg/mp3/wav> <gamedir>"
    Write-Host ""
}

if ($args.Length -ne 2) { 
    Write-Help
    Break Script
}

$args[0] = $args[0].Trim().ToLower()
if (($args[0] -ne "ogg") -and ($args[0] -ne "mp3") -and (args[0] -ne "wav")) {
    Write-Help
    Break Script
}

$target_ext = "." + $args[0]
$gamedir = $args[1]

function Convert-Asset($gamedir, $asstype, $ext) {
    if (-not (Test-Path "$gamedir/$asstype")) { return $false }
    if (((ls "$gamedir/$asstype/*$ext").Count -eq 0) -and (-not (Test-Path "$gamedir/$asstype/$asstype.pak"))) { return $false }

    if ($ext.ToUpper().Trim() -eq $target_ext) {
        return $false
    }

    $pack = $false
    
    $unpack_dir = "$gamedir/$asstype-unpack"
    if (Test-Path $unpack_dir) {
        Remove-Item $unpack_dir -Force -Recurse
    }

    if (Test-Path "$gamedir/$asstype/$asstype.pak") {
        $pack = $true
        mkdir $unpack_dir
        cpymo-tool unpack "$gamedir/$asstype/$asstype.pak" $ext $unpack_dir
    }
    else {
        mv "$gamedir/$asstype" $unpack_dir
    }

    $convert_dir = "$gamedir/$asstype-convert"
    if (Test-Path $convert_dir) {
        Remove-Item $unpack_dir -Force -Recurse
    }
    mkdir $convert_dir

    $files = (ls "$unpack_dir/*$ext").Name
    foreach ($i in $files) {
        Write-Host "$asstype/$i"
        ffmpeg -i "$unpack_dir/$i" "$convert_dir/$([System.IO.Path]::GetFileNameWithoutExtension($i))$target_ext" -v quiet
    }

    if ($pack) {
        Rename-Item "$gamedir/$asstype" "$asstype-backup"
        mkdir "$gamedir/$asstype"

        $filelist_file = "$convert_dir/$asstype.txt"

        Out-File `
            -FilePath $filelist_file `
            -Encoding utf8 `
            -InputObject ((ls "$convert_dir/").FullName)

        cpymo-tool pack "$gamedir/$asstype/$asstype.pak" --file-list $filelist_file
        Remove-Item $unpack_dir -Recurse
        Remove-Item $convert_dir -Recurse
    }
    else {
        if (Test-Path "$gamedir/$asstype") {
            Remove-Item "$gamedir/$asstype" -Recurse
        }

        Rename-Item $unpack_dir "$asstype-backup"
        Rename-Item $convert_dir "$asstype"
    }

    return $true
}

$config = Parse-GameConfig "$gamedir/gameconfig.txt"

$c_bgm = Convert-Asset $gamedir "bgm" $config["bgmformat"]
$c_se = Convert-Asset $gamedir "se" $config["seformat"]
$c_vo = Convert-Asset $gamedir "voice" $config["voiceformat"]

if ($c_bgm -or $c_se -or $c_vo) {
    Rename-Item -Path "$gamedir/gameconfig.txt" -NewName "gameconfig-backup.txt"

    $config["bgmformat"] = $target_ext
    $config["seformat"] = $target_ext
    $config["voiceformat"] = $target_ext
    
    $gameconfig_lines = @()
    foreach ($gameconfig_k in $config.Keys) {
        $gameconfig_lines += "$gameconfig_k,$($config[$gameconfig_k])"
    }

    Out-File `
        -FilePath "$gamedir/gameconfig.txt" `
        -Force `
        -InputObject $gameconfig_lines `
        -Encoding utf8

}