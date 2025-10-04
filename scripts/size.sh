# Size
# 10/3/25

#!/bin/bash

# Get The Path Where This Shell Script Is
SCRIPT_DIR="$(dirname "$(readlink -f "$0")")"

# Source Color Definitions
source $SCRIPT_DIR/colors.sh

printf "${LIGHT_YELLOW}*** Size ***${ENDCOLOR}\n"
idf.py size