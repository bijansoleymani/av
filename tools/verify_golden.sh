#!/bin/zsh
# Verify the current build is behaviorally identical to build/golden by replaying
# the same scripted input headlessly and diffing every frame.
cd "$(dirname "$0")/.."
make av >/dev/null 2>&1 || { echo "BUILD FAILED"; exit 2; }
INJ="5=1c0d,40=2d00,40=2e00,72=2e80,95=2d00,96=2d80,120=2e00"
rm -rf /tmp/gv; mkdir -p /tmp/gv
SDL_VIDEODRIVER=dummy AV_INJECT="$INJ" AV_SHOT_SEQ=/tmp/gv AV_SHOT_STEP=1 \
  AV_SHOT=/tmp/gv/final.ppm AV_SHOT_FRAMES=140 ./av &
P=$!; sleep 7; kill $P 2>/dev/null; wait $P 2>/dev/null || true
n=0; bad=0; firstbad=""
for f in build/golden/*.ppm; do
  b=$(basename "$f")
  if [ -f "/tmp/gv/$b" ]; then
    if ! cmp -s "$f" "/tmp/gv/$b"; then bad=$((bad+1)); [ -z "$firstbad" ] && firstbad="$b"; fi
    n=$((n+1))
  fi
done
echo "compared $n frames; $bad differ"
if [ "$bad" -eq 0 ] && [ "$n" -gt 100 ]; then
  echo "PASS: refactor is behavior-identical to golden ✓"
else
  echo "FAIL: first differing frame = $firstbad"
  exit 1
fi
