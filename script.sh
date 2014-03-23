trap "kill 0" SIGINT
WS=$1
P=$2
Q=$3
./mim 127.0.0.2 $P $Q &
./server $WS &
(
sleep 0.15
./client 127.0.0.1 $WS $P $Q 
kill $(ps aux | grep "./server" | awk '{print $2}')
kill $(ps aux | grep "./mim" | awk '{print $2}')
)
