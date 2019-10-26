rm -f ./init.c
rm -f ./init.o
rm -f ../gemOS.kernel
if [ $# -ge 1 ]; then
	files="$@"
else
	files="init*.c"
fi

for file in $files; do
	rm -f ./init.o
	cp $file init.c
	make -C ../ > /dev/null
	if [ ! -f ../gemOS.kernel ]; then
		printf "\n\n\n======================\n"
		echo "Compile error"
		printf "======================\n\n"
		read -n 1
		echo ""
		exit
	fi
	echo "Setup done for $file"
	rm init.c
	r='y'
	read -p "Run gemOS now. Press 'y' once done, 'q' to quit [y]: " -n 1 r
	echo ""
	if [ $r == 'q' ]; then
		exit
	fi
done
