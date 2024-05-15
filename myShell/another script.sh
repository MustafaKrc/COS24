clear
cat f1.txt ; cat f2.txt | grep hw ; grep odev
cat f1.txt ; cat f2.txt | grep hw ; grep odev
cat f1.txt ; cat f2.txt | grep hw ; grep odev
cat f1.txt ; cat f2.txt | grep hw ; grep odev
cat f1.txt ; cat f2.txt | grep hw ; grep odev
echo "Orders are random because of concurrency"
sleep 10
clear
pwd
ls -a
sleep 3
clear
echo "Waiting only 5 seconds because of concurrency"
sleep 5 ; sleep 5 ; sleep 5 ; sleep 5 ; sleep 5 ; sleep 5 ; sleep 5 ; sleep 5 ; sleep 5 ; sleep 5
sleep 3
clear
echo "My shell" | grep My 
echo "My shell" | grep My | wc
echo "My shell" | grep My | grep shell | wc
sleep 3
clear
pwd
cd ..
pwd
sleep 3
clear
whoami
neofetch
sleep 3
neofetch | grep OS ; grep Host; grep Kernel; grep CPU; grep GPU; grep Memory
echo "Grep commands work concurrent, prompts are printed in random order"
sleep 7
clear
history
history | grep 1