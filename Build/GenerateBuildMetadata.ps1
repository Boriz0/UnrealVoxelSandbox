param(
	[Parameter(Mandatory = $true)][string]$ProjectDirectory,
	[Parameter(Mandatory = $true)][string]$TargetName
)

$ErrorActionPreference = 'Stop'
$revision = 'Unknown'
$count = 'Unknown'
try {
	if (Get-Command git -ErrorAction SilentlyContinue) {
		$hash = & git -C $ProjectDirectory rev-parse --verify HEAD 2>$null
		if ($LASTEXITCODE -eq 0 -and $hash -match '^[0-9a-f]{40,64}$') {
			$revision = $hash
			# A shallow clone's count is incomplete, so do not present it as a revision number.
			$shallow = & git -C $ProjectDirectory rev-parse --is-shallow-repository 2>$null
			if ($LASTEXITCODE -eq 0 -and $shallow -eq 'false') {
				$total = & git -C $ProjectDirectory rev-list --count HEAD 2>$null
				if ($LASTEXITCODE -eq 0 -and $total -match '^\d+$') { $count = $total }
			}
		}
	}
} catch {
	# Git is optional for source archives and offline build machines.
	Write-Warning 'Git metadata unavailable; displaying Unknown.'
}

# UTC timestamp is sortable and distinguishes local builds of the same commit.
# It is a build identifier, not a centrally allocated release sequence.
$utc = [DateTime]::UtcNow
$number = $utc.ToString('yyyyMMddHHmmss', [Globalization.CultureInfo]::InvariantCulture)
$date = $utc.ToString('yyyy-MM-dd', [Globalization.CultureInfo]::InvariantCulture)
$directory = Join-Path $ProjectDirectory "Intermediate/Build/Metadata/$TargetName"
[IO.Directory]::CreateDirectory($directory) | Out-Null
$header = @"
#pragma once
#define VOXEL_BUILD_REVISION "$revision"
#define VOXEL_REVISION_COUNT "$count"
#define VOXEL_BUILD_NUMBER "$number"
#define VOXEL_BUILD_DATE "$date"
"@
[IO.File]::WriteAllText((Join-Path $directory 'VoxelBuildMetadata.h'), $header, [Text.Encoding]::ASCII)
Write-Host "Voxel build $number | revision $count ($revision)"
