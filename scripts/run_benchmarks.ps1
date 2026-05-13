param(
    [string]$Executable = "",
    [int]$MatrixSize = 2048,
    [string[]]$ProcessCounts = @("1", "2", "4"),
    [int]$Runs = 3,
    [int]$Repetitions = 1,
    [string]$OutputCsv = ""
)

$ErrorActionPreference = "Stop"

$projectRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path

if ([string]::IsNullOrWhiteSpace($Executable)) {
    $Executable = Join-Path $projectRoot "build\Release\mpi_matrix_multiplier.exe"
}

if ([string]::IsNullOrWhiteSpace($OutputCsv)) {
    $OutputCsv = Join-Path $projectRoot "benchmark_results.csv"
}

if (-not (Test-Path $Executable)) {
    throw "Executable not found: $Executable"
}

$normalizedProcessCounts = @()
foreach ($entry in $ProcessCounts) {
    $normalizedProcessCounts += ($entry -split ",")
}

$parsedProcessCounts = $normalizedProcessCounts |
    Where-Object { -not [string]::IsNullOrWhiteSpace($_) } |
    ForEach-Object { [int]$_.Trim() }

if ($parsedProcessCounts.Count -eq 0) {
    throw "At least one process count must be provided."
}

$results = @()

foreach ($processCount in $parsedProcessCounts) {
    $times = @()

    for ($run = 1; $run -le $Runs; $run++) {
        $commandOutput = & mpiexec -n $processCount $Executable --size $MatrixSize --repetitions $Repetitions
        if ($LASTEXITCODE -ne 0) {
            throw "mpiexec returned exit code $LASTEXITCODE for process count $processCount"
        }

        $resultLine = $commandOutput | Where-Object { $_ -like "RESULT *" } | Select-Object -Last 1
        if (-not $resultLine) {
            throw "Could not find RESULT line in program output for process count $processCount"
        }

        $timeToken = ($resultLine -split " ") | Where-Object { $_ -like "distributed_seconds=*" } | Select-Object -First 1
        if (-not $timeToken) {
            throw "Could not parse distributed_seconds from '$resultLine'"
        }

        $seconds = [double]($timeToken -replace "distributed_seconds=", "")
        $times += $seconds

        Write-Host ("processes={0} run={1} time={2:N6} s" -f $processCount, $run, $seconds)
    }

    $measurement = $times | Measure-Object -Average -Minimum -Maximum
    $results += [PSCustomObject]@{
        Processes      = $processCount
        MatrixSize     = $MatrixSize
        Runs           = $Runs
        Repetitions    = $Repetitions
        AverageSeconds = [math]::Round($measurement.Average, 6)
        MinSeconds     = [math]::Round($measurement.Minimum, 6)
        MaxSeconds     = [math]::Round($measurement.Maximum, 6)
    }
}

$baseline = ($results | Where-Object { $_.Processes -eq 1 } | Select-Object -First 1)
if (-not $baseline) {
    throw "A benchmark with 1 process is required to compute speedup."
}

$finalResults = foreach ($entry in $results) {
    $speedup = $baseline.AverageSeconds / $entry.AverageSeconds
    [PSCustomObject]@{
        Processes      = $entry.Processes
        MatrixSize     = $entry.MatrixSize
        Runs           = $entry.Runs
        Repetitions    = $entry.Repetitions
        AverageSeconds = $entry.AverageSeconds
        MinSeconds     = $entry.MinSeconds
        MaxSeconds     = $entry.MaxSeconds
        Speedup        = [math]::Round($speedup, 3)
        Efficiency     = [math]::Round($speedup / $entry.Processes, 3)
    }
}

$finalResults | Export-Csv -Path $OutputCsv -NoTypeInformation -Encoding UTF8
$finalResults | Format-Table -AutoSize

Write-Host "Saved benchmark table to $OutputCsv"
