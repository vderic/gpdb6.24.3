# Copyright (c) 2018-2021, Vitesse Data Inc. All rights reserved.
echo 'int main() { return 0; }' > t.c
OUT=''
if (gcc t.c -lldap >& /dev/null); then
    OUT="$OUT -lldap"
fi
if (gcc t.c -llber >& /dev/null); then
    OUT="$OUT -llber"
fi
echo $OUT

