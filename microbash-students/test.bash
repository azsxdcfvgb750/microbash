
#testing successfull commands
./microbash <test.microbash 2>log.txt;
RESULT=cat log.txt;
if [ "$RESULT" = "" ]; then
	echo 11111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111works 1/2;
fi


#testing unsuccessfull commands
./microbash <test_err.microbash 2>log.txt;
wc -l log.txt;
RESULT_L=$?
wc -l test_err.microbash;
TEST_L=$?
if (( TEST == RESULT_L )); then
	echo 222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222works 2/2;
fi

./microbash <testclean.microbash

