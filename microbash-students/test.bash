
#testing successfull commands
./microbash <test.microbash 2>log.txt 1>result.txt;
RESULT=$(< log.txt)
OUTPUT_IN=$(< ./test/out_test.txt)
OUTPUT_OUT=$(< ./test/in_test.txt)

echo $OUTPUT_IN 

if [[ "$RESULT" = ""&&  "$OUTPUT_IN" = "$OUTPUT_OUT" ]]; then
	echo 11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111works 1/2;
fi


#testing unsuccessfull commands
./microbash <test_err.microbash 2>log.txt 1>result.txt;
wc -l log.txt;
RESULT_L=$?;
wc -l test_err.microbash;
TEST_L=$?;
if (( TEST == RESULT_L )); then
	echo 222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222works 2/2;
fi

./microbash <testclean.microbash
echo
