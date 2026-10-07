param([Parameter(Mandatory=$true)][string]$Transcript)
$ErrorActionPreference='Stop'
$root=Split-Path $PSScriptRoot -Parent
$folder=Join-Path $root 'LocalHistory'
New-Item -ItemType Directory -Path $folder -Force | Out-Null
Copy-Item -LiteralPath $Transcript -Destination (Join-Path $folder 'original-codex-chat.jsonl')
$text=[System.Text.StringBuilder]::new()
[void]$text.AppendLine('# Original project conversation - private local archive')
[void]$text.AppendLine('User and assistant messages available in the local Codex transcript. This is an archive, not an imported Codex chat. Tool payloads and internal reasoning omitted. Not for GitHub publication.')
Get-Content -LiteralPath (Join-Path $folder 'original-codex-chat.jsonl') | ForEach-Object {
    try { $record=$_ | ConvertFrom-Json -Depth 100 } catch { return }
    $p=$record.payload
    if ($record.type -eq 'response_item' -and $p.type -eq 'message' -and $p.role -in @('user','assistant') -and $p.channel -ne 'analysis') {
        $parts=@($p.content | Where-Object {$_.type -in @('input_text','output_text','text')} | ForEach-Object {$_.text})
        if ($parts.Count) {
            [void]$text.AppendLine("`n## $($p.role) - $($record.timestamp)`n")
            [void]$text.AppendLine(($parts -join "`n"))
        }
    }
}
[IO.File]::WriteAllText((Join-Path $folder 'Conversation.md'),$text.ToString())
Write-Output "Local-only conversation archive saved to $folder"
