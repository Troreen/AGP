param([string]$ArgumentsFile, [string]$PythonFile)
$ErrorActionPreference = 'Stop'
if ($PythonFile) {
    $arguments = @{action='execute'; run='async'; transaction=$true; commands=@(@{kind='python'; mode='exec'; code=[string](Get-Content -LiteralPath $PythonFile -Raw)})}
} else {
    $arguments = Get-Content -LiteralPath $ArgumentsFile -Raw | ConvertFrom-Json
}
$body = @{jsonrpc='2.0'; id=1; method='tools/call'; params=@{name='unreal'; arguments=$arguments}} | ConvertTo-Json -Depth 40 -Compress
$response = Invoke-WebRequest -Uri 'http://127.0.0.1:18777/mcp' -Method Post -ContentType 'application/json' -Headers @{Accept='application/json, text/event-stream'; 'Mcp-Session-Id'='agp-component-sequence'} -Body $body -UseBasicParsing -TimeoutSec 120
$result = ($response.Content | ConvertFrom-Json).result.structuredContent
if ($PythonFile -and $result.data.task.id) {
    $taskId = $result.data.task.id
    for ($attempt = 0; $attempt -lt 60; ++$attempt) {
        Start-Sleep -Milliseconds 500
        $taskBody = @{jsonrpc='2.0'; id=2; method='tools/call'; params=@{name='unreal'; arguments=@{action='task'; command='get'; task_id=$taskId}}} | ConvertTo-Json -Depth 12 -Compress
        $taskResponse = Invoke-WebRequest -Uri 'http://127.0.0.1:18777/mcp' -Method Post -ContentType 'application/json' -Headers @{Accept='application/json, text/event-stream'; 'Mcp-Session-Id'='agp-component-sequence'} -Body $taskBody -UseBasicParsing -TimeoutSec 120
        $result = ($taskResponse.Content | ConvertFrom-Json).result.structuredContent
        if ($result.data.task.state -notin @('queued','running')) { break }
    }
}
if ($result.data.task.result) {
    $result.data.task.state
    foreach ($entry in $result.data.task.result.results) {
        'Command ok: ' + $entry.ok
        $entry.logs | ForEach-Object { $_.message }
        if ($entry.error) { $entry.error }
    }
} else {
    $result | ConvertTo-Json -Depth 40
}
