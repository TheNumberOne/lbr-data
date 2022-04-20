#!/bin/sh

LBR_APP_ID=1468260

# location of the windows filesystem used by Proton for Leaf Blower Revolution
WINDOWS_ROOT="${XDG_DATA_HOME:-$HOME/.local/share}/Steam/steamapps/compatdata/$LBR_APP_ID/pfx/drive_c"

# location of the savefile
SAVE_FILE="$WINDOWS_ROOT/users/steamuser/AppData/Local/blow_the_leaves_away/save.dat"

cat "$SAVE_FILE" | \
  base64 -d | \
  # remove the hash at the end so that it is valid json
  sed 's/#[[:xdigit:]]\+#$//' | \
  jq '.' > save.json
