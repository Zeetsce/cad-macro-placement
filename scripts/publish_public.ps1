<#
.SYNOPSIS
    Скрипт синхронизации и публикации релизного состояния в публичный репозиторий (Showcase).

.DESCRIPTION
    1. Проверяет чистоту рабочей директории git.
    2. Создает временную изолированную ветку из текущей ветки main.
    3. Удаляет служебные файлы агента, внутренние спецификации и скрипты автоматизации:
       AGENTS.md, TASKtoWORK.md, SPEC_DATA_FORMATS.md, SPEC_ALGORITHMS.md, SPEC_VERIFIER.md, docs/AI_CONTRIBUTIONS.md, scripts/.
    4. Создает релизный коммит.
    5. Выполняет push в репозиторий public в ветку main.
    6. Возвращается в ветку main и удаляет временную ветку.

.PARAMETER CommitMessage
    Сообщение релизного коммита. По умолчанию: "Release: public showcase sync"
#>

[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [string]$CommitMessage = "Release: public showcase sync"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

# 1. Проверка статуса git
$status = git status --porcelain
if ($status) {
    Write-Error "Рабочая директория не чиста! Сначала закоммитьте или спрячьте (git stash) текущие изменения в dev."
    exit 1
}

$currentBranch = (git branch --show-current).Trim()
if ($currentBranch -ne "main") {
    Write-Warning "Текущая ветка '$currentBranch', а не 'main'. Публикация рекомендуется из 'main'."
}

# 2. Создание временной ветки
$tempBranch = "temp-public-release-" + (Get-Date -Format "yyyyMMddHHmmss")
Write-Host "Создание временной ветки: $tempBranch..." -ForegroundColor Cyan
git checkout -b $tempBranch

try {
    # 3. Список исключаемых внутренних артефактов ИИ, спецификаций и скриптов
    $artifactsToRemove = @(
        "AGENTS.md",
        "TASKtoWORK.md",
        "SPEC_DATA_FORMATS.md",
        "SPEC_ALGORITHMS.md",
        "SPEC_VERIFIER.md",
        "docs/AI_CONTRIBUTIONS.md",
        "scripts"
    )

    foreach ($target in $artifactsToRemove) {
        if (Test-Path $target) {
            Write-Host "Исключение из публичного релиза: $target" -ForegroundColor Yellow
            git rm -rf --ignore-unmatch $target
        }
    }

    # 4. Проверка наличия изменений для коммита
    $tempStatus = git status --porcelain
    if ($tempStatus) {
        Write-Host "Создание релизного коммита: '$CommitMessage'..." -ForegroundColor Cyan
        git commit -m "$CommitMessage"
    } else {
        Write-Host "Изменений для нового коммита нет." -ForegroundColor Yellow
    }

    # 5. Push в публичный репозиторий (ветка main)
    Write-Host "Отправка в публичный репозиторий (public main)..." -ForegroundColor Green
    git push public "${tempBranch}:main" --force

    Write-Host "Успешно опубликовано в публичный репозиторий!" -ForegroundColor Green
}
finally {
    # 6. Возврат в исходную ветку и удаление временной ветки
    Write-Host "Возврат в исходную ветку $currentBranch..." -ForegroundColor Cyan
    git checkout $currentBranch
    git branch -D $tempBranch
}
