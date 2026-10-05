#!/usr/bin/env bash
# Creates a Git history with one commit + one tag per stage, from the files in this folder.
#
# Usage:  scripts/create_stage_history.sh
# Needs:  git configured with your name/email (git config user.name / user.email).
#
# TIP: for the best evidence of "continuous progress", run ONE stage at a time as you
# actually work through the course (use the --stage N option), instead of all at once.
#   scripts/create_stage_history.sh --stage 1     # commits only stage 1 files
set -euo pipefail
cd "$(dirname "$0")/.."

only=""
[ "${1:-}" = "--stage" ] && only="${2:?stage number}"

[ -d .git ] || git init -q -b main

stage() {  # stage N "commit message" paths...
  local n="$1" msg="$2"; shift 2
  [ -n "$only" ] && [ "$only" != "$n" ] && return 0
  git add -- "$@"
  if git diff --cached --quiet; then echo "stage $n: nothing new to commit"; else git commit -q -m "$msg"; fi
  git tag -f "stage-$n" >/dev/null
  echo "committed + tagged stage-$n"
}

stage 1 "docs: stage 1 - project introduction, scope and objectives" \
  README.md LICENSE .gitignore docs/stage1_introduction.md

stage 2 "docs: stage 2 - requirements document (PRD) and development plan" \
  docs/stage2_requirements_plan.md docs/PROGRESS.md

stage 3 "docs: stage 3 - architecture, UML diagrams, git workflow; build: CMake + CI skeleton" \
  docs/stage3_design_architecture.md CMakeLists.txt Makefile .github

stage 4 "feat: stage 4 - vsensor driver, detectors, state machine, daemon prototype" \
  driver include src/ scripts/load_driver.sh scripts/unload_driver.sh scripts/run_demo.sh docs/stage4_prototype.md

stage 5 "test: stage 5 - unit, integration, E2E and driver tests; results and fixes" \
  tests scripts/e2e_sim.sh scripts/test_driver.sh docs/stage5_testing_improvement.md

stage 6 "docs: stage 6 - final report, release v1.0.0" \
  docs/stage6_final_report.md scripts/create_stage_history.sh
[ -z "$only" ] || [ "$only" = 6 ] && git tag -f v1.0.0 >/dev/null

echo; git log --oneline --decorate
echo; echo "Next:  git remote add origin git@github.com:<you>/sensorguard.git && git push -u origin main --tags"
