#!/bin/bash
set -e

# Scans moller.uvagem.corrcoeff_cut (fCorrCoeffCut in MOLLERGEMModule.cxx,
# the U/V cluster charge correlation-coefficient cut) over SCAN_VALUES,
# re-replaying for each and producing a per-value ADC-vs-sample ROOT file.
# Restores the original DB file on exit either way.
#
# Usage: ./moller_scan_corrcoeff.sh <runnum>
# Afterwards: paste the printed file list into moller_scan_overlay.C and run it.

# ---- EDIT THESE ----
SCAN_VALUES=(0.3 0.4 0.5 0.6 0.7)
DB_KEY="corrcoeff_cut"          # or "corrcoeff_cut_deconv"
DB_FILE="$MOLLER_REPLAY/DB/20230226/db_moller.uvagem.dat"
REPLAY_SCRIPT="./run-replay_gep.sh"
NEVENTS=10000
FIRSTSEG=0
MAXSEG=1
FIRSTEVENT=0
# ---------------------

RUNNUM=$1

if [ -z "$RUNNUM" ]; then
  echo "Usage: $0 <runnum>"
  exit 1
fi

if [ -z "$OUT_DIR" ]; then
  echo "OUT_DIR is not set -- source the same environment run-replay_gep.sh expects."
  exit 1
fi

if [ ! -f "$DB_FILE" ]; then
  echo "ERROR: DB_FILE not found: $DB_FILE"
  exit 1
fi

if ! grep -q "moller\.uvagem\.${DB_KEY}[[:space:]]*=" "$DB_FILE"; then
  echo "ERROR: no line matching 'moller.uvagem.${DB_KEY} = ...' found in $DB_FILE"
  exit 1
fi

DB_BACKUP="${DB_FILE}.scan_backup"
cp "$DB_FILE" "$DB_BACKUP"
echo "Backed up $DB_FILE -> $DB_BACKUP"

cleanup() {
  cp "$DB_BACKUP" "$DB_FILE"
  echo "Restored original $DB_FILE"
}
trap cleanup EXIT

SCAN_ROOTFILES=()
SCAN_LABELS=()

for VAL in "${SCAN_VALUES[@]}"; do

  echo "=== Scan point: moller.uvagem.${DB_KEY} = ${VAL} ==="

  sed -i -E "s|^([[:space:]]*moller\.uvagem\.${DB_KEY}[[:space:]]*=[[:space:]]*)[^[:space:]#]+|\1${VAL}|" "$DB_FILE"
  echo -n "  -> "; grep "moller\.uvagem\.${DB_KEY}[[:space:]]*=" "$DB_FILE"

  bash "$REPLAY_SCRIPT" "$RUNNUM" "$FIRSTSEG" "$MAXSEG" "$FIRSTEVENT" "$NEVENTS"

  SRC="$OUT_DIR/moller_uvagem_replayed_${RUNNUM}_seg${FIRSTSEG}_${MAXSEG}.root"
  TAG="${DB_KEY}_$(echo "$VAL" | tr '.' 'p')"
  DST="$OUT_DIR/moller_uvagem_replayed_${RUNNUM}_${TAG}.root"

  if [ ! -f "$SRC" ]; then
    echo "ERROR: expected replay output not found: $SRC"
    exit 1
  fi

  mv "$SRC" "$DST"
  echo "Saved replay output: $DST"

  root -l -b -q "moller_scan_fill.C(\"${DST}\", \"${TAG}\")"

  SCAN_ROOTFILES+=("moller_scan_hist_${TAG}.root")
  SCAN_LABELS+=("${DB_KEY} = ${VAL}")

done

echo
echo "Scan done. Per-point histogram files:"
for f in "${SCAN_ROOTFILES[@]}"; do echo "  \"$f\","; done
echo
echo "Paste those into moller_scan_overlay.C's scanFiles/scanLabels, then run:"
echo "  root -l -b -q moller_scan_overlay.C"
