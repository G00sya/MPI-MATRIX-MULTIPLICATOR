param(
    [int]$Size = 1024,
    [int]$SeedA = 42,
    [int]$SeedB = 43,
    [string]$OutputDir = ""
)

$ErrorActionPreference = "Stop"

if ($Size -le 0) {
    throw "Size must be greater than zero."
}

$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

if ([string]::IsNullOrWhiteSpace($OutputDir)) {
    $OutputDir = Join-Path $projectRoot "data\\generated"
}

if (-not (Test-Path $OutputDir)) {
    New-Item -ItemType Directory -Path $OutputDir | Out-Null
}

$matrixAPath = Join-Path $OutputDir ("matrix_a_{0}.txt" -f $Size)
$matrixBPath = Join-Path $OutputDir ("matrix_b_{0}.txt" -f $Size)
$encoding = [System.Text.ASCIIEncoding]::new()

function Write-MatrixFile([string]$Path, [int]$Seed, [int]$Size, [System.Text.Encoding]$Encoding) {
    $random = [System.Random]::new($Seed)
    $writer = [System.IO.StreamWriter]::new($Path, $false, $Encoding)

    try {
        $writer.WriteLine("{0} {0}" -f $Size)

        for ($row = 0; $row -lt $Size; $row++) {
            $builder = [System.Text.StringBuilder]::new()

            for ($col = 0; $col -lt $Size; $col++) {
                if ($col -gt 0) {
                    [void]$builder.Append(' ')
                }

                [void]$builder.Append($random.Next(0, 10))
            }

            $writer.WriteLine($builder.ToString())
        }
    }
    finally {
        $writer.Dispose()
    }
}

Write-MatrixFile -Path $matrixAPath -Seed $SeedA -Size $Size -Encoding $encoding
Write-MatrixFile -Path $matrixBPath -Seed $SeedB -Size $Size -Encoding $encoding

Write-Host "Generated files:"
Write-Host $matrixAPath
Write-Host $matrixBPath
