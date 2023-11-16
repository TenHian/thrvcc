#!/bin/bash

# make a tmp dir, XXXXXX will be replaced by random string
tmp=`mktemp -d /tmp/thrvcc-test-XXXXXX`
# clean
# when received interruptions(ctrl+c), terminate,
# hang (ssh down, user out), exit, while exit,
# execute the rm command to delete the newly created temporary folder
trap 'rm -rf $tmp' INT TERM HUP EXIT
# touch new empty file in tmp dir, named empty.c
echo > $tmp/empty.c

# Determine whether the program was executed successfully by\
# determining whether the return value is 0
check() {
        if [ $? -eq 0 ]; then
                echo "testing $1 ... passed"
        else
                echo "testing $1 ... failed"
                exit 1
        fi
}

# -o
# clean the out file in $tmp
rm -f $tmp/out
# compile to generate out file
./thrvcc -o $tmp/out $tmp/empty.c
# conditional judgment, whether there is out file
[ -f $tmp/out ]
# pass -o into check()
check -o

# --help
# pass the results of --help to grep for line filtering
# -q does not output, 
# whether to match the results of lines where the thrvcc string exists
./thrvcc --help 2>&1 | grep -q thrvcc
# pass --help into check()
check --help

# -S
echo 'int main() {}' | ./thrvcc -S -o - - | grep -q 'main:'
check -S

# default output file
rm -f $tmp/out.o $tmp/out.s
echo 'int main() {}' > $tmp/out.c
(./thrvcc $tmp/out.c > $tmp/out.o)
[ -f $tmp/out.o ]
check 'default output file'

(./thrvcc -S $tmp/out.c > $tmp/out.s)
[ -f $tmp/out.s ]
check 'default output file'

# [154] Accepts multiple input files.
rm -f $tmp/foo.o $tmp/bar.o
echo 'int x;' > $tmp/foo.c
echo 'int y;' > $tmp/bar.c
(cd $tmp; $OLDPWD/thrvcc $tmp/foo.c $tmp/bar.c)
[ -f $tmp/foo.c ] && [ -f $tmp/bar.o ]
check 'multiple input files'

rm -f $tmp/foo.s $tmp/bar.s
echo 'int x;' > $tmp/foo.c
echo 'int y;' > $tmp/bar.c
(cd $tmp; $OLDPWD/thrvcc -S $tmp/foo.c $tmp/bar.c)
[ -f $tmp/foo.s ] && [ -f $tmp/bar.s ]
check 'multiple input files'

echo OK
