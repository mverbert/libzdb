# ===========================================================================
#
# SYNOPSIS
#
#  AX_CONFIGURE_INFO([Info Label], [Boolean Expression])
#
#   Provide colorized output of configure variables. Only
#   useful for tildeslash's configure scripts
#
#  AX_CONFIGURE_INFO_SEPARATOR()
#
#   Print an in-box separator line
#
#  AX_CONFIGURE_INFO_BREAK()
#
#   Print a box's start or end line
#
#
#  Copyright © 2020 Tildeslash Ltd. All rights reserved.

AC_DEFUN([AX_CONFIGURE_INFO],
[
    if $($2); then
        printf "|  %-47.47s %-24s  |\n" "$1" "$(tput bold)$(tput setaf 2)ENABLED$(tput sgr 0)"
    else
        printf "|  %-47.47s %-24s  |\n" "$1" "$(tput bold)$(tput setaf 0)DISABLED$(tput sgr 0)"
    fi
])

AC_DEFUN([AX_CONFIGURE_INFO_SEPARATOR],
[
    echo "|------------------------------------------------------------|"
])


AC_DEFUN([AX_CONFIGURE_INFO_BREAK],
[
    echo "+------------------------------------------------------------+"
])

