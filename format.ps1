#!/usr/bin/env pwsh

uncrustify -c uncrustify.cfg --no-backup (Get-ChildItem -Recurse include/satella/*.hpp).FullName
uncrustify -c uncrustify.cfg --no-backup (Get-ChildItem -Recurse sample/*.cpp).FullName
