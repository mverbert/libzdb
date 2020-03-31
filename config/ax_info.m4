#
#  Functions for printing a configure info box. 
#
#  Copyright © 2020 Tildeslash Ltd. All rights reserved.


AC_DEFUN([AX_INFO_GPL],
[
cat <<EOT
+------------------------------------------------------------+
| License:                                                   |
| This is Open Source Software and use is subject to the GNU |
| GENERAL PUBLIC LICENSE, available in this distribution in  |
| the file COPYING.                                          |
|                                                            |
| By continuing this installation process, you are bound by  |
| the terms of this license agreement. If you do not agree   |
| with the terms of this license, you must abort the         |
| installation process at this point.                        |
+------------------------------------------------------------+
EOT
])

AC_DEFUN([AX_INFO_AGPL],
[
cat <<EOT
+------------------------------------------------------------+
| License:                                                   |
| This is Open Source Software and use is subject to the GNU |
| AFFERO GENERAL PUBLIC LICENSE version 3, available in this |
| distribution in the file COPYING.                          |
|                                                            |
| By continuing this installation process, you are bound by  |
| the terms of this license agreement. If you do not agree   |
| with the terms of this license, you must abort the         |
| installation process at this point.                        |
+------------------------------------------------------------+
EOT
])

AC_DEFUN([AX_INFO_TITLE],
[
    printf "| %-58.58s |\n" "$1"
    printf "|%60s|\n"
    
])

AC_DEFUN([AX_INFO_ENABLED],
[
    if $($2); then
        printf "|  %-47.47s %-24s  |\n" "$1" "$(tput bold)$(tput setaf 2)ENABLED$(tput sgr 0)"
    else
        printf "|  %-47.47s %-24s  |\n" "$1" "$(tput bold)$(tput setaf 0)DISABLED$(tput sgr 0)"
    fi
])

AC_DEFUN([AX_INFO_SEPARATOR],
[
    printf "|%60s|\n" | tr " " "-"
])


AC_DEFUN([AX_INFO_BREAK],
[
    printf "+%60s+\n" | tr " " "-"
])

