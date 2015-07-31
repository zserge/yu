#!/bin/sh

ok() {
	msg=$*
	shift
	if test $* ; then
		echo "PASS: $msg"
	else
		echo "FAIL: $msg"
	fi
}

mkdir -p tmp

# Test: output one line into file
> tmp/log

echo foo | ./yu tmp/log > tmp/out

ok "exited with 0" $? -eq 0
ok "expected line in tmp/log" "x$(cat tmp/log)" = "xfoo"
ok "expected line in stdout" "x$(cat tmp/out)" = "xfoo"

# Test: output another line into file, should be appeneded

echo bar | ./yu tmp/log > tmp/out
ok "line should be appended" $(cat tmp/log | wc -l) -eq 2
ok "first line remains" "x$(head -n 1 tmp/log)" = "xfoo"
ok "second line added" "x$(tail -n 1 tmp/log)" = "xbar"

# Test rotation
cat << EOF | ./yu -n 4 -r 2 tmp/rot > /dev/null
foo
bar
baz
qux
EOF

ok "three files in total" $(ls tmp/rot* | wc -l) -eq 3

cat tmp/rot
echo "TEST FINISHED"
