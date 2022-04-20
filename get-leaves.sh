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
  # extract leaf data
  jq '[
    # selects both your main profile and your challenge profile
    .profiles[] |
    (.crafted_leaves, .crafted_backpack)[] |
    .shards as $shards |
    {
      id,
      type: .type_key,
      craft_quality: .quality_base,
      rng_quality: .quality,
      level,
      # the ascend level in the save file is 1 greater than in the UI
      ascensions: (.ascend_level - 1),
      props: [
        .props |
        to_entries[] |
        { name: .key, shards: ($shards[.key] // 0) } +
        (.value |
          (numbers | { value: . }),
          # handle properties that can target different resources
          (objects | { resource: .__resource_key, value: .[.__resource_key] })
        )
      ]
    }
  ]' > leaves.json
