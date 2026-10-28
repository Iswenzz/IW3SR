# Rebuilds iw3sr.ff from raw/ with the CoD4 mod tools. GZones mounts the zone from IW3SR/Zone with common_mp. The
# linker never puts a material's images in a fastfile: CMake packs raw/images into iw3sr.iwd, which IW3SR opens itself.
#
#   powershell -ExecutionPolicy Bypass -File resources\zone\build.ps1 -Sdk D:\Dev\CoD4\SDK
#
# -Sdk is the mod tools root (bin\linker_pc.exe, raw\shader_bin\shader_tool.exe), or set COD4_SDK. The raw/ files
# are copied into it first, since the tools only read their own tree.
param(
	[string]$Sdk = $env:COD4_SDK
)

$ErrorActionPreference = "Stop"
$Zone = "iw3sr"
$Here = $PSScriptRoot

if (-not $Sdk) { throw "Pass -Sdk <mod tools root> or set COD4_SDK." }
$Linker = Join-Path $Sdk "bin\linker_pc.exe"
$ShaderBin = Join-Path $Sdk "raw\shader_bin"
$ShaderTool = Join-Path $ShaderBin "shader_tool.exe"
foreach ($Tool in $Linker, $ShaderTool) {
	if (-not (Test-Path $Tool)) { throw "Missing $Tool." }
}

# shader_names holds the vertex list then the pixel list, each a count and {name hash, file hash} pairs that the
# linker binary-searches by name hash. shader_tool can register a shader out of order, and every lookup whose
# search crosses it then fails. Same fix as IzFF's "shadernames".
Add-Type -TypeDefinition @"
using System;
using System.IO;

public static class ShaderNames
{
	public static int Sort(string path)
	{
		byte[] data = File.ReadAllBytes(path);
		int at = 0, moved = 0;
		for (int list = 0; list < 2; list++)
		{
			int count = BitConverter.ToInt32(data, at);
			at += 4;
			ulong[] pairs = new ulong[count];
			ulong[] keys = new ulong[count];
			for (int i = 0; i < count; i++)
			{
				pairs[i] = BitConverter.ToUInt64(data, at + i * 8);
				// The index under the name keeps pairs that share a name in their order.
				keys[i] = ((ulong)(uint)pairs[i] << 32) | (uint)i;
				if (i > 0 && (uint)pairs[i] < (uint)pairs[i - 1])
					moved++;
			}
			Array.Sort(keys, pairs);
			for (int i = 0; i < count; i++)
				Array.Copy(BitConverter.GetBytes(pairs[i]), 0, data, at + i * 8, 8);
			at += count * 8;
		}
		if (moved > 0)
			File.WriteAllBytes(path, data);
		return moved;
	}
}
"@

# Windows PowerShell turns a native tool's stderr into errors, which "Stop" would throw on.
function Invoke-Tool([string]$Directory, [string]$Tool, [string[]]$Arguments) {
	Push-Location $Directory
	$Previous = $ErrorActionPreference
	$ErrorActionPreference = "Continue"
	try { $Output = & $Tool @Arguments 2>&1 | Out-String }
	finally {
		$ErrorActionPreference = $Previous
		Pop-Location
	}
	Write-Host $Output
	if ($LASTEXITCODE -ne 0) { throw "$(Split-Path -Leaf $Tool) failed with exit code $LASTEXITCODE." }
	return $Output
}

$Raw = Join-Path $Here "raw"
foreach ($File in Get-ChildItem $Raw -Recurse -File) {
	$Target = Join-Path (Join-Path $Sdk "raw") $File.FullName.Substring($Raw.Length + 1)
	New-Item -ItemType Directory -Force (Split-Path $Target) | Out-Null
	Copy-Item $File.FullName $Target -Force
}

Invoke-Tool $ShaderBin $ShaderTool @("iw3sr_flow") | Out-Null
$Moved = [ShaderNames]::Sort((Join-Path $ShaderBin "shader_names"))
if ($Moved) { Write-Host "shader_names: put $Moved entries back in order" }

Copy-Item (Join-Path $Here "$Zone.csv") (Join-Path $Sdk "zone_source\$Zone.csv") -Force
$Log = Invoke-Tool (Join-Path $Sdk "bin") $Linker @("-language", "english", "-nopause", $Zone)
# The linker can report a broken asset and still exit 0.
if ($Log -cmatch "ERROR" -or $Log -notmatch "save\.\.\.done") { throw "linker_pc did not build $Zone.ff." }
Copy-Item (Join-Path $Sdk "zone\english\$Zone.ff") (Join-Path $Here "$Zone.ff") -Force

Write-Host "Built $Zone.ff in $Here"
