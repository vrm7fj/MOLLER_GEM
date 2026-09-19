#!/bin/bash
set -e

###########################################################################
#
# Scans moller.uvagem.corrcoeff_cut (the U/V cluster charge correlation-
# coefficient cut, DBRequest key "corrcoeff_cut" -> fCorrCoeffCut in
# MOLLERGEMModule.cxx, used as "ccor >= ccor_cut" when filtering 2D hits)
# over a list of values, re-replaying the same run/segment/event window
# for each one, and produces a small per-scan-point ROOT file of
# ADC-vs-time-sample-per-APV profiles.
#
# Pipeline per scan value:
#   1. sed-edit the DB file's "moller.uvagem.corrcoeff_cut = ..." line
#   2. run-replay_gep.sh <runnum> <firstsegment> <maxsegments> <firstevent> <NEVENTS>
#   3. rename the replay output (same filename every time otherwise --
#      the next iteration would overwrite it)
#   4. root -l -b -q moller_scan_fill.C(that file, this scan point's tag)
#      -> writes moller_scan_hist_<tag>.root
#
# The original DB file is restored automatically when this script exits,
# whether it finishes normally, errors out, or is Ctrl-C'd.
#
# Afterwards, paste the file list this script prints at the end into
# moller_scan_overlay.C's scanFiles/scanLabels and run:
#   root -l -b -q moller_scan_overlay.C
#
# Usage: ./moller_scan_corrcoeff.sh <runnum>
# (firstsegment/maxsegments/firstevent are fixed below -- this setup
# always replays segment 0 only, starting at event 0)
#
###########################################################################

# =========================== EDIT THESE ===========================

# Values of moller.uvagem.corrcoeff_cut to scan over:
SCAN_VALUES=(0.3 0.4 0.5 0.6 0.7)

# DB key (the part after "moller.uvagem."). Switch to "corrcoeff_cut_deconv"
# if you're running with the deconvolution-based clustering flag instead.
DB_KEY="corrcoeff_cut"

# Full path to the actual DB file you replay against -- point this at
# whatever's live on your DAQ machine, not this repo's example copy:
DB_FILE="$MOLLER_REPLAY/DB/20230226/db_moller.uvagem.dat"

# Wherever you saved the run-replay script:
REPLAY_SCRIPT="./run-replay_gep.sh"

# Events per scan point:
NEVENTS=10000

# Fixed for this setup: always one segment, segment 0, starting at event 0.
FIRSTSEG=0
MAXSEG=1
FIRSTEVENT=0

# ====================================================================

RUNNUM=$1

if [ -z "$RUNNUM" ]; then
  echo "Usage: $0 <runnum>"
  exit 1
fi

if [ -z "$OUT_DIR" ]; then
  echo "OUT_DIR is not set in this shell -- source the same environment"
  echo "run-replay_gep.sh expects before running this script."
  exit 1
fi

if [ ! -f "$DB_FILE" ]; then
  echo "ERROR: DB_FILE not found: $DB_FILE"
  echo "Edit DB_FILE at the top of this script to point at your actual db_moller.uvagem.dat."
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

  echo "=========================================================="
  echo "Scan point: moller.uvagem.${DB_KEY} = ${VAL}"
  echo "=========================================================="

  # ---- 1. write the new value into the DB file ----
  # Matches "moller.uvagem.<key> = <old value>" (with optional leading
  # whitespace and an optional trailing "# comment") and replaces just
  # the value token, leaving everything else on the line untouched.
  sed -i -E "s|^([[:space:]]*moller\.uvagem\.${DB_KEY}[[:space:]]*=[[:space:]]*)[^[:space:]#]+|\1${VAL}|" "$DB_FILE"

  echo -n "  -> "; grep "moller\.uvagem\.${DB_KEY}[[:space:]]*=" "$DB_FILE"

  # ---- 2. run the replay ----
  bash "$REPLAY_SCRIPT" "$RUNNUM" "$FIRSTSEG" "$MAXSEG" "$FIRSTEVENT" "$NEVENTS"

  # ---- 3. rename the output so the next iteration doesn't clobber it ----
  SRC="$OUT_DIR/moller_uvagem_replayed_${RUNNUM}_seg${FIRSTSEG}_${MAXSEG}.root"
  TAG="${DB_KEY}_$(echo "$VAL" | tr '.' 'p')"   # e.g. corrcoeff_cut_0p50
  DST="$OUT_DIR/moller_uvagem_replayed_${RUNNUM}_${TAG}.root"

  if [ ! -f "$SRC" ]; then
    echo "ERROR: expected replay output not found: $SRC"
    echo "(check that OUT_DIR, RUNNUM, FIRSTSEG, MAXSEG match what run-replay_gep.sh actually wrote)"
    exit 1
  fi

  mv "$SRC" "$DST"
  echo "Saved replay output: $DST"

  # ---- 4. fill this scan point's ADC-vs-sample histograms ----
  root -l -b -q "moller_scan_fill.C(\"${DST}\", \"${TAG}\")"

  SCAN_ROOTFILES+=("moller_scan_hist_${TAG}.root")
  SCAN_LABELS+=("${DB_KEY} = ${VAL}")

done

echo
echo "Scan done. Per-point histogram files:"
for f in "${SCAN_ROOTFILES[@]}"; do echo "  \"$f\","; done
echo
echo "Paste those (with matching labels) into moller_scan_overlay.C's"
echo "scanFiles/scanLabels, then run:"
echo "  root -l -b -q moller_scan_overlay.C"
