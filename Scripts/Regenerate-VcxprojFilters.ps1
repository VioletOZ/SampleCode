<#
.SYNOPSIS
    .vcxproj의 실제 Include 경로(폴더 구조)를 기준으로 .vcxproj.filters를 재생성한다.

.PARAMETER VcxprojPath
    대상 .vcxproj 파일의 전체 경로.

.EXAMPLE
    powershell -File D:\Sample\Scripts\Regenerate-VcxprojFilters.ps1 -VcxprojPath D:\Sample\NetworkServer\OZServer\OZServer.vcxproj
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$VcxprojPath
)

if (-not (Test-Path $VcxprojPath)) {
    Write-Error "파일을 찾을 수 없습니다: $VcxprojPath"
    exit 1
}

$filtersPath = "$VcxprojPath.filters"

[xml]$proj = Get-Content -Raw -Encoding UTF8 $VcxprojPath

$ns = New-Object System.Xml.XmlNamespaceManager($proj.NameTable)
$ns.AddNamespace("msb", "http://schemas.microsoft.com/developer/msbuild/2003")

function Get-FolderOf([string]$path) {
    $idx = $path.LastIndexOf('\')
    if ($idx -lt 0) { return $null }
    return $path.Substring(0, $idx)
}

# 1) vcxproj에서 실제 Include 경로 수집
$compileItems = @($proj.SelectNodes("//msb:ClCompile[@Include]", $ns) | ForEach-Object { $_.Include })
$includeItems = @($proj.SelectNodes("//msb:ClInclude[@Include]", $ns) | ForEach-Object { $_.Include })

# 2) 폴더 집합 추출 (순서 유지, 중복 제거)
$folders = New-Object System.Collections.Generic.List[string]
foreach ($p in ($compileItems + $includeItems)) {
    $f = Get-FolderOf $p
    if ($f -and -not $folders.Contains($f)) {
        $folders.Add($f)
    }
}

# 3) 기존 filters 파일에서 폴더별 GUID 재사용 (없으면 새로 생성)
$guidMap = @{}
if (Test-Path $filtersPath) {
    [xml]$oldFilters = Get-Content -Raw -Encoding UTF8 $filtersPath
    $oldNs = New-Object System.Xml.XmlNamespaceManager($oldFilters.NameTable)
    $oldNs.AddNamespace("msb", "http://schemas.microsoft.com/developer/msbuild/2003")
    foreach ($node in $oldFilters.SelectNodes("//msb:Filter[@Include]", $oldNs)) {
        $guidNode = $node.SelectSingleNode("msb:UniqueIdentifier", $oldNs)
        if ($guidNode) {
            $guidMap[$node.Include] = $guidNode.InnerText
        }
    }
}

foreach ($f in $folders) {
    if (-not $guidMap.ContainsKey($f)) {
        $guidMap[$f] = "{" + [guid]::NewGuid().ToString().ToUpper() + "}"
    }
}

# 4) VS 표준 기본 필터 (고정 GUID)
$defaultFilters = [ordered]@{
    "소스 파일"   = @{ Guid = "{4FC737F1-C7A5-4376-A066-2A32D752A2FF}"; Ext = "cpp;c;cc;cxx;c++;cppm;ixx;def;odl;idl;hpj;bat;asm;asmx" }
    "헤더 파일"   = @{ Guid = "{93995380-89BD-4b04-88EB-625FBE52EBFB}"; Ext = "h;hh;hpp;hxx;h++;hm;inl;inc;ipp;xsd" }
    "리소스 파일" = @{ Guid = "{67DA6AB6-F800-4c08-8B7A-83BB121AAD01}"; Ext = "rc;ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe;resx;tiff;tif;png;wav;mfcribbon-ms" }
}

# 5) 새 .filters XML 작성
$sb = New-Object System.Text.StringBuilder
[void]$sb.AppendLine('<?xml version="1.0" encoding="utf-8"?>')
[void]$sb.AppendLine('<Project ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">')
[void]$sb.AppendLine('  <ItemGroup>')

foreach ($name in $defaultFilters.Keys) {
    $info = $defaultFilters[$name]
    [void]$sb.AppendLine("    <Filter Include=`"$name`">")
    [void]$sb.AppendLine("      <UniqueIdentifier>$($info.Guid)</UniqueIdentifier>")
    [void]$sb.AppendLine("      <Extensions>$($info.Ext)</Extensions>")
    [void]$sb.AppendLine('    </Filter>')
}

foreach ($f in $folders) {
    [void]$sb.AppendLine("    <Filter Include=`"$f`">")
    [void]$sb.AppendLine("      <UniqueIdentifier>$($guidMap[$f])</UniqueIdentifier>")
    [void]$sb.AppendLine('    </Filter>')
}

[void]$sb.AppendLine('  </ItemGroup>')
[void]$sb.AppendLine('  <ItemGroup>')

foreach ($p in $compileItems) {
    $f = Get-FolderOf $p
    if ($f) {
        [void]$sb.AppendLine("    <ClCompile Include=`"$p`">")
        [void]$sb.AppendLine("      <Filter>$f</Filter>")
        [void]$sb.AppendLine('    </ClCompile>')
    }
    else {
        [void]$sb.AppendLine("    <ClCompile Include=`"$p`" />")
    }
}

[void]$sb.AppendLine('  </ItemGroup>')
[void]$sb.AppendLine('  <ItemGroup>')

foreach ($p in $includeItems) {
    $f = Get-FolderOf $p
    if ($f) {
        [void]$sb.AppendLine("    <ClInclude Include=`"$p`">")
        [void]$sb.AppendLine("      <Filter>$f</Filter>")
        [void]$sb.AppendLine('    </ClInclude>')
    }
    else {
        [void]$sb.AppendLine("    <ClInclude Include=`"$p`" />")
    }
}

[void]$sb.AppendLine('  </ItemGroup>')
[void]$sb.AppendLine('</Project>')

Set-Content -Path $filtersPath -Value $sb.ToString() -Encoding UTF8
Write-Host "재생성 완료: $filtersPath"
