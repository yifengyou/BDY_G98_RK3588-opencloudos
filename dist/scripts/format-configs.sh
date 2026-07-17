#!/bin/bash --norc
# SPDX-License-Identifier: GPL-2.0
#
# Generate kernel config according to config matrix

# shellcheck source=./lib-config.sh
. "/$(dirname "$(realpath "$0")")/lib-config.sh"

# Sort and remove duplicated items in each config base file
for file in "$CONFIG_PATH"/*/*/*.config; do
	# Some config entries are symlinks to their source files. Resolve the
	# destination so the atomic rename does not replace the symlink itself.
	config_file=$(realpath "$file")
	config_sanitizer < "$config_file" > "$config_file.fmt.tmp"

	mv "$config_file.fmt.tmp" "$config_file"
done
